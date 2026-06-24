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

#include <QColor>
#include <QList>
#include <QRectF>
#include <QString>

// The annotation-authoring backend (Poppler-Qt6). Adds real PDF annotations to
// the document and saves them so any viewer (including our QtPdf one, on reopen)
// shows them. Coordinates are normalized to the page (top-left origin, with any
// display rotation already undone by the caller).
class Annotator {
public:
    struct Highlight {
        int page = 0;    // original 0-based page index
        QRectF rect;     // normalized [0,1] page rect
        QColor color;    // the highlight colour
    };

    struct Note {
        int page = 0;       // original 0-based page index
        QPointF pos;        // normalized [0,1] top-left anchor of the note icon
        QString text;       // the note's contents
        QColor color;       // the note icon colour
    };

    // Add `highlights` and `notes` to `inputPath` and write the result to
    // `outputPath`. Temp file + atomic rename, so `outputPath` may equal
    // `inputPath`. Returns true on success; on failure fills *error.
    static bool saveAnnotations(const QString& inputPath, const QString& outputPath,
                                const QList<Highlight>& highlights, const QList<Note>& notes,
                                QString* error);
};
