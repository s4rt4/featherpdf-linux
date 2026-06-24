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

#include "app/MainWindow.h"

#include "backends/PdfEditor.h"
#include "core/FeatherDocument.h"
#include "core/PageCommands.h"
#include "ui/AnnotationsPanel.h"
#include "ui/AttachmentsPanel.h"
#include "ui/CombineDialog.h"
#include "ui/CommandBar.h"
#include "ui/FindBar.h"
#include "ui/FloatingPill.h"
#include "ui/HomeView.h"
#include "ui/LayersPanel.h"
#include "ui/NavigationRail.h"
#include "ui/OutlinePanel.h"
#include "ui/PrintDialog.h"
#include "ui/ProtectDialog.h"
#include "ui/TabStrip.h"
#include "ui/Theme.h"
#include "ui/ThumbnailPanel.h"
#include "ui/Toast.h"
#include "ui/ToolsPane.h"
#include "ui/Viewport.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QInputDialog>
#include <QLocale>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QPrinter>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QUndoGroup>
#include <QUndoStack>
#include <QVBoxLayout>

namespace {
constexpr auto kRecentFilesKey = "recentFiles";
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildActions();
    buildMenus();
    buildShell();
    wireSignals();

    resize(1180, 820);
    updateWindowTitle();
    updateChromeState();
    rebuildRecentMenu();
    showHome(); // land on the start screen until a document is opened
}

MainWindow::~MainWindow() = default;

