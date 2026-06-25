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

#include "backends/FormEditor.h"

#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <exception>
#include <vector>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>
#include <qpdf/QPDFWriter.hh>

namespace {
// Field flag bits (PDF 32000-1, table 226/227/230), 1-based in the spec.
constexpr int kFfPushButton = 1 << 16; // /Btn: pushbutton (no value)
constexpr int kFfCombo = 1 << 17;      // /Ch: dropdown rather than list box

// The page's MediaBox as [x0,y0,x1,y1], normalized so x0<x1, y0<y1.
bool mediaBox(QPDFObjectHandle page, double out[4]) {
    QPDFObjectHandle b = page.getKey("/MediaBox");
    if (!b.isArray() || b.getArrayNItems() != 4)
        return false;
    double v[4];
    for (int i = 0; i < 4; ++i) {
        QPDFObjectHandle e = b.getArrayItem(i);
        if (!e.isNumber())
            return false;
        v[i] = e.getNumericValue();
    }
    out[0] = std::min(v[0], v[2]);
    out[1] = std::min(v[1], v[3]);
    out[2] = std::max(v[0], v[2]);
    out[3] = std::max(v[1], v[3]);
    return true;
}

// Ensure the catalog has an /AcroForm with a Helvetica default resource and a
// default appearance, and return it (and its /Fields array via the out-param).
QPDFObjectHandle ensureAcroForm(QPDF& qpdf, QPDFObjectHandle& fieldsOut) {
    QPDFObjectHandle root = qpdf.getRoot();
    QPDFObjectHandle acro = root.getKey("/AcroForm");
    if (!acro.isDictionary()) {
        acro = qpdf.makeIndirectObject(QPDFObjectHandle::newDictionary());
        root.replaceKey("/AcroForm", acro);
    }
    QPDFObjectHandle fields = acro.getKey("/Fields");
    if (!fields.isArray()) {
        fields = QPDFObjectHandle::newArray();
        acro.replaceKey("/Fields", fields);
    }
    // Regenerate appearances on open/save (so the new field is actually drawn).
    acro.replaceKey("/NeedAppearances", QPDFObjectHandle::newBool(true));
    if (!acro.getKey("/DA").isString())
        acro.replaceKey("/DA", QPDFObjectHandle::newString("/Helv 0 Tf 0 g"));

    QPDFObjectHandle dr = acro.getKey("/DR");
    if (!dr.isDictionary()) {
        dr = QPDFObjectHandle::newDictionary();
        acro.replaceKey("/DR", dr);
    }
    QPDFObjectHandle fonts = dr.getKey("/Font");
    if (!fonts.isDictionary()) {
        fonts = QPDFObjectHandle::newDictionary();
        dr.replaceKey("/Font", fonts);
    }
    if (!fonts.getKey("/Helv").isDictionary()) {
        QPDFObjectHandle helv = qpdf.makeIndirectObject(QPDFObjectHandle::newDictionary());
        helv.replaceKey("/Type", QPDFObjectHandle::newName("/Font"));
        helv.replaceKey("/Subtype", QPDFObjectHandle::newName("/Type1"));
        helv.replaceKey("/BaseFont", QPDFObjectHandle::newName("/Helvetica"));
        fonts.replaceKey("/Helv", helv);
    }
    fieldsOut = fields;
    return acro;
}
} // namespace

