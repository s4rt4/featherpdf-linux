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
#include <QImage>

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
    // can't leak into the overlay - then the overlay draws in page coordinates.
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

bool Watermarker::addImageWatermark(const QString& inputPath, const QString& outputPath,
                                    const ImageWatermarkOptions& opts, QString* error) {
    QImage src(opts.imagePath);
    if (src.isNull()) {
        if (error)
            *error = QStringLiteral("That image couldn't be read.");
        return false;
    }
    const QImage img = src.convertToFormat(QImage::Format_ARGB32);
    const int w = img.width(), h = img.height();
    // Tightly packed RGB and (if present) alpha buffers.
    std::string rgb;
    rgb.reserve(size_t(w) * h * 3);
    std::string alpha;
    bool hasAlpha = false;
    for (int y = 0; y < h; ++y) {
        const QRgb* row = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            const QRgb px = row[x];
            rgb.push_back(char(qRed(px)));
            rgb.push_back(char(qGreen(px)));
            rgb.push_back(char(qBlue(px)));
            const int a = qAlpha(px);
            alpha.push_back(char(a));
            if (a != 255)
                hasAlpha = true;
        }
    }

    try {
        QPDF qpdf;
        qpdf.processFile(inputPath.toLocal8Bit().constData());
        QPDFPageDocumentHelper dh(qpdf);
        dh.pushInheritedAttributesToPage();

        // One shared image XObject (raw RGB; QPDF compresses it on write). An
        // SMask carries transparency when the image has an alpha channel.
        QPDFObjectHandle image = QPDFObjectHandle::newStream(&qpdf);
        QPDFObjectHandle d = image.getDict();
        d.replaceKey("/Type", QPDFObjectHandle::newName("/XObject"));
        d.replaceKey("/Subtype", QPDFObjectHandle::newName("/Image"));
        d.replaceKey("/Width", QPDFObjectHandle::newInteger(w));
        d.replaceKey("/Height", QPDFObjectHandle::newInteger(h));
        d.replaceKey("/ColorSpace", QPDFObjectHandle::newName("/DeviceRGB"));
        d.replaceKey("/BitsPerComponent", QPDFObjectHandle::newInteger(8));
        image.replaceStreamData(rgb, QPDFObjectHandle::newNull(), QPDFObjectHandle::newNull());
        if (hasAlpha) {
            QPDFObjectHandle smask = QPDFObjectHandle::newStream(&qpdf);
            QPDFObjectHandle sd = smask.getDict();
            sd.replaceKey("/Type", QPDFObjectHandle::newName("/XObject"));
            sd.replaceKey("/Subtype", QPDFObjectHandle::newName("/Image"));
            sd.replaceKey("/Width", QPDFObjectHandle::newInteger(w));
            sd.replaceKey("/Height", QPDFObjectHandle::newInteger(h));
            sd.replaceKey("/ColorSpace", QPDFObjectHandle::newName("/DeviceGray"));
            sd.replaceKey("/BitsPerComponent", QPDFObjectHandle::newInteger(8));
            smask.replaceStreamData(alpha, QPDFObjectHandle::newNull(),
                                    QPDFObjectHandle::newNull());
            d.replaceKey("/SMask", smask);
        }

        const double rad = opts.rotationDeg * M_PI / 180.0;
        const double ca = std::cos(rad), sa = std::sin(rad);
        const double aspect = double(h) / double(w);

        for (QPDFPageObjectHelper& ph : dh.getAllPages()) {
            double pw, phh, llx, lly;
            if (!pageBox(ph.getObjectHandle(), pw, phh, llx, lly))
                continue;
            // Add the image to the page's XObject resources as /WmImg.
            QPDFObjectHandle res = ph.getObjectHandle().getKey("/Resources");
            if (!res.isDictionary()) {
                res = QPDFObjectHandle::newDictionary();
                ph.getObjectHandle().replaceKey("/Resources", res);
            }
            QPDFObjectHandle xobjs = res.getKey("/XObject");
            if (!xobjs.isDictionary()) {
                xobjs = QPDFObjectHandle::newDictionary();
                res.replaceKey("/XObject", xobjs);
            }
            xobjs.replaceKey("/WmImg", image);
            QPDFObjectHandle egs = res.getKey("/ExtGState");
            if (!egs.isDictionary()) {
                egs = QPDFObjectHandle::newDictionary();
                res.replaceKey("/ExtGState", egs);
            }
            QPDFObjectHandle gs = QPDFObjectHandle::newDictionary();
            gs.replaceKey("/ca", QPDFObjectHandle::newReal(num(opts.opacity)));
            gs.replaceKey("/CA", QPDFObjectHandle::newReal(num(opts.opacity)));
            egs.replaceKey("/GsStamp", gs);

            const double drawW = pw * opts.scale;
            const double drawH = drawW * aspect;
            const double cx = llx + pw / 2.0, cy = lly + phh / 2.0;
            // cm = rotation × scale, then translate to the centre, drawing the
            // unit image from its own centre (-0.5..0.5).
            const double m0 = drawW * ca, m1 = drawW * sa, m2 = -drawH * sa, m3 = drawH * ca;
            const double e = cx - (m0 * 0.5 + m2 * 0.5), f = cy - (m1 * 0.5 + m3 * 0.5);
            std::string content = "q\n/GsStamp gs\n" + num(m0) + " " + num(m1) + " " + num(m2) +
                                  " " + num(m3) + " " + num(e) + " " + num(f) + " cm\n/WmImg Do\nQ\n";
            // q/Q wrap so the original page state can't displace the overlay.
            ph.addPageContents(QPDFObjectHandle::newStream(&qpdf, "q\n"), /*first=*/true);
            ph.addPageContents(QPDFObjectHandle::newStream(&qpdf, "Q\n" + content), /*first=*/false);
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
