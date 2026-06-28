// Feather PDF — light on the system, full-featured on PDF.
// Copyright (C) 2026 Feather PDF contributors. Licensed under GPLv3 (see LICENSE).
//
// Exercises the advanced signing backend: a graphical (image) signature
// appearance produces a valid signature, and the RFC 3161 trusted-timestamp
// helper fails gracefully when it has nothing to talk to. The signing half
// builds a throwaway self-signed certificate in a temporary NSS database; if
// the NSS tooling isn't present (or Poppler can't see the cert) those checks
// skip rather than fail, so the suite stays green on minimal build hosts.

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
};

QTEST_MAIN(TestSigner)
#include "test_signer.moc"
