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

#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// The lossless structure backend (QPDF). It copies the original page objects
// rather than re-rendering, so saved files keep their fidelity. M1 starts with
// applying per-page rotation; deletes and reordering join the same writer.
class PdfEditor {
public:
    // Write `inputPath` to `outputPath` as the arrangement described by `order`
    // (display slot → original 0-based page index) and `rotations` (per slot,
    // degrees clockwise relative to the page's original orientation). Pages not
    // present in `order` are dropped; repeats/reordering are honoured. Writing
    // over the input is handled safely (temp file + atomic rename). Returns true
    // on success; on failure fills *error with a friendly message.
    static bool saveArrangement(const QString& inputPath, const QString& outputPath,
                                const QVector<int>& order, const QVector<int>& rotations,
                                QString* error);

    // Merge `inputPaths` into a single PDF at `outputPath`, concatenating all
    // pages of each input in list order. Pages are copied losslessly (foreign
    // object copy, no re-render). The write goes through a temp file then an
    // atomic rename, so `outputPath` may safely equal one of the inputs. Returns
    // true on success; on failure fills *error with a friendly message.
    static bool combine(const QStringList& inputPaths, const QString& outputPath, QString* error);

    // Write `inputPath` to `outputPath` encrypted with `password` (AES-256, the
    // PDF 2.0 R6 scheme), required to open the document. Lossless — the page
    // content is copied, only an encryption layer is added. Temp file + atomic
    // rename, so `outputPath` may equal `inputPath`. Returns true on success;
    // on failure fills *error with a friendly message.
    static bool protect(const QString& inputPath, const QString& outputPath,
                        const QString& password, QString* error);

    // True if `path` is an encrypted PDF that needs a user password to open.
    // (QtPdf reports AES-256 files as an "unsupported scheme" until the right
    // password is set, so the viewer asks QPDF whether to prompt for one.)
    static bool isPasswordProtected(const QString& path);

    // Write `inputPath` (opened with `password`) to `outputPath` with its
    // encryption removed. Lossless. Temp file + atomic rename, so `outputPath`
    // may equal `inputPath`. Returns true on success; fills *error otherwise.
    static bool removeProtection(const QString& inputPath, const QString& outputPath,
                                 const QString& password, QString* error);
};
