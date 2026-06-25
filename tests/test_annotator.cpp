// Feather PDF — light on the system, full-featured on PDF.
// Copyright (C) 2026 Feather PDF contributors. Licensed under GPLv3 (see LICENSE).
//
// Headless tests for the annotation backend (Annotator / QPDF): the vector
// markup/shape annotations land on the page with the right /Subtype.

#include "backends/Annotator.h"

#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSet>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>

class TestAnnotator : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_input;

    void makeSample(const QString& path) {
        QPdfWriter writer(path);
        writer.setPageSize(QPageSize(QPageSize::A4));
        QPainter painter(&writer);
        painter.drawText(100, 100, QStringLiteral("Page 1"));
        painter.end();
    }

    QSet<QString> subtypesOnPage0(const QString& path) {
        QPDF q;
        q.processFile(path.toLocal8Bit().constData());
        auto pages = QPDFPageDocumentHelper(q).getAllPages();
        QSet<QString> out;
        QPDFObjectHandle annots = pages.at(0).getObjectHandle().getKey("/Annots");
        if (annots.isArray())
            for (int i = 0; i < annots.getArrayNItems(); ++i)
                out.insert(QString::fromStdString(annots.getArrayItem(i).getKey("/Subtype").getName()));
        return out;
    }

private slots:
    void initTestCase() {
        QVERIFY(m_dir.isValid());
        m_input = m_dir.filePath("input.pdf");
        makeSample(m_input);
    }

    void writesMarkupAndShapeSubtypes() {
        const QString out = m_dir.filePath("shapes.pdf");
        QList<Annotator::Shape> shapes{
            {0, Annotator::Shape::Kind::Underline, QRectF(0.1, 0.1, 0.3, 0.02), QColor(Qt::red)},
            {0, Annotator::Shape::Kind::StrikeOut, QRectF(0.1, 0.2, 0.3, 0.02), QColor(Qt::blue)},
            {0, Annotator::Shape::Kind::Rectangle, QRectF(0.1, 0.3, 0.3, 0.2), QColor(Qt::green)},
        };
        QString err;
        QVERIFY2(Annotator::saveAnnotations(m_input, out, {}, {}, {}, shapes, &err), qPrintable(err));

        const QSet<QString> subtypes = subtypesOnPage0(out);
        QVERIFY(subtypes.contains(QStringLiteral("/Underline")));
        QVERIFY(subtypes.contains(QStringLiteral("/StrikeOut")));
        QVERIFY(subtypes.contains(QStringLiteral("/Square")));
    }

    void rejectsEmpty() {
        const QString out = m_dir.filePath("empty.pdf");
        QString err;
        QVERIFY(!Annotator::saveAnnotations(m_input, out, {}, {}, {}, {}, &err));
        QVERIFY(!err.isEmpty());
    }
};

QTEST_MAIN(TestAnnotator)
#include "test_annotator.moc"
