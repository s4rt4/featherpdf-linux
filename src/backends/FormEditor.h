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

#include <QRectF>
#include <QString>
#include <QStringList>

// The form-authoring backend (QPDF). Creates AcroForm fields — the complement to
// FormFiller, which fills existing ones. A field is written as a combined
// field/widget annotation on a page and registered in the catalog's /AcroForm;
// /NeedAppearances is set so any viewer regenerates the field's appearance.
class FormEditor {
public:
    enum class Type { Text, CheckBox, Dropdown, PushButton };

    struct NewField {
        Type type = Type::Text;
        QString name;            // field name (/T); should be unique in the document
        QString defaultValue;    // Text: initial value; PushButton: caption
        bool checked = false;    // CheckBox: initial state
        QStringList options;     // Dropdown: choices
        int page = 0;            // 0-based page to place it on
        QRectF rect;             // normalized [0,1], top-left origin (displayed page)
    };

    // Add `field` to `inputPath` and write `outputPath`. Lossless apart from the
    // new field. Temp file + atomic rename, so `outputPath` may equal `inputPath`.
    // Returns true on success; on failure fills *error with a friendly message.
    static bool addField(const QString& inputPath, const QString& outputPath,
                         const NewField& field, QString* error);
};
