// Feather PDF — light on the system, full-featured on PDF.
// Copyright (C) 2026 Feather PDF contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "backends/Signer.h"

#include <QDateTime>
#include <QFile>
#include <QObject>

#include <memory>

#include <poppler-converter.h>
#include <poppler-form.h>
#include <poppler-qt6.h>

namespace {
QString statusText(Poppler::SignatureValidationInfo::SignatureStatus s, bool* valid) {
    *valid = false;
    switch (s) {
    case Poppler::SignatureValidationInfo::SignatureValid:
        *valid = true;
        return QObject::tr("Valid — the document is intact.");
    case Poppler::SignatureValidationInfo::SignatureDigestMismatch:
        return QObject::tr("The document was changed after it was signed.");
    case Poppler::SignatureValidationInfo::SignatureInvalid:
        return QObject::tr("Invalid signature.");
    case Poppler::SignatureValidationInfo::SignatureDecodingError:
        return QObject::tr("The signature is malformed.");
    case Poppler::SignatureValidationInfo::SignatureNotFound:
        return QObject::tr("No signature found.");
    default:
        return QObject::tr("The signature could not be verified.");
    }
}
} // namespace

QStringList Signer::availableCertificates() {
    QStringList names;
    for (const Poppler::CertificateInfo& c : Poppler::getAvailableSigningCertificates()) {
        const QString cn = c.subjectInfo(Poppler::CertificateInfo::CommonName);
        names << (cn.isEmpty() ? c.nickName() : cn);
    }
    return names;
}

bool Signer::sign(const QString& inputPath, const QString& outputPath, const QString& certName,
                  const QString& password, const QString& reason, const QString& location,
                  int page, const QRectF& rect, QString* error) {
    // Resolve the display name back to the NSS nickname the backend signs with.
    QString nickname;
    for (const Poppler::CertificateInfo& c : Poppler::getAvailableSigningCertificates()) {
        const QString cn = c.subjectInfo(Poppler::CertificateInfo::CommonName);
        if (certName == cn || certName == c.nickName()) {
            nickname = c.nickName();
            break;
        }
    }
    if (nickname.isEmpty()) {
        if (error)
            *error = QStringLiteral("That signing certificate is no longer available.");
        return false;
    }

    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(inputPath);
    if (!doc || doc->isLocked()) {
        if (error)
            *error = QStringLiteral("The document couldn't be opened for signing.");
        return false;
    }

    const QString target = outputPath + QStringLiteral(".feather-tmp");
    std::unique_ptr<Poppler::PDFConverter> conv = doc->pdfConverter();
    conv->setOutputFileName(target);

    Poppler::PDFConverter::NewSignatureData data;
    data.setCertNickname(nickname);
    data.setPassword(password);
    data.setPage(page);
    data.setBoundingRectangle(rect);
    QString text = certName;
    if (!reason.isEmpty())
        text += QStringLiteral("\n") + reason;
    data.setSignatureText(QObject::tr("Digitally signed by\n%1").arg(text));
    if (!reason.isEmpty())
        data.setReason(reason);
    if (!location.isEmpty())
        data.setLocation(location);
    // A unique field name per signature so multiple signatures can coexist.
    data.setFieldPartialName(
        QStringLiteral("FeatherSig%1").arg(QDateTime::currentMSecsSinceEpoch()));

    if (!conv->sign(data)) {
        if (error)
            *error = QStringLiteral("Signing failed. Check the certificate password.");
        QFile::remove(target);
        return false;
    }

    QFile::remove(outputPath);
    if (!QFile::rename(target, outputPath)) {
        if (error)
            *error = QStringLiteral("The signed file couldn't replace the original.");
        QFile::remove(target);
        return false;
    }
    return true;
}

QList<Signer::SignatureInfo> Signer::verify(const QString& path) {
    QList<SignatureInfo> out;
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(path);
    if (!doc || doc->isLocked())
        return out;

    for (const auto& sig : doc->signatures()) {
        const Poppler::SignatureValidationInfo info =
            sig->validate(Poppler::FormFieldSignature::ValidateVerifyCertificate);
        SignatureInfo s;
        s.signer = info.signerName();
        s.status = statusText(info.signatureStatus(), &s.valid);
        const time_t t = info.signingTime();
        if (t > 0)
            s.time = QDateTime::fromSecsSinceEpoch(t).toString(Qt::TextDate);
        s.reason = info.reason();
        s.location = info.location();
        out.append(s);
    }
    return out;
}
