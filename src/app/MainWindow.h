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

#include <QMainWindow>
#include <QStringList>

class FeatherDocument;
class Viewport;
class QAction;
class QLabel;
class QMenu;

// The application shell. M0 is intentionally a single-document window; the
// Acrobat-style tab strip (multiple documents) arrives in a later increment.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Opens a PDF and shows it. Reports failures to the user. Returns success.
    bool openPath(const QString& path);

public slots:
    void openFileDialog();
    void closeDocument();

private:
    void buildActions();
    void buildMenus();
    void buildToolBar();
    void buildStatusBar();

    void updateWindowTitle();
    void updateActionsEnabled();
    void updatePageIndicator();
    void updateZoomIndicator();

    void addRecentFile(const QString& path);
    void rebuildRecentMenu();
    QStringList recentFiles() const;

    FeatherDocument* m_doc;
    Viewport* m_viewport;

    // Actions
    QAction* m_openAct = nullptr;
    QAction* m_closeAct = nullptr;
    QAction* m_quitAct = nullptr;
    QAction* m_zoomInAct = nullptr;
    QAction* m_zoomOutAct = nullptr;
    QAction* m_zoomActualAct = nullptr;
    QAction* m_fitWidthAct = nullptr;
    QAction* m_fitPageAct = nullptr;
    QAction* m_aboutAct = nullptr;

    QMenu* m_recentMenu = nullptr;

    // Status bar widgets
    QLabel* m_pageLabel = nullptr;
    QLabel* m_zoomLabel = nullptr;

    static constexpr int kMaxRecentFiles = 10;
};
