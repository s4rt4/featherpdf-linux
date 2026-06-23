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

#include "core/FeatherDocument.h"

#include <QCoreApplication>
#include <QUndoCommand>

// Page operations as undoable commands (ui-guidelines §10: one unified undo
// stack per document). Each command mutates the façade's edit model; the view
// follows via FeatherDocument::pageEdited.

class RotatePageCommand : public QUndoCommand {
    Q_DECLARE_TR_FUNCTIONS(RotatePageCommand)

public:
    RotatePageCommand(FeatherDocument* doc, int page, int deltaDegrees)
        : m_doc(doc), m_page(page), m_delta(((deltaDegrees % 360) + 360) % 360) {
        setText(tr("Rotate page %1").arg(page + 1));
    }

    void redo() override { m_doc->rotatePage(m_page, m_delta); }
    void undo() override { m_doc->rotatePage(m_page, -m_delta); }

private:
    FeatherDocument* m_doc;
    int m_page;
    int m_delta;
};
