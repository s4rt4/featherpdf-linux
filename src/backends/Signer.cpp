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
#include <QFileInfo>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

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
        return QObject::tr("Valid - the document is intact.");
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
                  int page, const QRectF& rect, const QString& imagePath, QString* error) {
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
    // A graphical signature: draw the chosen image behind the appearance widget.
    if (!imagePath.isEmpty() && QFileInfo::exists(imagePath))
        data.setImagePath(imagePath);
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

bool Signer::timestamp(const QString& filePath, const QString& tsaUrl,
                       const QString& outTokenPath, QString* error) {
    const auto fail = [&](const QString& m) {
        if (error)
            *error = m;
        return false;
    };
    const QString openssl = QStandardPaths::findExecutable(QStringLiteral("openssl"));
    const QString curl = QStandardPaths::findExecutable(QStringLiteral("curl"));
    if (openssl.isEmpty() || curl.isEmpty())
        return fail(QObject::tr("Trusted timestamping needs 'openssl' and 'curl' on your PATH."));
    if (tsaUrl.isEmpty())
        return fail(QObject::tr("No timestamp authority (TSA) URL was given."));
    if (!QFileInfo::exists(filePath))
        return fail(QObject::tr("The file to timestamp no longer exists."));

    QTemporaryDir tmp;
    if (!tmp.isValid())
        return fail(QObject::tr("Couldn't create a temporary working directory."));
    const QString tsq = tmp.filePath(QStringLiteral("request.tsq"));
    const QString tsr = tmp.filePath(QStringLiteral("response.tsr"));

    const auto okRun = [](const QString& prog, const QStringList& args, int ms) {
        QProcess p;
        p.start(prog, args);
        return p.waitForFinished(ms) && p.exitStatus() == QProcess::NormalExit
            && p.exitCode() == 0;
    };

    // 1) Build an RFC 3161 query over the file's SHA-256 (request the TSA's cert).
    if (!okRun(openssl,
               {QStringLiteral("ts"), QStringLiteral("-query"), QStringLiteral("-data"), filePath,
                QStringLiteral("-sha256"), QStringLiteral("-cert"), QStringLiteral("-out"), tsq},
               15000))
        return fail(QObject::tr("Couldn't build the timestamp request."));

    // 2) POST it to the timestamp authority.
    if (!okRun(curl,
               {QStringLiteral("-s"), QStringLiteral("-S"), QStringLiteral("--max-time"),
                QStringLiteral("25"), QStringLiteral("-H"),
                QStringLiteral("Content-Type: application/timestamp-query"),
                QStringLiteral("--data-binary"), QStringLiteral("@") + tsq, tsaUrl,
                QStringLiteral("-o"), tsr},
               30000))
        return fail(QObject::tr("Couldn't reach the timestamp authority. Check the TSA URL and "
                                "your connection."));

    // 3) Confirm the reply is a well-formed RFC 3161 timestamp response.
    if (!okRun(openssl,
               {QStringLiteral("ts"), QStringLiteral("-reply"), QStringLiteral("-in"), tsr,
                QStringLiteral("-text")},
               15000))
        return fail(QObject::tr("The timestamp authority's reply wasn't a valid RFC 3161 token."));

    // 4) Save the token next to the signed file.
    QFile::remove(outTokenPath);
    if (!QFile::copy(tsr, outTokenPath))
        return fail(QObject::tr("Couldn't save the timestamp token."));
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
