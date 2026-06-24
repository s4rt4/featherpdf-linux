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

#include "backends/Watermarker.h"

#include <QFile>

#include <cmath>
#include <exception>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFWriter.hh>

namespace {
std::string num(double v) {
    return QByteArray::number(v, 'f', 2).toStdString();
}

std::string pdfStr(const QString& text) {
    const QByteArray l = text.toLatin1();
    std::string out = "(";
    for (char c : l) {
        if (c == '(' || c == ')' || c == '\\')
            out += '\\';
        out += c;
    }
    out += ')';
    return out;
}

// Ensure the page carries a Helvetica font (/FtStamp) and, optionally, an
// ExtGState (/GsStamp) at `opacity`, then append `content` to the page.
void stampPage(QPDF& qpdf, QPDFPageObjectHelper& ph, const std::string& content, bool withGs,
               double opacity) {
    QPDFObjectHandle page = ph.getObjectHandle();
    QPDFObjectHandle res = page.getKey("/Resources");
    if (!res.isDictionary()) {
        res = QPDFObjectHandle::newDictionary();
        page.replaceKey("/Resources", res);
    }
    QPDFObjectHandle fonts = res.getKey("/Font");
    if (!fonts.isDictionary()) {
        fonts = QPDFObjectHandle::newDictionary();
        res.replaceKey("/Font", fonts);
    }
    if (!fonts.hasKey("/FtStamp")) {
        QPDFObjectHandle font = QPDFObjectHandle::newDictionary();
        font.replaceKey("/Type", QPDFObjectHandle::newName("/Font"));
        font.replaceKey("/Subtype", QPDFObjectHandle::newName("/Type1"));
        font.replaceKey("/BaseFont", QPDFObjectHandle::newName("/Helvetica-Bold"));
        font.replaceKey("/Encoding", QPDFObjectHandle::newName("/WinAnsiEncoding"));
        fonts.replaceKey("/FtStamp", qpdf.makeIndirectObject(font));
    }
    if (withGs) {
        QPDFObjectHandle egs = res.getKey("/ExtGState");
        if (!egs.isDictionary()) {
            egs = QPDFObjectHandle::newDictionary();
            res.replaceKey("/ExtGState", egs);
        }
        QPDFObjectHandle gs = QPDFObjectHandle::newDictionary();
        gs.replaceKey("/ca", QPDFObjectHandle::newReal(num(opacity)));
        gs.replaceKey("/CA", QPDFObjectHandle::newReal(num(opacity)));
        egs.replaceKey("/GsStamp", gs);
    }
    // Wrap the original content in q/Q so its (possibly unbalanced) graphics state
    // can't leak into the overlay — then the overlay draws in page coordinates.
    ph.addPageContents(QPDFObjectHandle::newStream(&qpdf, "q\n"), /*first=*/true);
    ph.addPageContents(QPDFObjectHandle::newStream(&qpdf, "Q\n" + content), /*first=*/false);
}

bool pageBox(QPDFObjectHandle page, double& w, double& h, double& llx, double& lly) {
    QPDFObjectHandle mb = page.getKey("/MediaBox");
    if (!mb.isArray() || mb.getArrayNItems() < 4)
        return false;
    llx = mb.getArrayItem(0).getNumericValue();
    lly = mb.getArrayItem(1).getNumericValue();
    w = mb.getArrayItem(2).getNumericValue() - llx;
    h = mb.getArrayItem(3).getNumericValue() - lly;
    return w > 0 && h > 0;
}

bool writeOut(QPDF& qpdf, const QString& outputPath, QString* error) {
    const QString target = outputPath + QStringLiteral(".feather-tmp");
    try {
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
            *error = QStringLiteral("The stamped file couldn't be written.");
        QFile::remove(target);
        return false;
    }
    return true;
}
} // namespace

bool Watermarker::addWatermark(const QString& inputPath, const QString& outputPath,
                               const WatermarkOptions& opts, QString* error) {
    if (opts.text.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("Enter the watermark text.");
        return false;
    }
    try {
        QPDF qpdf;
        qpdf.processFile(inputPath.toLocal8Bit().constData());
        QPDFPageDocumentHelper dh(qpdf);
        dh.pushInheritedAttributesToPage();

        const double rad = opts.rotationDeg * M_PI / 180.0;
        const double a = std::cos(rad), b = std::sin(rad);
        const double r = opts.color.redF(), g = opts.color.greenF(), bl = opts.color.blueF();
        // Rough Helvetica-Bold text width to centre the label.
        const double halfW = 0.30 * opts.fontSize * opts.text.length();

        for (QPDFPageObjectHelper& ph : dh.getAllPages()) {
            double w, h, llx, lly;
            if (!pageBox(ph.getObjectHandle(), w, h, llx, lly))
                continue;
            const double cx = llx + w / 2.0, cy = lly + h / 2.0;
            std::string content = "q\n/GsStamp gs\n" + num(r) + " " + num(g) + " " + num(bl) +
                                  " rg\nBT\n/FtStamp " + num(opts.fontSize) + " Tf\n" + num(a) +
                                  " " + num(b) + " " + num(-b) + " " + num(a) + " " + num(cx) +
                                  " " + num(cy) + " Tm\n" + num(-halfW) + " " +
                                  num(-opts.fontSize * 0.33) + " Td\n" + pdfStr(opts.text) +
                                  " Tj\nET\nQ\n";
            stampPage(qpdf, ph, content, /*withGs=*/true, opts.opacity);
        }
        return writeOut(qpdf, outputPath, error);
    } catch (const std::exception& e) {
        if (error)
            *error = QString::fromUtf8(e.what());
        return false;
    }
}

bool Watermarker::addBates(const QString& inputPath, const QString& outputPath,
                           const BatesOptions& opts, QString* error) {
    try {
        QPDF qpdf;
        qpdf.processFile(inputPath.toLocal8Bit().constData());
        QPDFPageDocumentHelper dh(qpdf);
        dh.pushInheritedAttributesToPage();

        const double r = opts.color.redF(), g = opts.color.greenF(), b = opts.color.blueF();
        constexpr double margin = 24.0;
        int n = opts.start;
        for (QPDFPageObjectHelper& ph : dh.getAllPages()) {
            double w, h, llx, lly;
            if (!pageBox(ph.getObjectHandle(), w, h, llx, lly)) {
                ++n;
                continue;
            }
            const QString label =
                opts.prefix +
                QStringLiteral("%1").arg(n, opts.digits, 10, QLatin1Char('0')) + opts.suffix;
            const double textW = 0.55 * opts.fontSize * label.length();
            double x = llx + margin, y = lly + margin;
            if (opts.corner == Corner::TopLeft || opts.corner == Corner::TopRight)
                y = lly + h - margin - opts.fontSize;
            if (opts.corner == Corner::TopRight || opts.corner == Corner::BottomRight)
                x = llx + w - margin - textW;

            std::string content = "q\n" + num(r) + " " + num(g) + " " + num(b) + " rg\nBT\n/FtStamp " +
                                  num(opts.fontSize) + " Tf\n1 0 0 1 " + num(x) + " " + num(y) +
                                  " Tm\n" + pdfStr(label) + " Tj\nET\nQ\n";
            stampPage(qpdf, ph, content, /*withGs=*/false, 1.0);
            ++n;
        }
        return writeOut(qpdf, outputPath, error);
    } catch (const std::exception& e) {
        if (error)
            *error = QString::fromUtf8(e.what());
        return false;
    }
}
