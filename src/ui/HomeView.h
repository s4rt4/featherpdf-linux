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

class QLabel;
class QVBoxLayout;

// The Home start screen (ui-guidelines §6, §9): the landing place when no
// document is focused. A welcome with the feather mark, one big suggested
// action (Open), recent files as cards, and a drag-and-drop zone — an
// invitation, not a blank. Reads recents from the same QSettings key the rest
// of the app uses; call refresh() after that list changes.
class HomeView : public QWidget {
    Q_OBJECT

public:
    explicit HomeView(QWidget* parent = nullptr);

    void refresh(); // rebuild the recent-files cards

signals:
    void openRequested();                       // the big Open button
    void openPathRequested(const QString& path); // a recent card or a drop

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void rebuildIcons();

    QLabel* m_logo = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_tagline = nullptr;
    QLabel* m_recentHead = nullptr;
    QWidget* m_cards = nullptr;     // container holding RecentCard rows
    QVBoxLayout* m_cardsLayout = nullptr;
    bool m_dragActive = false;
};
