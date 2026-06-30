// Feather PDF — light on the system, full-featured on PDF.
// Copyright (C) 2026 Feather PDF contributors. Licensed under GPLv3 (see LICENSE).
//
// Exercises the advanced signing backend: a graphical (image) signature
// appearance produces a valid signature, and the RFC 3161 trusted-timestamp
// helper fails gracefully when it has nothing to talk to. The signing half
// builds a throwaway self-signed certificate in a temporary NSS database; if
// the NSS tooling isn't present (or Poppler can't see the cert) those checks
// skip rather than fail, so the suite stays green on minimal build hosts.

#include "backends/LtvSigner.h"
#include "backends/Signer.h"

#include <QImage>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include <poppler-form.h>

#include <memory>

#include <qpdf/Buffer.hh>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>

namespace {
bool run(const QString& prog, const QStringList& args, int ms = 30000) {
    QProcess p;
    p.start(prog, args);
    return p.waitForFinished(ms) && p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}
} // namespace

class TestSigner : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_pdf;
    QString m_png;
    bool m_haveCert = false;

private slots:
    void initTestCase() {
        QVERIFY(m_dir.isValid());
        const QString d = m_dir.path();

        // A one-page PDF to sign, and a PNG to use as the graphical appearance.
        m_pdf = d + QStringLiteral("/in.pdf");
        {
            QPdfWriter w(m_pdf);
            w.setPageSize(QPageSize(QPageSize::A4));
            QPainter pa(&w);
            pa.drawText(120, 120, QStringLiteral("Feather signing test"));
            pa.end();
        }
        QVERIFY(QFileInfo::exists(m_pdf));

        m_png = d + QStringLiteral("/sig.png");
        {
            QImage img(220, 90, QImage::Format_ARGB32);
            img.fill(Qt::white);
            QPainter pa(&img);
            pa.setPen(Qt::darkBlue);
            pa.drawText(12, 50, QStringLiteral("V. Signature"));
            pa.end();
            QVERIFY(img.save(m_png));
        }

        // Best-effort: stand up a self-signed cert in a temp NSS DB so the
        // image-signature test has something to sign with.
        const QString openssl = QStandardPaths::findExecutable(QStringLiteral("openssl"));
        const QString certutil = QStandardPaths::findExecutable(QStringLiteral("certutil"));
        const QString pk12util = QStandardPaths::findExecutable(QStringLiteral("pk12util"));
        if (openssl.isEmpty() || certutil.isEmpty() || pk12util.isEmpty())
            return;

        const QString key = d + QStringLiteral("/k.pem");
        const QString crt = d + QStringLiteral("/c.pem");
        const QString p12 = d + QStringLiteral("/c.p12");
        const QString sqldb = QStringLiteral("sql:") + d;
        const bool built =
            run(openssl, {"req", "-x509", "-newkey", "rsa:2048", "-keyout", key, "-out", crt,
                          "-days", "2", "-nodes", "-subj", "/CN=Feather Test Signer"})
            && run(openssl, {"pkcs12", "-export", "-in", crt, "-inkey", key, "-out", p12,
                             "-passout", "pass:", "-name", "Feather Test Signer"})
            && run(certutil, {"-N", "-d", sqldb, "--empty-password"})
            && run(pk12util, {"-d", sqldb, "-i", p12, "-W", ""});
        if (!built)
            return;

        Poppler::setNSSDir(d);
        m_haveCert = Signer::availableCertificates().contains(QStringLiteral("Feather Test Signer"));
    }

    // A graphical signature signs cleanly and validates as intact.
    void signWithImageProducesValidSignature() {
        if (!m_haveCert)
            QSKIP("NSS signing certificate unavailable in this environment");

        const QString out = m_dir.path() + QStringLiteral("/signed.pdf");
        QString error;
        const bool ok = Signer::sign(m_pdf, out, QStringLiteral("Feather Test Signer"),
                                     QString(), QStringLiteral("Approved"),
                                     QStringLiteral("Jakarta"), 0, QRectF(48, 40, 220, 90), m_png,
                                     &error);
        QVERIFY2(ok, qPrintable(error));
        QVERIFY(QFileInfo::exists(out));

        const QList<Signer::SignatureInfo> sigs = Signer::verify(out);
        QCOMPARE(sigs.size(), 1);
        QVERIFY2(sigs.first().valid, qPrintable(sigs.first().status));
        QCOMPARE(sigs.first().signer, QStringLiteral("Feather Test Signer"));
    }

    // Signing without an image still works (the text appearance path is unaffected).
    void signWithoutImageStillWorks() {
        if (!m_haveCert)
            QSKIP("NSS signing certificate unavailable in this environment");

        const QString out = m_dir.path() + QStringLiteral("/signed-text.pdf");
        QString error;
        const bool ok = Signer::sign(m_pdf, out, QStringLiteral("Feather Test Signer"),
                                     QString(), QString(), QString(), 0,
                                     QRectF(48, 40, 220, 60), QString(), &error);
        QVERIFY2(ok, qPrintable(error));
        QCOMPARE(Signer::verify(out).size(), 1);
    }

    // The timestamp helper refuses an empty TSA URL up front.
    void timestampRejectsEmptyUrl() {
        QString error;
        const QString token = m_dir.path() + QStringLiteral("/a.tsr");
        QVERIFY(!Signer::timestamp(m_pdf, QString(), token, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFileInfo::exists(token));
    }

    // And fails gracefully (no crash, clear message, no half-written token) when
    // the TSA can't be reached. 127.0.0.1:1 refuses the connection immediately.
    void timestampFailsWhenTsaUnreachable() {
        QString error;
        const QString token = m_dir.path() + QStringLiteral("/b.tsr");
        const bool ok =
            Signer::timestamp(m_pdf, QStringLiteral("http://127.0.0.1:1/"), token, &error);
        QVERIFY(!ok);
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFileInfo::exists(token));
    }

    // PKCS#11: registering a security module makes it appear in the device list,
    // and removing it takes it away. Uses a real module that's present on the box
    // (no physical token needed to register one) against a throwaway NSS DB.
    void securityDeviceRegisterListRemove() {
        if (!Signer::hasSecurityDeviceTools())
            QSKIP("modutil (nss-tools) not available");
        // p11-kit-trust is the system trust module: it always loads (no reader or
        // pcscd needed), so it's a reliable stand-in for exercising the register
        // / list / remove plumbing. A real token uses e.g. opensc-pkcs11.so.
        QString module;
        for (const QString& c : {QStringLiteral("/usr/lib64/pkcs11/p11-kit-trust.so"),
                                 QStringLiteral("/usr/lib/pkcs11/p11-kit-trust.so"),
                                 QStringLiteral("/usr/lib/x86_64-linux-gnu/pkcs11/p11-kit-trust.so")})
            if (QFileInfo::exists(c)) {
                module = c;
                break;
            }
        if (module.isEmpty())
            QSKIP("no loadable PKCS#11 module available to register");

        QTemporaryDir nss;
        QVERIFY(nss.isValid());
        Signer::useNssDatabase(nss.path());

        QString err;
        QVERIFY2(Signer::addSecurityDevice(QStringLiteral("FeatherTestToken"), module, &err),
                 qPrintable(err));
        QVERIFY2(Signer::securityDevices().contains(QStringLiteral("FeatherTestToken")),
                 qPrintable(Signer::securityDevices().join(QStringLiteral(", "))));
        QVERIFY2(Signer::removeSecurityDevice(QStringLiteral("FeatherTestToken"), &err),
                 qPrintable(err));
        QVERIFY(!Signer::securityDevices().contains(QStringLiteral("FeatherTestToken")));
    }

    // Long-term validation: adding a DSS to a signed document embeds the signer's
    // certificate chain, keeps the original signature intact (the incremental
    // update never touches the signed byte range), and records a VRI entry keyed by
    // the signature. Runs fully offline — a self-signed cert has no OCSP/CRL, so the
    // DSS holds just the certificate(s), which is still valid LTV material.
    void ltvAddsDssAndPreservesSignature() {
        if (!m_haveCert)
            QSKIP("NSS signing certificate unavailable in this environment");

        const QString signed_ = m_dir.path() + QStringLiteral("/ltv-in.pdf");
        QString error;
        QVERIFY2(Signer::sign(m_pdf, signed_, QStringLiteral("Feather Test Signer"), QString(),
                              QStringLiteral("Approved"), QStringLiteral("Jakarta"), 0,
                              QRectF(48, 40, 220, 60), QString(), &error),
                 qPrintable(error));
        QVERIFY(Signer::verify(signed_).size() == 1 && Signer::verify(signed_).first().valid);

        const QString out = m_dir.path() + QStringLiteral("/ltv-out.pdf");
        LtvSigner::Result res;
        QVERIFY2(LtvSigner::addValidationInfo(signed_, out, &error, &res), qPrintable(error));
        QVERIFY(QFileInfo::exists(out));

        // The store carried at least the signer's own certificate.
        QCOMPARE(res.signatures, 1);
        QVERIFY(res.certs >= 1);

        // The original signature still validates: the byte range wasn't disturbed.
        const QList<Signer::SignatureInfo> sigs = Signer::verify(out);
        QCOMPARE(sigs.size(), 1);
        QVERIFY2(sigs.first().valid, qPrintable(sigs.first().status));

        // The catalog now references a /DSS with a non-empty /Certs and a /VRI.
        QPDF pdf;
        pdf.processFile(out.toLocal8Bit().constData());
        QPDFObjectHandle dss = pdf.getRoot().getKey("/DSS");
        QVERIFY(dss.isDictionary());
        QPDFObjectHandle certs = dss.getKey("/Certs");
        QVERIFY(certs.isArray());
        QVERIFY(certs.getArrayNItems() >= 1);
        QVERIFY(dss.getKey("/VRI").isDictionary());

        // The embedded cert really is a certificate stream (DER starts with a SEQUENCE).
        QPDFObjectHandle cert0 = certs.getArrayItem(0);
        std::shared_ptr<Buffer> buf = cert0.getStreamData();
        QVERIFY(buf && buf->getSize() > 0);
        QCOMPARE(static_cast<unsigned char>(buf->getBuffer()[0]), static_cast<unsigned char>(0x30));
    }

    // LTV refuses a document that has no signatures, with a clear message.
    void ltvRejectsUnsignedDocument() {
        const QString out = m_dir.path() + QStringLiteral("/ltv-unsigned.pdf");
        QString error;
        QVERIFY(!LtvSigner::addValidationInfo(m_pdf, out, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFileInfo::exists(out));
    }

    // A missing module path is rejected before touching NSS.
    void addSecurityDeviceRejectsMissingModule() {
        if (!Signer::hasSecurityDeviceTools())
            QSKIP("modutil (nss-tools) not available");
        QTemporaryDir nss;
        QVERIFY(nss.isValid());
        Signer::useNssDatabase(nss.path());
        QString err;
        QVERIFY(!Signer::addSecurityDevice(QStringLiteral("X"),
                                           QStringLiteral("/no/such/module.so"), &err));
        QVERIFY(!err.isEmpty());
    }
};

QTEST_MAIN(TestSigner)
#include "test_signer.moc"
