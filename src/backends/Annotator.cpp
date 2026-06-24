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

#include "backends/Annotator.h"

#include <QFile>
#include <QFileInfo>

#include <exception>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFWriter.hh>

// We author highlights with QPDF rather than Poppler so we can attach an explicit
// appearance stream (/AP). PDFium (our QtPdf viewer) renders an annotation's /AP
// directly; without one it falls back to its own quad interpretation, which draws
// text-markup highlights incorrectly. Building the /AP ourselves makes the result
// render identically everywhere.

namespace {
std::string num(double v) {
    return QByteArray::number(v, 'f', 2).toStdString();
}

QPDFObjectHandle numberArray(std::initializer_list<double> vals) {
    QPDFObjectHandle a = QPDFObjectHandle::newArray();
    for (double v : vals)
        a.appendItem(QPDFObjectHandle::newReal(num(v)));
    return a;
}
} // namespace

namespace {
// Append an annotation to a page's /Annots, creating the array if absent.
void appendAnnot(QPDFObjectHandle& page, QPDFObjectHandle annot, QPDF& qpdf) {
    QPDFObjectHandle annots = page.getKey("/Annots");
    if (!annots.isArray())
        annots = QPDFObjectHandle::newArray();
    annots.appendItem(qpdf.makeIndirectObject(annot));
    page.replaceKey("/Annots", annots);
}
} // namespace