void MainWindow::buildActions() {
    m_openAct = new QAction(tr("&Open…"), this);
    m_openAct->setShortcut(QKeySequence::Open);
    connect(m_openAct, &QAction::triggered, this, &MainWindow::openFileDialog);

    m_saveAct = new QAction(tr("&Save"), this);
    m_saveAct->setShortcut(QKeySequence::Save);
    connect(m_saveAct, &QAction::triggered, this, [this] { saveActive(); });

    m_saveAsAct = new QAction(tr("Save &As…"), this);
    m_saveAsAct->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAsAct, &QAction::triggered, this, [this] { saveActiveAs(); });

    m_protectAct = new QAction(tr("Pro&tect with Password…"), this);
    connect(m_protectAct, &QAction::triggered, this, &MainWindow::protectDocument);

    m_printAct = new QAction(tr("&Print…"), this);
    m_printAct->setShortcut(QKeySequence::Print);
    connect(m_printAct, &QAction::triggered, this, &MainWindow::printActive);

    // One undo history per document, tracked by a group so Undo/Redo always act
    // on the active tab (ui-guidelines §10).
    m_undoGroup = new QUndoGroup(this);
    m_undoAct = m_undoGroup->createUndoAction(this, tr("&Undo"));
    m_undoAct->setShortcut(QKeySequence::Undo);
    m_redoAct = m_undoGroup->createRedoAction(this, tr("&Redo"));
    m_redoAct->setShortcut(QKeySequence::Redo);

    m_rotateLeftAct = new QAction(tr("Rotate &Left"), this);
    m_rotateLeftAct->setShortcut(Qt::CTRL | Qt::Key_BracketLeft);
    connect(m_rotateLeftAct, &QAction::triggered, this, [this] { rotateActivePage(-90); });
    m_rotateRightAct = new QAction(tr("Rotate &Right"), this);
    m_rotateRightAct->setShortcut(Qt::CTRL | Qt::Key_BracketRight);
    connect(m_rotateRightAct, &QAction::triggered, this, [this] { rotateActivePage(90); });

    m_deletePageAct = new QAction(tr("&Delete Page"), this);
    connect(m_deletePageAct, &QAction::triggered, this, &MainWindow::deleteActivePage);

    m_closeAct = new QAction(tr("&Close"), this);
    m_closeAct->setShortcut(QKeySequence::Close);
    connect(m_closeAct, &QAction::triggered, this, &MainWindow::closeDocument);

    m_quitAct = new QAction(tr("&Quit"), this);
    m_quitAct->setShortcut(QKeySequence::Quit);
    // Route Quit through close() so it goes past the unsaved-changes guard.
    connect(m_quitAct, &QAction::triggered, this, &MainWindow::close);

    m_zoomInAct = new QAction(tr("Zoom &In"), this);
    m_zoomInAct->setShortcut(QKeySequence::ZoomIn);

    m_zoomOutAct = new QAction(tr("Zoom &Out"), this);
    m_zoomOutAct->setShortcut(QKeySequence::ZoomOut);

    m_zoomActualAct = new QAction(tr("&Actual Size"), this);
    m_zoomActualAct->setShortcut(Qt::CTRL | Qt::Key_0);

    m_fitWidthAct = new QAction(tr("Fit &Width"), this);
    m_fitWidthAct->setShortcut(Qt::CTRL | Qt::Key_1);

    m_fitPageAct = new QAction(tr("Fit &Page"), this);
    m_fitPageAct->setShortcut(Qt::CTRL | Qt::Key_2);

    // Page layout — a radio group (default Continuous).
    auto* layoutGroup = new QActionGroup(this);
    m_singlePageAct = new QAction(tr("&Single Page"), this);
    m_continuousAct = new QAction(tr("&Continuous"), this);
    m_twoUpAct = new QAction(tr("&Two Pages"), this);
    for (QAction* a : {m_singlePageAct, m_continuousAct, m_twoUpAct}) {
        a->setCheckable(true);
        layoutGroup->addAction(a);
    }
    m_continuousAct->setChecked(true);
    connect(m_singlePageAct, &QAction::triggered, this,
            [this] { m_viewport->setLayoutMode(PageView::LayoutMode::SinglePage); });
    connect(m_continuousAct, &QAction::triggered, this,
            [this] { m_viewport->setLayoutMode(PageView::LayoutMode::Continuous); });
    connect(m_twoUpAct, &QAction::triggered, this,
            [this] { m_viewport->setLayoutMode(PageView::LayoutMode::TwoUp); });

    m_toggleThemeAct = new QAction(tr("Toggle &Light / Dark"), this);
    connect(m_toggleThemeAct, &QAction::triggered, this, [] { Theme::instance().toggleMode(); });

    m_immersiveAct = new QAction(tr("&Immersive Reading"), this);
    m_immersiveAct->setCheckable(true);
    m_immersiveAct->setShortcut(Qt::Key_F11);
    connect(m_immersiveAct, &QAction::toggled, this, &MainWindow::setImmersive);
    // Esc leaves immersive reading. The action stays enabled so the shortcut works
    // even with the menu bar hidden.
    addAction(m_immersiveAct);
    auto* escape = new QAction(this);
    escape->setShortcut(Qt::Key_Escape);
    connect(escape, &QAction::triggered, this, [this] {
        if (m_immersive)
            m_immersiveAct->setChecked(false);
    });
    addAction(escape);

    m_aboutAct = new QAction(tr("&About Feather PDF"), this);
    connect(m_aboutAct, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::buildMenus() {
    QMenu* file = menuBar()->addMenu(tr("&File"));
    file->addAction(m_openAct);
    m_recentMenu = file->addMenu(tr("Open &Recent"));
    file->addSeparator();
    file->addAction(m_saveAct);
    file->addAction(m_saveAsAct);
    file->addAction(m_protectAct);
    file->addSeparator();
    file->addAction(m_printAct);
    file->addSeparator();
    file->addAction(m_closeAct);
    file->addSeparator();
    file->addAction(m_quitAct);

    QMenu* edit = menuBar()->addMenu(tr("&Edit"));
    edit->addAction(m_undoAct);
    edit->addAction(m_redoAct);
    edit->addSeparator();
    QAction* find = edit->addAction(tr("&Find…"));
    find->setShortcut(QKeySequence::Find);
    connect(find, &QAction::triggered, this, &MainWindow::openFind);

    QMenu* view = menuBar()->addMenu(tr("&View"));
    view->addAction(m_zoomInAct);
    view->addAction(m_zoomOutAct);
    view->addAction(m_zoomActualAct);
    view->addSeparator();
    view->addAction(m_fitWidthAct);
    view->addAction(m_fitPageAct);
    view->addSeparator();
    QMenu* layoutMenu = view->addMenu(tr("Page &Layout"));
    layoutMenu->addAction(m_singlePageAct);
    layoutMenu->addAction(m_continuousAct);
    layoutMenu->addAction(m_twoUpAct);
    view->addSeparator();
    view->addAction(m_immersiveAct);
    view->addSeparator();
    view->addAction(m_toggleThemeAct);

    QMenu* document = menuBar()->addMenu(tr("&Document"));
    document->addAction(m_rotateLeftAct);
    document->addAction(m_rotateRightAct);
    document->addAction(m_deletePageAct);
    document->addSeparator();
    QAction* props = document->addAction(tr("Properties…"));
    connect(props, &QAction::triggered, this, &MainWindow::showProperties);

    QMenu* tools = menuBar()->addMenu(tr("&Tools"));
    for (const QString& name : {tr("Export"), tr("Create"), tr("Edit"), tr("Comment"),
                                tr("Combine"), tr("Organize"), tr("Redact"), tr("Sign")}) {
        QAction* a = tools->addAction(name);
        connect(a, &QAction::triggered, this, [this, name] { notImplemented(name); });
    }

    QMenu* help = menuBar()->addMenu(tr("&Help"));
    help->addAction(m_aboutAct);
}

void MainWindow::buildShell() {
    auto* shell = new QWidget(this);
    shell->setObjectName("Shell");
    setCentralWidget(shell);

    auto* outer = new QVBoxLayout(shell);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_tabStrip = new TabStrip(shell);
    m_commandBar = new CommandBar(shell);
    m_findBar = new FindBar(shell);
    m_findBar->hide();
    outer->addWidget(m_tabStrip);
    outer->addWidget(m_commandBar);
    outer->addWidget(m_findBar);

    auto* body = new QWidget(shell);
    auto* bodyRow = new QHBoxLayout(body);
    bodyRow->setContentsMargins(0, 0, 0, 0);
    bodyRow->setSpacing(0);

    m_rail = new NavigationRail(body);
    m_viewport = new Viewport(body);
    m_home = new HomeView(body);
    m_toolsPane = new ToolsPane(body);

    // The rail's expandable panel (Thumbnails real; others a placeholder for now).
    m_panelHost = new QWidget(body);
    m_panelHost->setObjectName("RailPanel");
    m_panelHost->setFixedWidth(200);
    auto* panelCol = new QVBoxLayout(m_panelHost);
    panelCol->setContentsMargins(0, 0, 0, 0);
    panelCol->setSpacing(0);
    m_panelHead = new QLabel(m_panelHost);
    m_panelHead->setObjectName("RailPanelHead");
    m_panelHead->setContentsMargins(16, 14, 16, 8);
    panelCol->addWidget(m_panelHead);
    m_panelStack = new QStackedWidget(m_panelHost);
    m_thumbnails = new ThumbnailPanel(m_panelHost);
    m_outline = new OutlinePanel(m_panelHost);
    m_annotations = new AnnotationsPanel(m_panelHost);
    m_attachments = new AttachmentsPanel(m_panelHost);
    m_layers = new LayersPanel(m_panelHost);
    m_panelStack->addWidget(m_thumbnails);  // index 0
    m_panelStack->addWidget(m_outline);     // index 1
    m_panelStack->addWidget(m_annotations); // index 2
    m_panelStack->addWidget(m_attachments); // index 3
    m_panelStack->addWidget(m_layers);      // index 4
    panelCol->addWidget(m_panelStack, 1);
    m_panelHost->setVisible(false);

    // The center swaps between the Home start screen and the document viewport.
    m_centerStack = new QStackedWidget(body);
    m_centerStack->addWidget(m_home);     // index 0
    m_centerStack->addWidget(m_viewport); // index 1

    bodyRow->addWidget(m_rail);
    bodyRow->addWidget(m_panelHost);
    bodyRow->addWidget(m_centerStack, 1);
    bodyRow->addWidget(m_toolsPane);
    outer->addWidget(body, 1);

    // Overlays: the floating pill rides the viewport; the toast rides the shell.
    m_pill = new FloatingPill(m_viewport);
    m_toast = new Toast(shell);
}

void MainWindow::showHome() {
    if (m_immersive)
        m_immersiveAct->setChecked(false); // leave immersive before showing Home
    m_home->refresh();
    m_findBar->hide();
    m_centerStack->setCurrentWidget(m_home);
    m_rail->setVisible(false);
    m_panelHost->setVisible(false);
    m_toolsPane->setVisible(false);
    m_commandBar->setVisible(false);
    m_pill->setVisible(false);
    m_tabStrip->setActiveHome();
}

void MainWindow::showDocument() {
    m_centerStack->setCurrentWidget(m_viewport);
    m_rail->setVisible(true);
    // Restore the rail panel if one was open before we left for Home.
    m_panelHost->setVisible(m_rail->current() != NavigationRail::Panel::None);
    m_toolsPane->setVisible(true);
    m_commandBar->setVisible(true);
    m_pill->setVisible(true);
    // activateSession() owns which document tab is active.
}

void MainWindow::setImmersive(bool on) {
    if (on && !hasActiveDoc()) {
        m_immersiveAct->setChecked(false);
        return;
    }
    if (on == m_immersive)
        return;
    m_immersive = on;

    if (on) {
        // Everything recedes; only the page and its floating pill remain.
        m_panelWasVisible = m_panelHost->isVisible();
        m_findBar->hide();
        m_toast->show(tr("Press Esc or F11 to leave immersive reading"));
    }
    animateChrome(/*collapse=*/on);
}

QVector<QPair<QWidget*, bool>> MainWindow::chromeItems() const {
    // {widget, vertical} — vertical collapses height, horizontal collapses width.
    QVector<QPair<QWidget*, bool>> items{
        {menuBar(), true}, {m_tabStrip, true}, {m_commandBar, true},
        {m_rail, false}, {m_toolsPane, false}};
    if (m_panelWasVisible)
        items.push_back({m_panelHost, false});
    return items;
}

void MainWindow::animateChrome(bool collapse) {
    if (m_chromeAnim) {
        m_chromeAnim->stop();
        m_chromeAnim->deleteLater();
        m_chromeAnim = nullptr;
    }

    const auto items = chromeItems();
    auto* group = new QParallelAnimationGroup(this);
    for (const auto& [w, vertical] : items) {
        int extent;
        if (collapse) {
            extent = vertical ? w->height() : w->width();
            m_chromeExtent[w] = extent;
        } else {
            extent = m_chromeExtent.value(
                w, vertical ? w->sizeHint().height() : w->sizeHint().width());
        }
        // Relax the minimum so the fixed-size bars/panes can actually shrink.
        if (vertical)
            w->setMinimumHeight(0);
        else
            w->setMinimumWidth(0);
        if (!collapse) {
            if (vertical)
                w->setMaximumHeight(0);
            else
                w->setMaximumWidth(0);
            w->show();
        }
        auto* anim = new QPropertyAnimation(w, vertical ? "maximumHeight" : "maximumWidth", group);
        anim->setDuration(220);
        anim->setEasingCurve(QEasingCurve::InOutCubic);
        anim->setStartValue(collapse ? extent : 0);
        anim->setEndValue(collapse ? 0 : extent);
        group->addAnimation(anim);
    }

    connect(group, &QParallelAnimationGroup::finished, this, [this, collapse] {
        for (const auto& [w, vertical] : chromeItems()) {
            if (collapse) {
                w->hide();
            } else {
                const int ext = m_chromeExtent.value(w, 0);
                if (w == menuBar()) {
                    w->setMinimumHeight(0);
                    w->setMaximumHeight(QWIDGETSIZE_MAX);
                } else if (vertical) {
                    w->setFixedHeight(ext);
                } else {
                    w->setFixedWidth(ext);
                }
            }
        }
        m_chromeAnim = nullptr;
    });

    m_chromeAnim = group;
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::wireSignals() {
    // Home start screen → open a file or a recent.
    connect(m_home, &HomeView::openRequested, this, &MainWindow::openFileDialog);
    connect(m_home, &HomeView::openPathRequested, this, [this](const QString& path) { openPath(path); });

    // Viewport → indicators (command bar + pill).
    connect(m_viewport, &Viewport::currentPageChanged, this, &MainWindow::updatePageIndicator);
    connect(m_viewport, &Viewport::pageCountChanged, this, &MainWindow::updatePageIndicator);
    connect(m_viewport, &Viewport::zoomChanged, this, &MainWindow::updateZoomIndicator);

    // Command bar.
    connect(m_commandBar, &CommandBar::zoomInRequested, m_viewport, &Viewport::zoomIn);
    connect(m_commandBar, &CommandBar::zoomOutRequested, m_viewport, &Viewport::zoomOut);
    connect(m_commandBar, &CommandBar::nextPageRequested, this, &MainWindow::nextPage);
    connect(m_commandBar, &CommandBar::prevPageRequested, this, &MainWindow::previousPage);
    connect(m_commandBar, &CommandBar::saveRequested, this, [this] { saveActive(); });
    connect(m_commandBar, &CommandBar::exportRequested, this, [this] { saveActiveAs(); });
    connect(m_commandBar, &CommandBar::printRequested, this, &MainWindow::printActive);
    connect(m_commandBar, &CommandBar::emailRequested, this, [this] { notImplemented(tr("Email")); });
    connect(m_commandBar, &CommandBar::moreRequested, this, [this] {
        QMenu menu(this);
        menu.addAction(m_rotateLeftAct);
        menu.addAction(m_rotateRightAct);
        menu.addAction(m_deletePageAct);
        menu.addSeparator();
        menu.addAction(m_singlePageAct);
        menu.addAction(m_continuousAct);
        menu.addAction(m_twoUpAct);
        menu.addSeparator();
        menu.addAction(m_zoomActualAct);
        menu.addAction(m_fitWidthAct);
        menu.addAction(m_fitPageAct);
        menu.addSeparator();
        for (int pct : {50, 75, 125, 150, 200}) {
            QAction* a = menu.addAction(tr("Zoom to %1%").arg(pct));
            connect(a, &QAction::triggered, this,
                    [this, pct] { m_viewport->setZoomFactor(pct / 100.0); });
        }
        menu.exec(QCursor::pos());
    });

    // Keep the layout radio in sync if the mode changes by any path.
    connect(m_viewport, &Viewport::layoutModeChanged, this, [this](PageView::LayoutMode m) {
        if (m == PageView::LayoutMode::SinglePage)
            m_singlePageAct->setChecked(true);
        else if (m == PageView::LayoutMode::TwoUp)
            m_twoUpAct->setChecked(true);
        else
            m_continuousAct->setChecked(true);
    });
    connect(m_commandBar, &CommandBar::shareRequested, this, [this] { notImplemented(tr("Share")); });
    connect(m_commandBar, &CommandBar::counterClicked, this, [this] {
        if (!hasActiveDoc())
            return;
        const int n = m_doc->pageCount();
        bool ok = false;
        const int page = QInputDialog::getInt(this, tr("Go to page"),
                                              tr("Page (1–%1):").arg(n),
                                              m_viewport->currentPage() + 1, 1, n, 1, &ok);
        if (ok)
            m_viewport->goToPage(page - 1);
    });

    // Floating pill.
    connect(m_pill, &FloatingPill::zoomInRequested, m_viewport, &Viewport::zoomIn);
    connect(m_pill, &FloatingPill::zoomOutRequested, m_viewport, &Viewport::zoomOut);
    connect(m_pill, &FloatingPill::nextPageRequested, this, &MainWindow::nextPage);
    connect(m_pill, &FloatingPill::prevPageRequested, this, &MainWindow::previousPage);

    // View actions → viewport.
    connect(m_zoomInAct, &QAction::triggered, m_viewport, &Viewport::zoomIn);
    connect(m_zoomOutAct, &QAction::triggered, m_viewport, &Viewport::zoomOut);
    connect(m_zoomActualAct, &QAction::triggered, m_viewport, &Viewport::zoomActualSize);
    connect(m_fitWidthAct, &QAction::triggered, m_viewport, &Viewport::fitToWidth);
    connect(m_fitPageAct, &QAction::triggered, m_viewport, &Viewport::fitWholePage);

    // Navigation rail → expandable panel. Each panel owns its own content; the
    // semantic panels (annotations/attachments/layers) load lazily when shown.
    connect(m_rail, &NavigationRail::panelChanged, this, [this](NavigationRail::Panel p) {
        switch (p) {
        case NavigationRail::Panel::None:
            m_panelHost->setVisible(false);
            return;
        case NavigationRail::Panel::Thumbnails:
            m_panelHead->setText(tr("THUMBNAILS"));
            m_panelStack->setCurrentWidget(m_thumbnails);
            break;
        case NavigationRail::Panel::Outline:
            m_panelHead->setText(tr("OUTLINE"));
            m_panelStack->setCurrentWidget(m_outline);
            break;
        case NavigationRail::Panel::Annotations:
            m_panelHead->setText(tr("ANNOTATIONS"));
            m_panelStack->setCurrentWidget(m_annotations);
            break;
        case NavigationRail::Panel::Attachments:
            m_panelHead->setText(tr("ATTACHMENTS"));
            m_panelStack->setCurrentWidget(m_attachments);
            break;
        case NavigationRail::Panel::Layers:
            m_panelHead->setText(tr("LAYERS"));
            m_panelStack->setCurrentWidget(m_layers);
            break;
        }
        m_panelHost->setVisible(true);
    });

    // Thumbnails ↔ viewport: click navigates; the current page stays highlighted.
    connect(m_thumbnails, &ThumbnailPanel::pageActivated, m_viewport, &Viewport::goToPage);
    connect(m_viewport, &Viewport::currentPageChanged, m_thumbnails, &ThumbnailPanel::setCurrentPage);
    connect(m_thumbnails, &ThumbnailPanel::pageMoved, this, [this](int from, int to) {
        Session* s = session(m_activeId);
        if (s && s->undo && hasActiveDoc())
            s->undo->push(new MovePageCommand(s->doc, from, to));
    });

    // Outline → viewport navigation.
    connect(m_outline, &OutlinePanel::pageActivated, m_viewport, &Viewport::goToPage);

    // Annotations list → viewport. Poppler reports the original page index; map
    // it to the current display slot so edits (reorder/delete) are honoured.
    connect(m_annotations, &AnnotationsPanel::annotationActivated, this, [this](int originalPage) {
        if (!m_doc)
            return;
        const int slot = m_doc->pageOrder().indexOf(originalPage);
        if (slot >= 0)
            m_viewport->goToPage(slot);
    });

    // Tools pane.
    connect(m_toolsPane, &ToolsPane::toolActivated, this, [this](const QString& id) {
        // Wire the tools whose backends already exist; the rest await their
        // milestones (Comment→M3, Redact→M2, Sign→M6, Edit→M8, Create→M7).
        if (id == QLatin1String("export")) {
            saveActiveAs();
        } else if (id == QLatin1String("organize")) {
            if (hasActiveDoc())
                m_rail->setCurrentPanel(NavigationRail::Panel::Thumbnails);
        } else if (id == QLatin1String("combine")) {
            combineDocuments();
        } else {
            notImplemented(id.left(1).toUpper() + id.mid(1));
        }
    });
    connect(m_toolsPane, &ToolsPane::customizeRequested, this, [this] { notImplemented(tr("Customize tools")); });

    // Tab strip.
    connect(m_tabStrip, &TabStrip::homeSelected, this, &MainWindow::showHome);
    connect(m_tabStrip, &TabStrip::documentSelected, this, &MainWindow::activateSession);
    connect(m_tabStrip, &TabStrip::toolsSelected, this, [this] { notImplemented(tr("Tools gallery")); });
    connect(m_tabStrip, &TabStrip::newTabRequested, this, &MainWindow::openFileDialog);
    connect(m_tabStrip, &TabStrip::closeDocumentRequested, this, &MainWindow::closeTab);
    connect(m_tabStrip, &TabStrip::searchRequested, this, &MainWindow::openFind);

    // Find bar ↔ viewport search.
    connect(m_findBar, &FindBar::queryChanged, this, [this](const QString& q) {
        m_searchIndex = 0;
        const int count = m_viewport->search(q);
        if (count > 0)
            m_viewport->showSearchResult(0);
        updateFindCount();
    });
    connect(m_findBar, &FindBar::findNext, this, [this] { stepFind(1); });
    connect(m_findBar, &FindBar::findPrevious, this, [this] { stepFind(-1); });
    connect(m_findBar, &FindBar::closed, this, [this] {
        m_viewport->clearSearch();
        m_findBar->hide();
        m_viewport->setFocus();
    });
    connect(m_viewport, &Viewport::searchResultsChanged, this, &MainWindow::updateFindCount);
    connect(m_tabStrip, &TabStrip::menuRequested, this, [this] {
        QMenu menu(this);
        menu.addAction(m_openAct);
        menu.addAction(m_toggleThemeAct);
        menu.addSeparator();
        menu.addAction(m_aboutAct);
        menu.addAction(m_quitAct);
        menu.exec(QCursor::pos());
    });
}

void MainWindow::rotateActivePage(int deltaDegrees) {
    Session* s = session(m_activeId);
    if (!s || !hasActiveDoc() || !s->undo)
        return;
    s->undo->push(new RotatePageCommand(s->doc, m_viewport->currentPage(), deltaDegrees));
}

void MainWindow::deleteActivePage() {
    Session* s = session(m_activeId);
    if (!s || !hasActiveDoc() || !s->undo)
        return;
    if (m_doc->pageCount() <= 1) {
        m_toast->show(tr("A document must keep at least one page."));
        return;
    }
    s->undo->push(new DeletePageCommand(s->doc, m_viewport->currentPage()));
}

bool MainWindow::saveActiveAs() {
    if (!hasActiveDoc())
        return false;
    const QFileInfo info(m_doc->filePath());
    const QString suggested =
        info.dir().filePath(info.completeBaseName() + QStringLiteral("-edited.pdf"));
    const QString out = QFileDialog::getSaveFileName(this, tr("Export PDF"), suggested,
                                                     tr("PDF documents (*.pdf)"));
    if (out.isEmpty())
        return false;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString error;
    const bool ok = PdfEditor::saveArrangement(m_doc->filePath(), out, m_doc->pageOrder(),
                                               m_doc->rotations(), &error);
    QApplication::restoreOverrideCursor();

    if (!ok) {
        QMessageBox::warning(this, tr("Couldn't export"), error);
        return false;
    }
    m_toast->show(tr("Exported to %1").arg(QFileInfo(out).fileName()));
    return true;
}

void MainWindow::combineDocuments() {
    // Seed the list with the current document (if any), then let the user add
    // more and reorder. The merge is lossless (QPDF foreign-page copy).
    CombineDialog dialog(hasActiveDoc() ? m_doc->filePath() : QString(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QStringList inputs = dialog.paths();
    if (inputs.size() < 2)
        return;

    const QString suggested =
        QFileInfo(inputs.first()).dir().filePath(QStringLiteral("combined.pdf"));
    const QString out = QFileDialog::getSaveFileName(this, tr("Save combined PDF"), suggested,
                                                     tr("PDF documents (*.pdf)"));
    if (out.isEmpty())
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString error;
    const bool ok = PdfEditor::combine(inputs, out, &error);
    QApplication::restoreOverrideCursor();

    if (!ok) {
        QMessageBox::warning(this, tr("Couldn't combine"), error);
        return;
    }
    m_toast->show(tr("Combined %1 files").arg(inputs.size()));
    openPath(out); // open the merged result in a new tab
}

void MainWindow::protectDocument() {
    if (!hasActiveDoc())
        return;

    ProtectDialog dialog(m_doc->fileName(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString password = dialog.password();
    if (password.isEmpty())
        return;

    const QFileInfo info(m_doc->filePath());
    const QString suggested =
        info.dir().filePath(info.completeBaseName() + QStringLiteral("-protected.pdf"));
    const QString out = QFileDialog::getSaveFileName(this, tr("Save protected PDF"), suggested,
                                                     tr("PDF documents (*.pdf)"));
    if (out.isEmpty())
        return;

    // Apply the current edit arrangement first (rotation/delete/reorder), then
    // encrypt — so the protected copy reflects what's on screen. Two-step: write
    // the arrangement to the chosen path, then encrypt it in place.
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString error;
    bool ok = PdfEditor::saveArrangement(m_doc->filePath(), out, m_doc->pageOrder(),
                                         m_doc->rotations(), &error);
    if (ok)
        ok = PdfEditor::protect(out, out, password, &error);
    QApplication::restoreOverrideCursor();

    if (!ok) {
        QMessageBox::warning(this, tr("Couldn't protect"), error);
        return;
    }
    // The result is encrypted; the viewer can't reopen it without password
    // support yet, so just confirm rather than loading it into a tab.
    m_toast->show(tr("Saved protected copy to %1").arg(QFileInfo(out).fileName()));
}

bool MainWindow::saveActive() {
    if (!hasActiveDoc())
        return false;
    Session* s = session(m_activeId);
    if (s && s->undo && s->undo->isClean())
        return true; // nothing to write

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString error;
    const bool ok = PdfEditor::saveArrangement(m_doc->filePath(), m_doc->filePath(),
                                               m_doc->pageOrder(), m_doc->rotations(), &error);
    QApplication::restoreOverrideCursor();

    if (!ok) {
        QMessageBox::warning(this, tr("Couldn't save"), error);
        return false;
    }
    m_doc->markSaved();
    if (s && s->undo)
        s->undo->setClean();
    m_toast->show(tr("Saved"));
    return true;
}

void MainWindow::printActive() {
    if (!hasActiveDoc())
        return;

    // Custom dialog: opens instantly, discovers printers off-thread, shows a
    // preview, and supports odd/even subsets — none of which the native dialog
    // does without freezing the UI on open.
    PrintDialog dialog(m_doc->pdf(), m_doc->pageOrder(), m_doc->rotations(), m_doc->fileName(),
                       m_viewport->currentPage(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QList<int> pages = dialog.selectedSlots();
    if (pages.isEmpty())
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QPrinter printer(QPrinter::HighResolution);
    const QString printerName = dialog.selectedPrinter();
    if (printerName.isEmpty()) {
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(dialog.outputFile());
    } else {
        printer.setPrinterName(printerName); // loads the PPD (brief, expected)
    }
    printer.setCopyCount(dialog.copies());
    printer.setDocName(m_doc->fileName());

    printDocument(printer, pages, dialog.grayscale());
    QApplication::restoreOverrideCursor();
    m_toast->show(printerName.isEmpty()
                      ? tr("Saved to %1").arg(QFileInfo(dialog.outputFile()).fileName())
                      : tr("Sent to %1").arg(printerName));
}

void MainWindow::printDocument(QPrinter& printer, const QList<int>& pages, bool grayscale) {
    QPdfDocument* pdf = m_doc->pdf();
    QPainter painter;
    if (!painter.begin(&printer))
        return;

    // Cap the render resolution so high-DPI printers don't make gigantic images;
    // the painter scales each page to fill the sheet.
    const int dpi = std::min(300, printer.resolution());

    bool firstPage = true;
    for (int slot : pages) {
        const int orig = m_doc->originalPageAt(slot);
        if (orig < 0)
            continue;
        if (!firstPage)
            printer.newPage();
        firstPage = false;

        const int rotation = m_doc->rotation(slot);
        QPdfDocumentRenderOptions opts;
        switch (rotation) {
        case 90: opts.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise90); break;
        case 180: opts.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise180); break;
        case 270: opts.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise270); break;
        default: break;
        }

        QSizeF pt = pdf->pagePointSize(orig); // unrotated, in points
        if (rotation == 90 || rotation == 270)
            pt.transpose();
        const QSize imageSize(qRound(pt.width() / 72.0 * dpi), qRound(pt.height() / 72.0 * dpi));
        QImage image = pdf->render(orig, imageSize, opts);
        if (image.isNull())
            continue;
        if (grayscale)
            image = image.convertToFormat(QImage::Format_Grayscale8);

        // Fit the page onto the printable area, preserving aspect and centering.
        const QRect sheet = printer.pageLayout().paintRectPixels(printer.resolution());
        const QSize drawn = image.size().scaled(sheet.size(), Qt::KeepAspectRatio);
        const QRect target(sheet.x() + (sheet.width() - drawn.width()) / 2,
                           sheet.y() + (sheet.height() - drawn.height()) / 2, drawn.width(),
                           drawn.height());
        painter.drawImage(target, image);
    }

    painter.end();
}

void MainWindow::openFind() {
    if (!hasActiveDoc())
        return;
    m_findBar->activate();
    // Re-run the current query so the count reflects the active document.
    if (!m_findBar->query().isEmpty()) {
        m_searchIndex = 0;
        m_viewport->search(m_findBar->query());
        m_viewport->showSearchResult(0);
        updateFindCount();
    }
}

void MainWindow::stepFind(int delta) {
    const int count = m_viewport->searchResultCount();
    if (count <= 0)
        return;
    m_searchIndex += delta;
    m_viewport->showSearchResult(m_searchIndex);
    updateFindCount();
}

void MainWindow::updateFindCount() {
    const int count = m_viewport->searchResultCount();
    const int current = count > 0 ? (((m_searchIndex % count) + count) % count) + 1 : 0;
    m_findBar->setResult(current, count);
}

void MainWindow::nextPage() {
    if (m_viewport->hasDocument())
        m_viewport->goToPage(m_viewport->currentPage() + 1);
}

void MainWindow::previousPage() {
    if (m_viewport->hasDocument())
        m_viewport->goToPage(m_viewport->currentPage() - 1);
}

bool MainWindow::openPath(const QString& path) {
    const QString absolute = QFileInfo(path).absoluteFilePath();

    // Already open? Just focus that tab instead of opening a duplicate.
    for (const Session& s : m_sessions) {
        if (s.path == absolute) {
            activateSession(s.id);
            return true;
        }
    }

    auto* doc = new FeatherDocument(this);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const FeatherDocument::LoadResult result = doc->load(absolute);
    QApplication::restoreOverrideCursor();
    if (result != FeatherDocument::LoadResult::Ok) {
        QMessageBox::warning(this, tr("Couldn't open document"),
                             FeatherDocument::describe(result));
        doc->deleteLater();
        return false;
    }

    addRecentFile(absolute);
    rebuildRecentMenu();

    // A new session gets its own tab and its own undo history.
    Session s;
    s.doc = doc;
    s.path = absolute;
    s.id = m_tabStrip->addDocument(doc->title());
    s.undo = new QUndoStack(this);
    m_undoGroup->addStack(s.undo);
    // The unsaved-changes dot follows this document's clean state.
    const int sid = s.id;
    connect(s.undo, &QUndoStack::cleanChanged, this,
            [this, sid](bool clean) { m_tabStrip->setDocumentDirty(sid, !clean); });
    // Keep the thumbnail panel in step with edits to this document while active.
    connect(doc, &FeatherDocument::pageEdited, this, [this, doc](int slot) {
        if (m_doc == doc)
            m_thumbnails->setRotation(slot, doc->rotation(slot));
    });
    connect(doc, &FeatherDocument::arrangementChanged, this, [this, doc] {
        if (m_doc == doc) {
            m_thumbnails->setArrangement(doc->pageOrder(), doc->rotations());
            updatePageIndicator();
        }
    });
    m_sessions.append(s);
    activateSession(s.id);
    return true;
}

MainWindow::Session* MainWindow::session(int id) {
    for (Session& s : m_sessions) {
        if (s.id == id)
            return &s;
    }
    return nullptr;
}

void MainWindow::activateSession(int id) {
    Session* target = session(id);
    if (!target)
        return;

    // Remember where the outgoing document was before we swap it out.
    if (Session* current = session(m_activeId); current && m_viewport->hasDocument())
        current->lastPage = m_viewport->currentPage();

    m_activeId = id;
    m_doc = target->doc;
    if (target->undo)
        m_undoGroup->setActiveStack(target->undo);
    m_findBar->hide(); // search is per-document; setDocument clears the query
    m_searchIndex = 0;
    m_viewport->setDocument(m_doc);
    m_viewport->goToPage(target->lastPage);
    m_thumbnails->setDocument(m_doc->pdf());
    m_thumbnails->setArrangement(m_doc->pageOrder(), m_doc->rotations());
    m_thumbnails->setCurrentPage(target->lastPage);
    m_outline->setDocument(m_doc->pdf());
    // The semantic panels read the file on disk via Poppler; they rebuild lazily
    // the next time each is shown.
    const QString path = m_doc->filePath();
    m_annotations->setDocumentPath(path);
    m_attachments->setDocumentPath(path);
    m_layers->setDocumentPath(path);

    m_tabStrip->setActiveDocument(id);
    updateWindowTitle();
    updateChromeState();
    showDocument();
}

void MainWindow::closeSession(int id) {
    const int idx = [this, id] {
        for (int i = 0; i < m_sessions.size(); ++i)
            if (m_sessions[i].id == id)
                return i;
        return -1;
    }();
    if (idx < 0)
        return;

    FeatherDocument* doc = m_sessions[idx].doc;
    QUndoStack* stack = m_sessions[idx].undo;
    m_sessions.remove(idx);
    m_tabStrip->removeDocument(id);
    if (stack) {
        m_undoGroup->removeStack(stack);
        stack->deleteLater();
    }

    const bool wasActive = (id == m_activeId);
    if (wasActive) {
        m_activeId = -1;
        m_doc = nullptr;
        m_viewport->clear();
        m_thumbnails->clear();
        m_outline->clear();
        m_annotations->clear();
        m_attachments->clear();
        m_layers->clear();
    }
    if (doc)
        doc->deleteLater();

    if (wasActive) {
        if (!m_sessions.isEmpty())
            activateSession(m_sessions.last().id); // focus the next remaining tab
        else {
            updateWindowTitle();
            updateChromeState();
            showHome();
        }
    }
}

bool MainWindow::hasActiveDoc() const {
    return m_doc != nullptr && m_doc->isLoaded();
}

void MainWindow::openFileDialog() {
    const QString start = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open PDF"), start, tr("PDF documents (*.pdf);;All files (*)"));
    if (!path.isEmpty())
        openPath(path);
}

void MainWindow::closeDocument() {
    if (m_activeId != -1)
        closeTab(m_activeId);
}

void MainWindow::closeTab(int id) {
    if (maybeSaveSession(id))
        closeSession(id);
}

bool MainWindow::maybeSaveSession(int id) {
    Session* s = session(id);
    if (!s || !s->undo || s->undo->isClean())
        return true; // nothing unsaved

    activateSession(id); // show the document the prompt is about
    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("Unsaved changes"));
    box.setText(tr("Save changes to “%1” before closing?").arg(s->doc->title()));
    box.setInformativeText(tr("Your changes will be lost if you don't save them."));
    box.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Save);
    switch (box.exec()) {
    case QMessageBox::Save:
        return saveActive(); // abort the close if the save fails
    case QMessageBox::Discard:
        return true;
    default:
        return false; // Cancel
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Collect tabs with unsaved changes.
    QVector<int> dirty;
    for (const Session& s : m_sessions) {
        if (s.undo && !s.undo->isClean())
            dirty.append(s.id);
    }

    if (!dirty.isEmpty()) {
        QMessageBox box(this);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle(tr("Unsaved changes"));
        box.setText(tr("%n document(s) have unsaved changes.", "", dirty.size()));
        box.setInformativeText(tr("Save them before closing Feather PDF?"));
        box.setStandardButtons(QMessageBox::SaveAll | QMessageBox::Discard | QMessageBox::Cancel);
        box.setDefaultButton(QMessageBox::SaveAll);
        switch (box.exec()) {
        case QMessageBox::SaveAll:
            for (int id : dirty) {
                activateSession(id);
                if (!saveActive()) { // a failed/cancelled save aborts the quit
                    event->ignore();
                    return;
                }
            }
            break;
        case QMessageBox::Discard:
            break;
        default:
            event->ignore();
            return;
        }
    } else if (m_sessions.size() > 1) {
        // No unsaved work, but several tabs are open — confirm the bulk close.
        const auto choice = QMessageBox::question(
            this, tr("Close Feather PDF"),
            tr("%n tab(s) are open. Close them all?", "", m_sessions.size()),
            QMessageBox::Close | QMessageBox::Cancel, QMessageBox::Close);
        if (choice != QMessageBox::Close) {
            event->ignore();
            return;
        }
    }
    event->accept();
}

void MainWindow::updateWindowTitle() {
    if (hasActiveDoc())
        setWindowTitle(tr("%1 — Feather PDF").arg(m_doc->title()));
    else
        setWindowTitle(tr("Feather PDF"));
}

void MainWindow::updateChromeState() {
    const bool loaded = hasActiveDoc();

    m_saveAct->setEnabled(loaded);
    m_saveAsAct->setEnabled(loaded);
    m_protectAct->setEnabled(loaded);
    m_printAct->setEnabled(loaded);
    m_rotateLeftAct->setEnabled(loaded);
    m_rotateRightAct->setEnabled(loaded);
    m_deletePageAct->setEnabled(loaded);
    m_closeAct->setEnabled(loaded);
    m_zoomInAct->setEnabled(loaded);
    m_zoomOutAct->setEnabled(loaded);
    m_zoomActualAct->setEnabled(loaded);
    m_fitWidthAct->setEnabled(loaded);
    m_fitPageAct->setEnabled(loaded);
    m_singlePageAct->setEnabled(loaded);
    m_continuousAct->setEnabled(loaded);
    m_twoUpAct->setEnabled(loaded);
    m_immersiveAct->setEnabled(loaded);

    m_commandBar->setDocumentLoaded(loaded);

    updatePageIndicator();
    updateZoomIndicator();
}

void MainWindow::updatePageIndicator() {
    if (!hasActiveDoc()) {
        m_commandBar->setPageText("— / —");
        m_pill->setPageText("— / —");
        return;
    }
    const QString text =
        tr("%1 / %2").arg(m_viewport->currentPage() + 1).arg(m_viewport->pageCount());
    m_commandBar->setPageText(text);
    m_pill->setPageText(text);
}

void MainWindow::updateZoomIndicator() {
    if (!hasActiveDoc()) {
        m_commandBar->setZoomText("—");
        m_pill->setZoomText("—");
        return;
    }
    const QString text = tr("%1%").arg(qRound(m_viewport->zoomFactor() * 100.0));
    m_commandBar->setZoomText(text);
    m_pill->setZoomText(text);
}

void MainWindow::showAbout() {
    const auto& pal = Theme::instance().palette();

    // Frameless so there is no title-bar "Feather PDF" duplicating the name
    // inside — a clean floating card with a shadow instead.
    QDialog dlg(this);
    dlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setAttribute(Qt::WA_TranslucentBackground);

    auto* outer = new QVBoxLayout(&dlg);
    outer->setContentsMargins(18, 18, 18, 18); // room for the shadow

    auto* card = new QFrame(&dlg);
    card->setObjectName("AboutCard");
    card->setStyleSheet(QStringLiteral("#AboutCard { background:%1; border:1px solid %2;"
                                       " border-radius:16px; }")
                            .arg(pal.surface.name(), pal.hairline.name(QColor::HexArgb)));
    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(44);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 90));
    card->setGraphicsEffect(shadow);
    outer->addWidget(card);

    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(40, 32, 40, 22);
    v->setSpacing(0);
    auto centered = [&](QWidget* w) { v->addWidget(w, 0, Qt::AlignHCenter); };

    auto* logo = new QLabel(card);
    logo->setPixmap(Theme::instance().brandLogo(96));
    centered(logo);
    v->addSpacing(16);

    auto* title = new QLabel(tr("Feather PDF"), &dlg);
    title->setStyleSheet(
        QStringLiteral("color:%1; font-size:24px; font-weight:700;").arg(pal.text.name()));
    centered(title);
    v->addSpacing(2);

    auto* ver = new QLabel(tr("Version %1").arg(QStringLiteral(FEATHERPDF_VERSION)), &dlg);
    ver->setStyleSheet(QStringLiteral("color:%1; font-family:'Source Code Pro',monospace;"
                                      " font-size:12px;")
                           .arg(pal.dim.name()));
    centered(ver);
    v->addSpacing(12);

    auto* tagline = new QLabel(tr("Light on the system, full-featured on PDF."), &dlg);
    tagline->setStyleSheet(QStringLiteral("color:%1; font-size:13px;").arg(pal.text.name()));
    centered(tagline);
    v->addSpacing(4);

    auto* desc = new QLabel(
        tr("A native, open-source PDF tool for Linux, licensed under the GPLv3."), &dlg);
    desc->setWordWrap(true);
    desc->setAlignment(Qt::AlignHCenter);
    desc->setFixedWidth(300);
    desc->setStyleSheet(QStringLiteral("color:%1; font-size:12px;").arg(pal.dim.name()));
    centered(desc);
    v->addSpacing(18);

    auto* links = new QLabel(&dlg);
    links->setTextFormat(Qt::RichText);
    links->setOpenExternalLinks(true);
    const QString a = QStringLiteral("color:%1; text-decoration:none; font-weight:600;")
                          .arg(pal.accent.name());
    links->setText(
        tr("<a style='%1' href='https://github.com/s4rt4/featherpdf-linux'>GitHub</a>"
           "&nbsp;&nbsp;·&nbsp;&nbsp;"
           "<a style='%1' href='https://www.gnu.org/licenses/gpl-3.0.html'>License</a>")
            .arg(a));
    centered(links);
    v->addSpacing(20);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    v->addWidget(buttons);

    dlg.exec();
}

void MainWindow::showProperties() {
    if (!hasActiveDoc())
        return;
    QPdfDocument* pdf = m_doc->pdf();

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Document properties"));
    auto* form = new QFormLayout(&dialog);
    form->setRowWrapPolicy(QFormLayout::DontWrapRows);

    auto addRow = [&](const QString& label, const QString& value, bool mono = false) {
        if (value.trimmed().isEmpty())
            return;
        auto* v = new QLabel(value, &dialog);
        v->setTextInteractionFlags(Qt::TextSelectableByMouse);
        if (mono) {
            QFont f = v->font();
            f.setFamily(QStringLiteral("monospace"));
            v->setFont(f);
        }
        form->addRow(new QLabel(label, &dialog), v);
    };
    auto meta = [&](QPdfDocument::MetaDataField f) { return pdf->metaData(f).toString(); };

    addRow(tr("Title"), meta(QPdfDocument::MetaDataField::Title));
    addRow(tr("Author"), meta(QPdfDocument::MetaDataField::Author));
    addRow(tr("Subject"), meta(QPdfDocument::MetaDataField::Subject));
    addRow(tr("Keywords"), meta(QPdfDocument::MetaDataField::Keywords));
    addRow(tr("Creator"), meta(QPdfDocument::MetaDataField::Creator));
    addRow(tr("Producer"), meta(QPdfDocument::MetaDataField::Producer));

    const QString created =
        pdf->metaData(QPdfDocument::MetaDataField::CreationDate).toDateTime().toString(Qt::TextDate);
    addRow(tr("Created"), created);

    const int original = m_doc->originalPageCount();
    QString pages = QString::number(m_doc->pageCount());
    if (m_doc->pageCount() != original)
        pages += tr(" (was %1)").arg(original);
    addRow(tr("Pages"), pages, /*mono=*/true);

    const QSizeF sz = pdf->pagePointSize(m_doc->originalPageAt(m_viewport->currentPage()));
    addRow(tr("Page size"), tr("%1 × %2 pt").arg(qRound(sz.width())).arg(qRound(sz.height())), true);

    const QFileInfo fi(m_doc->filePath());
    addRow(tr("File size"), QLocale().formattedDataSize(fi.size()), true);
    addRow(tr("Location"), fi.absoluteFilePath(), true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    form->addRow(buttons);

    dialog.exec();
}

void MainWindow::notImplemented(const QString& feature) {
    m_toast->show(tr("%1 arrives in a later milestone.").arg(feature));
}

QStringList MainWindow::recentFiles() const {
    QSettings settings;
    return settings.value(kRecentFilesKey).toStringList();
}

void MainWindow::addRecentFile(const QString& path) {
    QSettings settings;
    QStringList files = settings.value(kRecentFilesKey).toStringList();
    files.removeAll(path);
    files.prepend(path);
    while (files.size() > kMaxRecentFiles)
        files.removeLast();
    settings.setValue(kRecentFilesKey, files);
}

void MainWindow::rebuildRecentMenu() {
    m_recentMenu->clear();
    const QStringList files = recentFiles();
    if (files.isEmpty()) {
        QAction* empty = m_recentMenu->addAction(tr("No recent documents"));
        empty->setEnabled(false);
        return;
    }
    for (const QString& path : files) {
        QAction* act = m_recentMenu->addAction(QFileInfo(path).fileName());
        act->setData(path);
        act->setToolTip(path);
        connect(act, &QAction::triggered, this, [this, path] { openPath(path); });
    }
    m_recentMenu->addSeparator();
    QAction* clear = m_recentMenu->addAction(tr("Clear list"));
    connect(clear, &QAction::triggered, this, [this] {
        QSettings().remove(kRecentFilesKey);
        rebuildRecentMenu();
    });
}
