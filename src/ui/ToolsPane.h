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

#include <QVector>
#include <QWidget>

class ToolRow;

// The right Tools pane (ui-guidelines §8 / mockup .tools): a vertical list of
// tool rows, each an accent-tinted icon chip + label, some expandable (chevron).
// Default tools: Export · Create · Edit · Comment · Combine · Organize · Redact ·
// Sign. No upsell — the footer is "Customize tools". Each tool's own panel
// replaces this list in a later increment; for now selecting a row announces it.
class ToolsPane : public QWidget {
    Q_OBJECT

public:
    explicit ToolsPane(QWidget* parent = nullptr);

signals:
    void toolActivated(const QString& id);
    void customizeRequested();

private:
    void refreshIcons();

    QVector<ToolRow*> m_rows;
};