bool Annotator::saveAnnotations(const QString& inputPath, const QString& outputPath,
                                const QList<Highlight>& highlights, const QList<Note>& notes,
                                QString* error) {
    if (highlights.isEmpty() && notes.isEmpty()) {
        if (error)
            *error = QStringLiteral("Nothing to save.");
        return false;
    }

    const QString target = outputPath + QStringLiteral(".feather-tmp");

    try {
        QPDF qpdf;
        qpdf.processFile(inputPath.toLocal8Bit().constData());

        QPDFPageDocumentHelper dh(qpdf);
        dh.pushInheritedAttributesToPage(); // ensure each page carries its own MediaBox
        std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
        const int n = static_cast<int>(pages.size());

        for (const Highlight& hl : highlights) {
            if (hl.page < 0 || hl.page >= n)
                continue;
            QPDFObjectHandle page = pages[hl.page].getObjectHandle();

            // Page box → absolute PDF coordinates (origin bottom-left).
            QPDFObjectHandle mb = page.getKey("/MediaBox");
            const double llx = mb.getArrayItem(0).getNumericValue();
            const double lly = mb.getArrayItem(1).getNumericValue();
            const double urx = mb.getArrayItem(2).getNumericValue();
            const double ury = mb.getArrayItem(3).getNumericValue();
            const double w = urx - llx;
            const double h = ury - lly;

            const double x0 = llx + hl.rect.left() * w;
            const double x1 = llx + hl.rect.right() * w;
            const double yTop = ury - hl.rect.top() * h;    // normalized top → high y
            const double yBot = ury - hl.rect.bottom() * h; // normalized bottom → low y

            const double r = hl.color.redF();
            const double g = hl.color.greenF();
            const double b = hl.color.blueF();
            constexpr double opacity = 0.4;

            // Appearance: a Multiply-blended filled rectangle (highlighter look).
            QPDFObjectHandle form = QPDFObjectHandle::newStream(&qpdf);
            QPDFObjectHandle fd = form.getDict();
            fd.replaceKey("/Type", QPDFObjectHandle::newName("/XObject"));
            fd.replaceKey("/Subtype", QPDFObjectHandle::newName("/Form"));
            fd.replaceKey("/BBox", numberArray({x0, yBot, x1, yTop}));
            QPDFObjectHandle gs = QPDFObjectHandle::newDictionary();
            gs.replaceKey("/BM", QPDFObjectHandle::newName("/Multiply"));
            QPDFObjectHandle egs = QPDFObjectHandle::newDictionary();
            egs.replaceKey("/GS0", gs);
            QPDFObjectHandle res = QPDFObjectHandle::newDictionary();
            res.replaceKey("/ExtGState", egs);
            fd.replaceKey("/Resources", res);
            const std::string content = "/GS0 gs\n" + num(r) + " " + num(g) + " " + num(b) +
                                        " rg\n" + num(x0) + " " + num(yBot) + " " + num(x1 - x0) +
                                        " " + num(yTop - yBot) + " re f\n";
            form.replaceStreamData(content, QPDFObjectHandle::newNull(),
                                   QPDFObjectHandle::newNull());

            QPDFObjectHandle ap = QPDFObjectHandle::newDictionary();
            ap.replaceKey("/N", form);

            QPDFObjectHandle annot = QPDFObjectHandle::newDictionary();
            annot.replaceKey("/Type", QPDFObjectHandle::newName("/Annot"));
            annot.replaceKey("/Subtype", QPDFObjectHandle::newName("/Highlight"));
            annot.replaceKey("/Rect", numberArray({x0, yBot, x1, yTop}));
            // QuadPoints (text-markup convention: upper-left, upper-right,
            // lower-left, lower-right).
            annot.replaceKey("/QuadPoints",
                             numberArray({x0, yTop, x1, yTop, x0, yBot, x1, yBot}));
            annot.replaceKey("/C", numberArray({r, g, b}));
            annot.replaceKey("/CA", QPDFObjectHandle::newReal(num(opacity)));
            annot.replaceKey("/F", QPDFObjectHandle::newInteger(4)); // Print
            annot.replaceKey("/AP", ap);

            appendAnnot(page, annot, qpdf);
        }

        for (const Note& note : notes) {
            if (note.page < 0 || note.page >= n)
                continue;
            QPDFObjectHandle page = pages[note.page].getObjectHandle();
            QPDFObjectHandle mb = page.getKey("/MediaBox");
            const double llx = mb.getArrayItem(0).getNumericValue();
            const double lly = mb.getArrayItem(1).getNumericValue();
            const double urx = mb.getArrayItem(2).getNumericValue();
            const double ury = mb.getArrayItem(3).getNumericValue();
            const double w = urx - llx;
            const double h = ury - lly;

            constexpr double s = 20.0; // icon size in points
            const double x = llx + note.pos.x() * w;
            const double yTop = ury - note.pos.y() * h;
            const double y = yTop - s; // icon grows downward from the anchor

            const double r = note.color.redF();
            const double g = note.color.greenF();
            const double b = note.color.blueF();

            // Appearance: a little note card with three "text" lines.
            QPDFObjectHandle form = QPDFObjectHandle::newStream(&qpdf);
            QPDFObjectHandle fd = form.getDict();
            fd.replaceKey("/Type", QPDFObjectHandle::newName("/XObject"));
            fd.replaceKey("/Subtype", QPDFObjectHandle::newName("/Form"));
            fd.replaceKey("/BBox", numberArray({x, y, x + s, y + s}));
            const std::string content =
                num(r) + " " + num(g) + " " + num(b) + " rg " + num(x) + " " + num(y) + " " +
                num(s) + " " + num(s) + " re f " + "0.25 0.25 0.25 RG 1.4 w " + num(x + 4) + " " +
                num(y + 14) + " m " + num(x + 16) + " " + num(y + 14) + " l S " + num(x + 4) + " " +
                num(y + 10) + " m " + num(x + 16) + " " + num(y + 10) + " l S " + num(x + 4) + " " +
                num(y + 6) + " m " + num(x + 12) + " " + num(y + 6) + " l S";
            form.replaceStreamData(content, QPDFObjectHandle::newNull(),
                                   QPDFObjectHandle::newNull());
            QPDFObjectHandle ap = QPDFObjectHandle::newDictionary();
            ap.replaceKey("/N", form);

            QPDFObjectHandle annot = QPDFObjectHandle::newDictionary();
            annot.replaceKey("/Type", QPDFObjectHandle::newName("/Annot"));
            annot.replaceKey("/Subtype", QPDFObjectHandle::newName("/Text"));
            annot.replaceKey("/Rect", numberArray({x, y, x + s, y + s}));
            annot.replaceKey("/Name", QPDFObjectHandle::newName("/Comment"));
            annot.replaceKey("/Contents", QPDFObjectHandle::newUnicodeString(note.text.toStdString()));
            annot.replaceKey("/C", numberArray({r, g, b}));
            annot.replaceKey("/F", QPDFObjectHandle::newInteger(4));
            annot.replaceKey("/AP", ap);

            appendAnnot(page, annot, qpdf);
        }

        QPDFWriter writer(qpdf, target.toLocal8Bit().constData());
        writer.write();
    } catch (const std::exception& e) {
        if (error)
            *error = QString::fromUtf8(e.what());
        QFile::remove(target);
        return false;
    }

    QFile::remove(outputPath);
    if (!QFile::rename(target, outputPath)) {
        if (error)
            *error = QStringLiteral("The annotated file couldn't replace the original.");
        QFile::remove(target);
        return false;
    }
    return true;
}
