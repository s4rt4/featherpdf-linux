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

#include <QHash>
#include <QMainWindow>
#include <QPair>
#include <QStringList>
#include <QVector>

class FeatherDocument;
class Viewport;
class HomeView;
class ThumbnailPanel;
class OutlinePanel;
class AnnotationsPanel;
class AttachmentsPanel;
class LayersPanel;
class FormPanel;
class TabStrip;
class CommandBar;
class FindBar;
class RedactionBar;
class AnnotationBar;
class NavigationRail;
class ToolsPane;
class FloatingPill;
class Toast;
class QAction;
class QAbstractAnimation;
class QLabel;
class QMenu;
class QPrinter;
class QStackedWidget;
class QUndoGroup;
class QUndoStack;

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
    void closeTab(int id); // prompts to save if the tab is dirty, then closes it

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    // Returns true if it is OK to proceed (saved or discarded); false to cancel.
    bool maybeSaveSession(int id);

    void buildActions();
    void buildMenus();
    void buildShell();
    void wireSignals();

    void nextPage();
    void previousPage();

    // Page edits (M1).
    void rotateActivePage(int deltaDegrees);
    void deleteActivePage();
    bool saveActiveAs();      // export the edited document to a chosen path (QPDF)
    bool saveActive();        // write edits back to the current file (QPDF)
    void setRedactionMode(bool on); // enter/leave the draw-to-redact mode
    void applyRedactions();         // flatten marked pages, save, open the result
    void setHighlightMode(bool on); // enter/leave the annotation-authoring mode
    void applyAnnotations();        // write highlight + note annotations, save, open result
    void combineDocuments();  // merge several PDFs into one (QPDF)
    void protectDocument();   // write a password-encrypted copy (QPDF, AES-256)
    void removeProtection();  // write a decrypted (password-free) copy (QPDF)
    void printActive();       // custom print dialog (async printers + preview)
    void printDocument(QPrinter& printer, const QList<int>& pages, bool grayscale);

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
        QUndoStack* undo = nullptr; // this document's edit history
    };
    Session* session(int id);
    void activateSession(int id);
    void closeSession(int id);
    bool hasActiveDoc() const;

    // Switch the center between the Home start screen and the document workspace,
    // toggling the document-only chrome (rail · command bar · Tools pane · pill).
    void showHome();
    void showDocument();

    // Immersive reading: hide all chrome, leaving the page alone with its pill
    // (ui-guidelines §1 — the signature moment). F11 toggles, Esc exits.
    void setImmersive(bool on);
    void animateChrome(bool collapse);                 // slide the chrome in/out
    QVector<QPair<QWidget*, bool>> chromeItems() const; // {widget, vertical}

    void updateWindowTitle();
    void updateChromeState();
    void updatePageIndicator();
    void updateZoomIndicator();

    void addRecentFile(const QString& path);
    void rebuildRecentMenu();
    QStringList recentFiles() const;

    void showProperties(); // Document ▸ Properties — metadata dialog
    void showAbout();      // Help ▸ About — branded dialog with links

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
    ThumbnailPanel* m_thumbnails = nullptr;
    OutlinePanel* m_outline = nullptr;
    AnnotationsPanel* m_annotations = nullptr;
    AttachmentsPanel* m_attachments = nullptr;
    LayersPanel* m_layers = nullptr;
    FormPanel* m_forms = nullptr;
    CommandBar* m_commandBar = nullptr;
    FindBar* m_findBar = nullptr;
    RedactionBar* m_redactionBar = nullptr;
    AnnotationBar* m_annotationBar = nullptr;
    int m_searchIndex = 0;
    NavigationRail* m_rail = nullptr;
    ToolsPane* m_toolsPane = nullptr;
    FloatingPill* m_pill = nullptr;
    Toast* m_toast = nullptr;

    // Actions (centralized; menus and shortcuts use these).
    QAction* m_openAct = nullptr;
    QAction* m_saveAct = nullptr;
    QAction* m_saveAsAct = nullptr;
    QAction* m_protectAct = nullptr;
    QAction* m_removeProtectionAct = nullptr;
    QAction* m_printAct = nullptr;
    QAction* m_closeAct = nullptr;
    QAction* m_quitAct = nullptr;
    QAction* m_undoAct = nullptr;
    QAction* m_redoAct = nullptr;
    QAction* m_rotateLeftAct = nullptr;
    QAction* m_rotateRightAct = nullptr;
    QAction* m_deletePageAct = nullptr;
    QUndoGroup* m_undoGroup = nullptr;
    QAction* m_zoomInAct = nullptr;
    QAction* m_zoomOutAct = nullptr;
    QAction* m_zoomActualAct = nullptr;
    QAction* m_fitWidthAct = nullptr;
    QAction* m_fitPageAct = nullptr;
    QAction* m_singlePageAct = nullptr;
    QAction* m_continuousAct = nullptr;
    QAction* m_twoUpAct = nullptr;
    QAction* m_toggleThemeAct = nullptr;
    QAction* m_immersiveAct = nullptr;
    QAction* m_aboutAct = nullptr;

    bool m_immersive = false;
    bool m_panelWasVisible = false;
    QAbstractAnimation* m_chromeAnim = nullptr;
    QHash<QWidget*, int> m_chromeExtent;

    QMenu* m_recentMenu = nullptr;

    static constexpr int kMaxRecentFiles = 10;
};
