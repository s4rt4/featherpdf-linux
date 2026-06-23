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
#include <QVector>

class FeatherDocument;
class Viewport;
class HomeView;
class ThumbnailPanel;
class OutlinePanel;
class TabStrip;
class CommandBar;
class FindBar;
class NavigationRail;
class ToolsPane;
class FloatingPill;
class Toast;
class QAction;
class QLabel;
class QMenu;
class QStackedWidget;

// The application shell (ui-guidelines §5): a tabbed Acrobat-style workspace —
// menu bar, tab strip, command toolbar, then the body (left navigation rail ·
// center viewport with its floating pill · right Tools pane). M0 is a single
// document; the multi-document tab workspace builds on this same shell.
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
    void buildShell();
    void wireSignals();

    void nextPage();
    void previousPage();

    // Find: open the find bar, drive matches, and report the running count.
    void openFind();
    void stepFind(int delta);
    void updateFindCount();

    // One open document = one tab. Each session owns its FeatherDocument and
    // remembers the page it was last on so switching tabs restores the spot.
    struct Session {
        int id = -1;
        FeatherDocument* doc = nullptr;
        QString path;
        int lastPage = 0;
    };
    Session* session(int id);
    void activateSession(int id);
    void closeSession(int id);
    bool hasActiveDoc() const;

    // Switch the center between the Home start screen and the document workspace,
    // toggling the document-only chrome (rail · command bar · Tools pane · pill).
    void showHome();
    void showDocument();

    void updateWindowTitle();
    void updateChromeState();
    void updatePageIndicator();
    void updateZoomIndicator();

    void addRecentFile(const QString& path);
    void rebuildRecentMenu();
    QStringList recentFiles() const;

    // Honest placeholder for features that arrive in later milestones.
    void notImplemented(const QString& feature);

    QVector<Session> m_sessions;
    int m_activeId = -1;
    FeatherDocument* m_doc = nullptr; // the active session's document, or nullptr

    Viewport* m_viewport = nullptr;
    HomeView* m_home = nullptr;
    QStackedWidget* m_centerStack = nullptr;
    TabStrip* m_tabStrip = nullptr;

    // Left rail expandable panel (one at a time).
    QWidget* m_panelHost = nullptr;
    QStackedWidget* m_panelStack = nullptr;
    QLabel* m_panelHead = nullptr;
    QLabel* m_panelPlaceholder = nullptr;
    ThumbnailPanel* m_thumbnails = nullptr;
    OutlinePanel* m_outline = nullptr;
    CommandBar* m_commandBar = nullptr;
    FindBar* m_findBar = nullptr;
    int m_searchIndex = 0;
    NavigationRail* m_rail = nullptr;
    ToolsPane* m_toolsPane = nullptr;
    FloatingPill* m_pill = nullptr;
    Toast* m_toast = nullptr;

    // Actions (centralized; menus and shortcuts use these).
    QAction* m_openAct = nullptr;
    QAction* m_closeAct = nullptr;
    QAction* m_quitAct = nullptr;
    QAction* m_zoomInAct = nullptr;
    QAction* m_zoomOutAct = nullptr;
    QAction* m_zoomActualAct = nullptr;
    QAction* m_fitWidthAct = nullptr;
    QAction* m_fitPageAct = nullptr;
    QAction* m_toggleThemeAct = nullptr;
    QAction* m_aboutAct = nullptr;

    QMenu* m_recentMenu = nullptr;

    static constexpr int kMaxRecentFiles = 10;
};
