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

#include "backends/Converter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

namespace {
const QStringList kImageExts{"png", "jpg", "jpeg", "bmp", "gif", "tif", "tiff", "webp"};
}

bool Converter::isImage(const QString& path) {
    return kImageExts.contains(QFileInfo(path).suffix().toLower());
}

bool Converter::hasOfficeConverter() {
    return !QStandardPaths::findExecutable(QStringLiteral("soffice")).isEmpty() ||
           !QStandardPaths::findExecutable(QStringLiteral("libreoffice")).isEmpty();
}

bool Converter::imagesToPdf(const QStringList& images, const QString& outputPath, QString* error) {
    if (images.isEmpty()) {
        if (error)
            *error = QStringLiteral("No images to convert.");
        return false;
    }

    QPdfWriter writer(outputPath);
    writer.setResolution(150); // painter dots-per-inch
    writer.setPageMargins(QMarginsF(0, 0, 0, 0));
    QPainter painter;
    bool started = false;

    for (const QString& path : images) {
        QImage img(path);
        if (img.isNull())
            continue;
        // Page sized to the image at the writer resolution.
        const QSizeF pagePt(img.width() / 150.0 * 72.0, img.height() / 150.0 * 72.0);
        writer.setPageSize(QPageSize(pagePt, QPageSize::Point));
        if (!started) {
            if (!painter.begin(&writer)) {
                if (error)
                    *error = QStringLiteral("Couldn't create the PDF.");
                return false;
            }
            started = true;
        } else {
            writer.newPage();
        }
        painter.drawImage(QRect(0, 0, writer.width(), writer.height()), img);
    }
    painter.end();

    if (!started) {
        if (error)
            *error = QStringLiteral("None of the images could be read.");
        QFile::remove(outputPath);
        return false;
    }
    return true;
}

bool Converter::officeToPdf(const QString& inputPath, const QString& outputPath, QString* error) {
    QString soffice = QStandardPaths::findExecutable(QStringLiteral("soffice"));
    if (soffice.isEmpty())
        soffice = QStandardPaths::findExecutable(QStringLiteral("libreoffice"));
    if (soffice.isEmpty()) {
        if (error)
            *error = QStringLiteral("LibreOffice isn't installed, so this document can't be "
                                    "converted.");
        return false;
    }

    // Convert into an isolated temp dir with a private profile (so it works even
    // if a LibreOffice instance is already running).
    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        if (error)
            *error = QStringLiteral("Couldn't create a temporary working directory.");
        return false;
    }
    const QString outDir = tmp.path();
    const QString profile = QStringLiteral("file://") + outDir + QStringLiteral("/profile");

    QProcess proc;
    proc.start(soffice, {QStringLiteral("--headless"), QStringLiteral("--convert-to"),
                         QStringLiteral("pdf"), QStringLiteral("--outdir"), outDir,
                         QStringLiteral("-env:UserInstallation=") + profile, inputPath});
    if (!proc.waitForStarted(15000)) {
        if (error)
            *error = QStringLiteral("LibreOffice couldn't be started.");
        return false;
    }
    if (!proc.waitForFinished(120000) || proc.exitStatus() != QProcess::NormalExit ||
        proc.exitCode() != 0) {
        if (error)
            *error = QStringLiteral("The document couldn't be converted.");
        return false;
    }

    const QString produced =
        QDir(outDir).filePath(QFileInfo(inputPath).completeBaseName() + QStringLiteral(".pdf"));
    if (!QFile::exists(produced)) {
        if (error)
            *error = QStringLiteral("LibreOffice produced no PDF.");
        return false;
    }
    QFile::remove(outputPath);
    if (!QFile::copy(produced, outputPath)) {
        if (error)
            *error = QStringLiteral("The converted PDF couldn't be saved.");
        return false;
    }
    return true;
}
