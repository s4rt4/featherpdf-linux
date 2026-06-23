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

#include "core/FeatherDocument.h"
#include "ui/CommandBar.h"
#include "ui/FindBar.h"
#include "ui/FloatingPill.h"
#include "ui/HomeView.h"
#include "ui/NavigationRail.h"
#include "ui/OutlinePanel.h"
#include "ui/TabStrip.h"
#include "ui/Theme.h"
#include "ui/ThumbnailPanel.h"
#include "ui/Toast.h"
#include "ui/ToolsPane.h"
#include "ui/Viewport.h"

#include <QAction>
#include <QApplication>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QStandardPaths>
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

    m_closeAct = new QAction(tr("&Close"), this);
    m_closeAct->setShortcut(QKeySequence::Close);
    connect(m_closeAct, &QAction::triggered, this, &MainWindow::closeDocument);

    m_quitAct = new QAction(tr("&Quit"), this);
    m_quitAct->setShortcut(QKeySequence::Quit);
    connect(m_quitAct, &QAction::triggered, qApp, &QApplication::quit);

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
    connect(m_aboutAct, &QAction::triggered, this, [this] {
        QMessageBox about(this);
        about.setWindowTitle(tr("About Feather PDF"));
        about.setIconPixmap(Theme::instance().brandLogo(72));
        about.setText(tr("<h3>Feather PDF %1</h3>"
                         "<p>Light on the system, full-featured on PDF.</p>"
                         "<p>A native, open-source PDF tool for Linux.</p>"
                         "<p>Licensed under the GNU General Public License v3.</p>")
                          .arg(QStringLiteral(FEATHERPDF_VERSION)));
        about.exec();
    });
}

