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

class Tab;
class QHBoxLayout;
class QToolButton;

// The tab strip (ui-guidelines §6 / mockup .tabs) — the defining element. Two
// fixed mode tabs (Home · Tools) are pinned left, then one document tab per open
// PDF, then a "+". The active tab fills with `surface`, carries a 2px accent top
// indicator, and connects seamlessly to the command toolbar below. Right of the
// strip: quick search, a light/dark affordance, and the app menu.
//
// M0 is single-document, so there is exactly one document tab; true multi-tab
// document management arrives with the tabbed-workspace increment.
class TabStrip : public QWidget {
    Q_OBJECT

public:
    enum class Active { Home, Tools, Document };

    explicit TabStrip(QWidget* parent = nullptr);

    // Set the document tab's title (file name) and dirty state. An empty title
    // means no document is open — the document tab shows a neutral placeholder
    // and Home stays active.
    void setDocumentTitle(const QString& title);
    void setDocumentDirty(bool dirty);
    void setActive(Active which);
    bool hasDocument() const { return m_hasDocument; }

protected:
    void paintEvent(QPaintEvent* event) override;

signals:
    void homeSelected();
    void toolsSelected();
    void documentSelected();
    void newTabRequested();
    void closeDocumentRequested();
    void searchRequested();
    void menuRequested();

private:
    QToolButton* makeRightButton(const QString& iconName, const QString& tip);
    void refreshIcons();

    QHBoxLayout* m_layout = nullptr;
    Tab* m_home = nullptr;
    Tab* m_tools = nullptr;
    Tab* m_doc = nullptr;
    Tab* m_add = nullptr;
    QToolButton* m_search = nullptr;
    QToolButton* m_theme = nullptr;
    QToolButton* m_menu = nullptr;
    bool m_hasDocument = false;
};
