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

#include "backends/PdfEditor.h"

#include <QFile>
#include <QFileInfo>

#include <exception>
#include <vector>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>
#include <qpdf/QPDFWriter.hh>

bool PdfEditor::saveArrangement(const QString& inputPath, const QString& outputPath,
                                const QVector<int>& order, const QVector<int>& rotations,
                                QString* error) {
    const bool inPlace =
        QFileInfo(inputPath).absoluteFilePath() == QFileInfo(outputPath).absoluteFilePath();
    // Never write over the file we are reading — go through a sibling temp file.
    const QString target = inPlace ? outputPath + QStringLiteral(".feather-tmp") : outputPath;

    try {
        QPDF qpdf;
        qpdf.processFile(inputPath.toLocal8Bit().constData());

        QPDFPageDocumentHelper dh(qpdf);
        // Make each page self-contained (own MediaBox/Resources) before we detach
        // them from the page tree, so re-adding in a new order keeps fidelity.
        dh.pushInheritedAttributesToPage();
        std::vector<QPDFPageObjectHelper> original = dh.getAllPages();

        // Detach every page, then re-add only those in `order`, in that order,
        // each rotated as requested. Dropped pages simply never come back.
        for (auto& page : original)
            dh.removePage(page);

        const int n = static_cast<int>(original.size());
        for (int slot = 0; slot < order.size(); ++slot) {
            const int orig = order[slot];
            if (orig < 0 || orig >= n)
                continue;
            QPDFPageObjectHelper page = original[orig];
            int rot = (slot < rotations.size()) ? rotations[slot] : 0;
            rot = ((rot % 360) + 360) % 360;
            if (rot != 0)
                page.rotatePage(rot, /*relative=*/true);
            dh.addPage(page, /*first=*/false);
        }

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
        QFile::remove(outputPath); // rename won't clobber an existing file
        if (!QFile::rename(target, outputPath)) {
            if (error)
                *error = QStringLiteral("The edited file couldn't replace the original.");
            QFile::remove(target);
            return false;
        }
    }
    return true;
}
