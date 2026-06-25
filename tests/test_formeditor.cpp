// Feather PDF — light on the system, full-featured on PDF.
// Copyright (C) 2026 Feather PDF contributors. Licensed under GPLv3 (see LICENSE).
//
// Headless tests for the form-authoring backend (FormEditor / QPDF): a created
// field lands in the catalog's /AcroForm and on the page, with the right type.

#include "backends/FormEditor.h"

#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QRectF>
#include <QTemporaryDir>
#include <QtTest>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>

class TestFormEditor : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_input;

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

    QPDFObjectHandle acroFields(QPDF& q) {
        return q.getRoot().getKey("/AcroForm").getKey("/Fields");
    }

private slots:
    void initTestCase() {
        QVERIFY(m_dir.isValid());
        m_input = m_dir.filePath("input.pdf");
        makeSample(m_input, 2);
    }

    void addsTextField() {
        const QString out = m_dir.filePath("text.pdf");
        FormEditor::NewField f;
        f.type = FormEditor::Type::Text;
        f.name = "full_name";
        f.defaultValue = "hi";
        f.page = 0;
        f.rect = QRectF(0.1, 0.1, 0.3, 0.04);
        QString err;
        QVERIFY2(FormEditor::addField(m_input, out, f, &err), qPrintable(err));

        QPDF q;
        q.processFile(out.toLocal8Bit().constData());
        QPDFObjectHandle acro = q.getRoot().getKey("/AcroForm");
        QVERIFY(acro.isDictionary());
        QVERIFY(acro.getKey("/NeedAppearances").isBool());
        QVERIFY(acro.getKey("/NeedAppearances").getBoolValue());

        QPDFObjectHandle fields = acroFields(q);
        QVERIFY(fields.isArray());
        QCOMPARE(fields.getArrayNItems(), 1);
        QPDFObjectHandle field = fields.getArrayItem(0);
        QCOMPARE(QString::fromStdString(field.getKey("/T").getUTF8Value()),
                 QStringLiteral("full_name"));
        QCOMPARE(QString::fromStdString(field.getKey("/FT").getName()), QStringLiteral("/Tx"));
        QCOMPARE(QString::fromStdString(field.getKey("/V").getUTF8Value()), QStringLiteral("hi"));

        // The widget is also on the page it targets.
        auto pages = QPDFPageDocumentHelper(q).getAllPages();
        QPDFObjectHandle annots = pages.at(0).getObjectHandle().getKey("/Annots");
        QVERIFY(annots.isArray());
        QVERIFY(annots.getArrayNItems() >= 1);
    }

    void addsCheckedCheckBox() {
        const QString out = m_dir.filePath("check.pdf");
        FormEditor::NewField f;
        f.type = FormEditor::Type::CheckBox;
        f.name = "agree";
        f.checked = true;
        f.page = 0;
        f.rect = QRectF(0.1, 0.1, 0.03, 0.02);
        QString err;
        QVERIFY2(FormEditor::addField(m_input, out, f, &err), qPrintable(err));

        QPDF q;
        q.processFile(out.toLocal8Bit().constData());
        QPDFObjectHandle field = acroFields(q).getArrayItem(0);
        QCOMPARE(QString::fromStdString(field.getKey("/FT").getName()), QStringLiteral("/Btn"));
        QCOMPARE(QString::fromStdString(field.getKey("/V").getName()), QStringLiteral("/Yes"));
        QCOMPARE(QString::fromStdString(field.getKey("/AS").getName()), QStringLiteral("/Yes"));
    }

    void addsDropdownWithOptions() {
        const QString out = m_dir.filePath("drop.pdf");
        FormEditor::NewField f;
        f.type = FormEditor::Type::Dropdown;
        f.name = "color";
        f.options = {"Red", "Green", "Blue"};
        f.page = 1;
        f.rect = QRectF(0.1, 0.1, 0.3, 0.04);
        QString err;
        QVERIFY2(FormEditor::addField(m_input, out, f, &err), qPrintable(err));

        QPDF q;
        q.processFile(out.toLocal8Bit().constData());
        QPDFObjectHandle field = acroFields(q).getArrayItem(0);
        QCOMPARE(QString::fromStdString(field.getKey("/FT").getName()), QStringLiteral("/Ch"));
        QVERIFY(field.getKey("/Ff").getIntValueAsInt() & (1 << 17)); // combo flag
        QPDFObjectHandle opt = field.getKey("/Opt");
        QVERIFY(opt.isArray());
        QCOMPARE(opt.getArrayNItems(), 3);
        QCOMPARE(QString::fromStdString(opt.getArrayItem(0).getUTF8Value()),
                 QStringLiteral("Red"));
        QCOMPARE(QString::fromStdString(field.getKey("/V").getUTF8Value()), QStringLiteral("Red"));
    }

    void accumulatesFields() {
        const QString one = m_dir.filePath("one.pdf");
        const QString two = m_dir.filePath("two.pdf");
        FormEditor::NewField a;
        a.type = FormEditor::Type::Text;
        a.name = "a";
        a.page = 0;
        a.rect = QRectF(0.1, 0.1, 0.3, 0.04);
        QString err;
        QVERIFY2(FormEditor::addField(m_input, one, a, &err), qPrintable(err));
        FormEditor::NewField b = a;
        b.name = "b";
        QVERIFY2(FormEditor::addField(one, two, b, &err), qPrintable(err));

        QPDF q;
        q.processFile(two.toLocal8Bit().constData());
        QCOMPARE(acroFields(q).getArrayNItems(), 2);
    }

    void addsRadioGroup() {
        const QString out = m_dir.filePath("radio.pdf");
        QString err;
        QVERIFY2(FormEditor::addRadioGroup(m_input, out, "plan", {"Basic", "Pro", "Max"}, 0,
                                           QRectF(0.1, 0.1, 0.03, 0.02), &err),
                 qPrintable(err));

        QPDF q;
        q.processFile(out.toLocal8Bit().constData());
        QPDFObjectHandle fields = acroFields(q);
        QCOMPARE(fields.getArrayNItems(), 1); // the parent only
        QPDFObjectHandle parent = fields.getArrayItem(0);
        QCOMPARE(QString::fromStdString(parent.getKey("/FT").getName()), QStringLiteral("/Btn"));
        QVERIFY(parent.getKey("/Ff").getIntValueAsInt() & (1 << 15)); // radio flag
        QPDFObjectHandle kids = parent.getKey("/Kids");
        QVERIFY(kids.isArray());
        QCOMPARE(kids.getArrayNItems(), 3);

        QPDFObjectHandle kid = kids.getArrayItem(0);
        QCOMPARE(QString::fromStdString(kid.getKey("/Subtype").getName()),
                 QStringLiteral("/Widget"));
        QVERIFY(kid.getKey("/Parent").getObjGen() == parent.getObjGen());
        QCOMPARE(QString::fromStdString(kid.getKey("/AS").getName()), QStringLiteral("/Off"));
        // /AP /N defines two states: the export value and /Off.
        QPDFObjectHandle apN = kid.getKey("/AP").getKey("/N");
        QVERIFY(apN.isDictionary());
        QCOMPARE(static_cast<int>(apN.getKeys().size()), 2);

        // All three buttons are on the page.
        auto pages = QPDFPageDocumentHelper(q).getAllPages();
        QCOMPARE(pages.at(0).getObjectHandle().getKey("/Annots").getArrayNItems(), 3);
    }

    void rejectsTooFewRadioOptions() {
        const QString out = m_dir.filePath("radio-bad.pdf");
        QString err;
        QVERIFY(!FormEditor::addRadioGroup(m_input, out, "x", {"only"}, 0,
                                           QRectF(0.1, 0.1, 0.03, 0.02), &err));
        QVERIFY(!err.isEmpty());
    }

    void rejectsEmptyName() {
        const QString out = m_dir.filePath("noname.pdf");
        FormEditor::NewField f;
        f.type = FormEditor::Type::Text;
        f.name = "   ";
        f.page = 0;
        f.rect = QRectF(0.1, 0.1, 0.3, 0.04);
        QString err;
        QVERIFY(!FormEditor::addField(m_input, out, f, &err));
        QVERIFY(!err.isEmpty());
    }
};

QTEST_MAIN(TestFormEditor)
#include "test_formeditor.moc"