void MainWindow::buildMenus() {
    QMenu* file = menuBar()->addMenu(tr("&File"));
    file->addAction(m_openAct);
    m_recentMenu = file->addMenu(tr("Open &Recent"));
    file->addSeparator();
    file->addAction(m_closeAct);
    file->addSeparator();
    file->addAction(m_quitAct);

    QMenu* edit = menuBar()->addMenu(tr("&Edit"));
    QAction* undo = edit->addAction(tr("&Undo"));
    QAction* redo = edit->addAction(tr("&Redo"));
    undo->setShortcut(QKeySequence::Undo);
    redo->setShortcut(QKeySequence::Redo);
    undo->setEnabled(false); // a unified undo stack arrives with editing milestones
    redo->setEnabled(false);
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
    view->addAction(m_immersiveAct);
    view->addSeparator();
    view->addAction(m_toggleThemeAct);

    QMenu* document = menuBar()->addMenu(tr("&Document"));
    for (const auto& [label, feature] :
         {std::pair{tr("Rotate View"), tr("Rotate view")},
          std::pair{tr("Properties…"), tr("Document properties")}}) {
        QAction* a = document->addAction(label);
        const QString f = feature;
        connect(a, &QAction::triggered, this, [this, f] { notImplemented(f); });
    }

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
    m_panelPlaceholder = new QLabel(m_panelHost);
    m_panelPlaceholder->setObjectName("RailPanelPlaceholder");
    m_panelPlaceholder->setAlignment(Qt::AlignCenter);
    m_panelPlaceholder->setWordWrap(true);
    m_panelPlaceholder->setContentsMargins(16, 16, 16, 16);
    m_panelStack->addWidget(m_thumbnails);       // index 0
    m_panelStack->addWidget(m_outline);          // index 1
    m_panelStack->addWidget(m_panelPlaceholder); // index 2
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
    connect(m_commandBar, &CommandBar::saveRequested, this, [this] { notImplemented(tr("Save")); });
    connect(m_commandBar, &CommandBar::exportRequested, this, [this] { notImplemented(tr("Export")); });
    connect(m_commandBar, &CommandBar::printRequested, this, [this] { notImplemented(tr("Print")); });
    connect(m_commandBar, &CommandBar::emailRequested, this, [this] { notImplemented(tr("Email")); });
    connect(m_commandBar, &CommandBar::findRequested, this, &MainWindow::openFind);
    connect(m_commandBar, &CommandBar::moreRequested, this, [this] { notImplemented(tr("More options")); });
    connect(m_commandBar, &CommandBar::shareRequested, this, [this] { notImplemented(tr("Share")); });

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

    // Navigation rail → expandable panel. Thumbnails is real; the others show a
    // placeholder until their increments land.
    connect(m_rail, &NavigationRail::panelChanged, this, [this](NavigationRail::Panel p) {
        if (p == NavigationRail::Panel::None) {
            m_panelHost->setVisible(false);
            return;
        }
        if (p == NavigationRail::Panel::Thumbnails) {
            m_panelHead->setText(tr("THUMBNAILS"));
            m_panelStack->setCurrentWidget(m_thumbnails);
        } else if (p == NavigationRail::Panel::Outline) {
            m_panelHead->setText(tr("OUTLINE"));
            m_panelStack->setCurrentWidget(m_outline);
        } else {
            static const QHash<NavigationRail::Panel, QString> names{
                {NavigationRail::Panel::Annotations, tr("ANNOTATIONS")},
                {NavigationRail::Panel::Attachments, tr("ATTACHMENTS")},
                {NavigationRail::Panel::Layers, tr("LAYERS")}};
            m_panelHead->setText(names.value(p));
            m_panelPlaceholder->setText(tr("This panel arrives in a later milestone."));
            m_panelStack->setCurrentWidget(m_panelPlaceholder);
        }
        m_panelHost->setVisible(true);
    });

    // Thumbnails ↔ viewport: click navigates; the current page stays highlighted.
    connect(m_thumbnails, &ThumbnailPanel::pageActivated, m_viewport, &Viewport::goToPage);
    connect(m_viewport, &Viewport::currentPageChanged, m_thumbnails, &ThumbnailPanel::setCurrentPage);

    // Outline → viewport navigation.
    connect(m_outline, &OutlinePanel::pageActivated, m_viewport, &Viewport::goToPage);

    // Tools pane.
    connect(m_toolsPane, &ToolsPane::toolActivated, this, [this](const QString& id) {
        notImplemented(id.left(1).toUpper() + id.mid(1));
    });
    connect(m_toolsPane, &ToolsPane::customizeRequested, this, [this] { notImplemented(tr("Customize tools")); });

    // Tab strip.
    connect(m_tabStrip, &TabStrip::homeSelected, this, &MainWindow::showHome);
    connect(m_tabStrip, &TabStrip::documentSelected, this, &MainWindow::activateSession);
    connect(m_tabStrip, &TabStrip::toolsSelected, this, [this] { notImplemented(tr("Tools gallery")); });
    connect(m_tabStrip, &TabStrip::newTabRequested, this, &MainWindow::openFileDialog);
    connect(m_tabStrip, &TabStrip::closeDocumentRequested, this, &MainWindow::closeSession);
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

    // A new session gets its own tab; opening focuses it.
    Session s;
    s.doc = doc;
    s.path = absolute;
    s.id = m_tabStrip->addDocument(doc->title());
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
    m_findBar->hide(); // search is per-document; setDocument clears the query
    m_searchIndex = 0;
    m_viewport->setDocument(m_doc);
    m_viewport->goToPage(target->lastPage);
    m_thumbnails->setDocument(m_doc->pdf());
    m_thumbnails->setCurrentPage(target->lastPage);
    m_outline->setDocument(m_doc->pdf());

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
    m_sessions.remove(idx);
    m_tabStrip->removeDocument(id);

    const bool wasActive = (id == m_activeId);
    if (wasActive) {
        m_activeId = -1;
        m_doc = nullptr;
        m_viewport->clear();
        m_thumbnails->clear();
        m_outline->clear();
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
        closeSession(m_activeId);
}

void MainWindow::updateWindowTitle() {
    if (hasActiveDoc())
        setWindowTitle(tr("%1 — Feather PDF").arg(m_doc->title()));
    else
        setWindowTitle(tr("Feather PDF"));
}

void MainWindow::updateChromeState() {
    const bool loaded = hasActiveDoc();

    m_closeAct->setEnabled(loaded);
    m_zoomInAct->setEnabled(loaded);
    m_zoomOutAct->setEnabled(loaded);
    m_zoomActualAct->setEnabled(loaded);
    m_fitWidthAct->setEnabled(loaded);
    m_fitPageAct->setEnabled(loaded);
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
