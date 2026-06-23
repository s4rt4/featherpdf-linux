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

#include <QWidget>

class QPdfDocument;
class QPdfBookmarkModel;
class QTreeView;
class QStackedWidget;
class QLabel;

// The Outline rail panel (ui-guidelines §5.5): the document's bookmark tree
// (QPdfBookmarkModel). Clicking an entry navigates the viewport to its page.
// Documents without bookmarks show a quiet empty state rather than a blank.
class OutlinePanel : public QWidget {
    Q_OBJECT

public:
    explicit OutlinePanel(QWidget* parent = nullptr);

    void setDocument(QPdfDocument* doc);
    void clear();

signals:
    void pageActivated(int page); // 0-based

private:
    void updateEmptyState();

    QPdfBookmarkModel* m_model = nullptr;
    QTreeView* m_tree = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLabel* m_empty = nullptr;
};
