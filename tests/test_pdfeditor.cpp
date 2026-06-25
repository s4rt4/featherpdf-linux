// Feather PDF — light on the system, full-featured on PDF.
// Copyright (C) 2026 Feather PDF contributors. Licensed under GPLv3 (see LICENSE).
//
// Headless tests for the lossless page-arrangement backend (PdfEditor / QPDF):
// rotate, delete, and reorder must produce the right page count and rotations.

#include "backends/PdfEditor.h"

#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryDir>
#include <QtTest>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>

class TestPdfEditor : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_input;
    QString m_source; // a separate 2-page PDF to insert from

    // A simple 3-page PDF (no display needed; runs under QT_QPA_PLATFORM=offscreen).
    void makeSample(const QString& path, int pages) {
        QPdfWriter writer(path);
        writer.setPageSize(QPageSize(QPageSize::A4));
        QPainter painter(&writer);
        for (int i = 0; i < pages; ++i) {
            if (i > 0)
                writer.newPage();
            painter.drawText(100, 100, QStringLiteral("Page %1").arg(i + 1));
        }
        painter.end();
    }

    // Effective /Rotate for a page in the file at `path`.
    int pageRotation(const QString& path, int index) {
        QPDF q;
        q.processFile(path.toLocal8Bit().constData());
        QPDFPageDocumentHelper dh(q);
        dh.pushInheritedAttributesToPage();
        auto pages = dh.getAllPages();
        QPDFObjectHandle rot = pages.at(index).getObjectHandle().getKey("/Rotate");
        return rot.isInteger() ? rot.getIntValueAsInt() : 0;
    }

    int pageCount(const QString& path) {
        QPDF q;
        q.processFile(path.toLocal8Bit().constData());
        return static_cast<int>(QPDFPageDocumentHelper(q).getAllPages().size());
    }

private slots:
    void initTestCase() {
        QVERIFY(m_dir.isValid());
        m_input = m_dir.filePath("input.pdf");
        makeSample(m_input, 3);
        QCOMPARE(pageCount(m_input), 3);
        m_source = m_dir.filePath("source.pdf");
        makeSample(m_source, 2);
        QCOMPARE(pageCount(m_source), 2);
    }

    void identityKeepsAllPages() {
        const QString out = m_dir.filePath("identity.pdf");
        QString err;
        QVERIFY2(PdfEditor::saveArrangement(m_input, out, {0, 1, 2}, {0, 0, 0}, &err),
                 qPrintable(err));
        QCOMPARE(pageCount(out), 3);
    }

    void deleteAndReorder() {
        // Keep original pages 2 and 0 (drop page 1), reversed.
        const QString out = m_dir.filePath("reordered.pdf");
        QString err;
        QVERIFY2(PdfEditor::saveArrangement(m_input, out, {2, 0}, {0, 0}, &err), qPrintable(err));
        QCOMPARE(pageCount(out), 2);
    }

    void rotationIsWritten() {
        const QString out = m_dir.filePath("rotated.pdf");
        QString err;
        QVERIFY2(PdfEditor::saveArrangement(m_input, out, {0, 1, 2}, {90, 180, 0}, &err),
                 qPrintable(err));
        QCOMPARE(pageCount(out), 3);
        QCOMPARE(pageRotation(out, 0), 90);
        QCOMPARE(pageRotation(out, 1), 180);
        QCOMPARE(pageRotation(out, 2), 0);
    }

    void insertAtEndAppends() {
        const QString out = m_dir.filePath("ins-end.pdf");
        QString err;
        QVERIFY2(PdfEditor::insertPages(m_input, out, {0, 1, 2}, {0, 0, 0}, m_source, {0, 1},
                                        /*atSlot=*/3, &err),
                 qPrintable(err));
        QCOMPARE(pageCount(out), 5);
    }

    void insertAtBeginningPrepends() {
        // Base pages carry distinct rotations as markers; the inserted page has
        // none, so its position 0 and the preserved base order are both visible.
        const QString out = m_dir.filePath("ins-begin.pdf");
        QString err;
        QVERIFY2(PdfEditor::insertPages(m_input, out, {0, 1, 2}, {90, 180, 270}, m_source, {0},
                                        /*atSlot=*/0, &err),
                 qPrintable(err));
        QCOMPARE(pageCount(out), 4);
        QCOMPARE(pageRotation(out, 0), 0);   // inserted page, first
        QCOMPARE(pageRotation(out, 1), 90);  // base page 0
        QCOMPARE(pageRotation(out, 2), 180); // base page 1
        QCOMPARE(pageRotation(out, 3), 270); // base page 2
    }

    void insertInMiddleKeepsOrderAndRotation() {
        const QString out = m_dir.filePath("ins-mid.pdf");
        QString err;
        QVERIFY2(PdfEditor::insertPages(m_input, out, {0, 1, 2}, {90, 180, 270}, m_source, {0},
                                        /*atSlot=*/1, &err),
                 qPrintable(err));
        QCOMPARE(pageCount(out), 4);
        QCOMPARE(pageRotation(out, 0), 90);  // base page 0
        QCOMPARE(pageRotation(out, 1), 0);   // inserted page
        QCOMPARE(pageRotation(out, 2), 180); // base page 1
        QCOMPARE(pageRotation(out, 3), 270); // base page 2
    }
};

QTEST_MAIN(TestPdfEditor)
#include "test_pdfeditor.moc"