bool FormEditor::addField(const QString& inputPath, const QString& outputPath,
                          const NewField& field, QString* error) {
    if (field.name.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("Give the field a name.");
        return false;
    }

    const bool inPlace =
        QFileInfo(inputPath).absoluteFilePath() == QFileInfo(outputPath).absoluteFilePath();
    const QString target = inPlace ? outputPath + QStringLiteral(".feather-tmp") : outputPath;

    try {
        QPDF qpdf;
        qpdf.processFile(inputPath.toLocal8Bit().constData());

        QPDFPageDocumentHelper dh(qpdf);
        std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
        const int n = static_cast<int>(pages.size());
        if (n == 0) {
            if (error)
                *error = QStringLiteral("The document has no pages.");
            if (inPlace)
                QFile::remove(target);
            return false;
        }
        const int pg = std::max(0, std::min(field.page, n - 1));
        QPDFObjectHandle pageObj = pages[pg].getObjectHandle();

        double mb[4];
        if (!mediaBox(pageObj, mb)) {
            if (error)
                *error = QStringLiteral("The target page has no media box.");
            if (inPlace)
                QFile::remove(target);
            return false;
        }
        const double W = mb[2] - mb[0], H = mb[3] - mb[1];
        // Normalized rect uses a top-left origin; PDF space is y-up.
        const QRectF& r = field.rect;
        const double x0 = mb[0] + r.left() * W;
        const double x1 = mb[0] + r.right() * W;
        const double yTop = mb[3] - r.top() * H;
        const double yBot = mb[3] - r.bottom() * H;
        QPDFObjectHandle rect = QPDFObjectHandle::newArray();
        rect.appendItem(QPDFObjectHandle::newReal(std::min(x0, x1), 2));
        rect.appendItem(QPDFObjectHandle::newReal(std::min(yTop, yBot), 2));
        rect.appendItem(QPDFObjectHandle::newReal(std::max(x0, x1), 2));
        rect.appendItem(QPDFObjectHandle::newReal(std::max(yTop, yBot), 2));

        // Combined field/widget annotation.
        QPDFObjectHandle node = qpdf.makeIndirectObject(QPDFObjectHandle::newDictionary());
        node.replaceKey("/Type", QPDFObjectHandle::newName("/Annot"));
        node.replaceKey("/Subtype", QPDFObjectHandle::newName("/Widget"));
        node.replaceKey("/T", QPDFObjectHandle::newUnicodeString(field.name.toStdString()));
        node.replaceKey("/P", pageObj);
        node.replaceKey("/F", QPDFObjectHandle::newInteger(4)); // Print
        node.replaceKey("/Rect", rect);

        switch (field.type) {
        case Type::Text:
            node.replaceKey("/FT", QPDFObjectHandle::newName("/Tx"));
            node.replaceKey("/DA", QPDFObjectHandle::newString("/Helv 0 Tf 0 g"));
            if (!field.defaultValue.isEmpty()) {
                node.replaceKey("/V",
                                QPDFObjectHandle::newUnicodeString(field.defaultValue.toStdString()));
                node.replaceKey("/DV",
                                QPDFObjectHandle::newUnicodeString(field.defaultValue.toStdString()));
            }
            break;
        case Type::CheckBox: {
            node.replaceKey("/FT", QPDFObjectHandle::newName("/Btn"));
            const char* state = field.checked ? "/Yes" : "/Off";
            node.replaceKey("/V", QPDFObjectHandle::newName(state));
            node.replaceKey("/AS", QPDFObjectHandle::newName(state));
            break;
        }
        case Type::Dropdown: {
            node.replaceKey("/FT", QPDFObjectHandle::newName("/Ch"));
            node.replaceKey("/Ff", QPDFObjectHandle::newInteger(kFfCombo));
            node.replaceKey("/DA", QPDFObjectHandle::newString("/Helv 0 Tf 0 g"));
            QPDFObjectHandle opt = QPDFObjectHandle::newArray();
            for (const QString& o : field.options)
                opt.appendItem(QPDFObjectHandle::newUnicodeString(o.toStdString()));
            node.replaceKey("/Opt", opt);
            const QString def = !field.defaultValue.isEmpty() ? field.defaultValue
                                : (!field.options.isEmpty() ? field.options.first() : QString());
            if (!def.isEmpty())
                node.replaceKey("/V", QPDFObjectHandle::newUnicodeString(def.toStdString()));
            break;
        }
        case Type::PushButton: {
            node.replaceKey("/FT", QPDFObjectHandle::newName("/Btn"));
            node.replaceKey("/Ff", QPDFObjectHandle::newInteger(kFfPushButton));
            QPDFObjectHandle mk = QPDFObjectHandle::newDictionary();
            const QString caption =
                !field.defaultValue.isEmpty() ? field.defaultValue : field.name;
            mk.replaceKey("/CA", QPDFObjectHandle::newString(caption.toStdString()));
            node.replaceKey("/MK", mk);
            break;
        }
        }

        // Attach to the page's annotations and the AcroForm field list.
        QPDFObjectHandle annots = pageObj.getKey("/Annots");
        if (!annots.isArray()) {
            annots = QPDFObjectHandle::newArray();
            pageObj.replaceKey("/Annots", annots);
        }
        annots.appendItem(node);

        QPDFObjectHandle fields;
        ensureAcroForm(qpdf, fields);
        fields.appendItem(node);

        QPDFWriter writer(qpdf, target.toLocal8Bit().constData());
        writer.write();
    } catch (const std::exception& e) {
        if (error)
            *error = QString::fromUtf8(e.what());
        if (inPlace)
            QFile::remove(target);
        return false;
    }

    if (inPlace) {
        QFile::remove(outputPath);
        if (!QFile::rename(target, outputPath)) {
            if (error)
                *error = QStringLiteral("The file with the new field couldn't replace the "
                                        "original.");
            QFile::remove(target);
            return false;
        }
    }
    return true;
}
