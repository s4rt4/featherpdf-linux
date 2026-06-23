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
#include "ui/FloatingPill.h"
#include "ui/HomeView.h"
#include "ui/NavigationRail.h"
#include "ui/TabStrip.h"
#include "ui/Theme.h"
#include "ui/Toast.h"
#include "ui/ToolsPane.h"
#include "ui/Viewport.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeySequence>
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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_doc(new FeatherDocument(this)) {
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
    connect(find, &QAction::triggered, this, [this] { notImplemented(tr("Find")); });

    QMenu* view = menuBar()->addMenu(tr("&View"));
    view->addAction(m_zoomInAct);
    view->addAction(m_zoomOutAct);
    view->addAction(m_zoomActualAct);
    view->addSeparator();
    view->addAction(m_fitWidthAct);
    view->addAction(m_fitPageAct);
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
    outer->addWidget(m_tabStrip);
    outer->addWidget(m_commandBar);

    auto* body = new QWidget(shell);
    auto* bodyRow = new QHBoxLayout(body);
    bodyRow->setContentsMargins(0, 0, 0, 0);
    bodyRow->setSpacing(0);

    m_rail = new NavigationRail(body);
    m_viewport = new Viewport(body);
    m_home = new HomeView(body);
    m_toolsPane = new ToolsPane(body);

    // The center swaps between the Home start screen and the document viewport.
    m_centerStack = new QStackedWidget(body);
    m_centerStack->addWidget(m_home);     // index 0
    m_centerStack->addWidget(m_viewport); // index 1

    bodyRow->addWidget(m_rail);
    bodyRow->addWidget(m_centerStack, 1);
    bodyRow->addWidget(m_toolsPane);
    outer->addWidget(body, 1);

    // Overlays: the floating pill rides the viewport; the toast rides the shell.
    m_pill = new FloatingPill(m_viewport);
    m_toast = new Toast(shell);
}

void MainWindow::showHome() {
    m_home->refresh();
    m_centerStack->setCurrentWidget(m_home);
    m_rail->setVisible(false);
    m_toolsPane->setVisible(false);
    m_commandBar->setVisible(false);
    m_pill->setVisible(false);
    m_tabStrip->setActive(TabStrip::Active::Home);
}

void MainWindow::showDocument() {
    m_centerStack->setCurrentWidget(m_viewport);
    m_rail->setVisible(true);
    m_toolsPane->setVisible(true);
    m_commandBar->setVisible(true);
    m_pill->setVisible(true);
    m_tabStrip->setActive(TabStrip::Active::Document);
}

void MainWindow::wireSignals() {
    // Document load → wire into the viewport, refresh chrome, enter the workspace.
    connect(m_doc, &FeatherDocument::loaded, this, [this] {
        m_viewport->setDocument(m_doc);
        updateWindowTitle();
        updateChromeState();
        showDocument();
    });

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
    connect(m_commandBar, &CommandBar::findRequested, this, [this] { notImplemented(tr("Find")); });
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

    // Navigation rail → panels (arrive later).
    connect(m_rail, &NavigationRail::panelChanged, this, [this](NavigationRail::Panel p) {
        if (p == NavigationRail::Panel::None)
            return;
        notImplemented(tr("Side panels"));
    });

    // Tools pane.
    connect(m_toolsPane, &ToolsPane::toolActivated, this, [this](const QString& id) {
        notImplemented(id.left(1).toUpper() + id.mid(1));
    });
    connect(m_toolsPane, &ToolsPane::customizeRequested, this, [this] { notImplemented(tr("Customize tools")); });

    // Tab strip.
    connect(m_tabStrip, &TabStrip::homeSelected, this, &MainWindow::showHome);
    connect(m_tabStrip, &TabStrip::documentSelected, this, [this] {
        if (m_doc->isLoaded())
            showDocument();
    });
    connect(m_tabStrip, &TabStrip::toolsSelected, this, [this] { notImplemented(tr("Tools gallery")); });
    connect(m_tabStrip, &TabStrip::newTabRequested, this, &MainWindow::openFileDialog);
    connect(m_tabStrip, &TabStrip::closeDocumentRequested, this, &MainWindow::closeDocument);
    connect(m_tabStrip, &TabStrip::searchRequested, this, [this] { notImplemented(tr("Search")); });
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
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const FeatherDocument::LoadResult result = m_doc->load(absolute);
    QApplication::restoreOverrideCursor();
    if (result != FeatherDocument::LoadResult::Ok) {
        QMessageBox::warning(this, tr("Couldn't open document"),
                             FeatherDocument::describe(result));
        return false;
    }
    addRecentFile(absolute);
    rebuildRecentMenu();
    return true;
}

void MainWindow::openFileDialog() {
    const QString start = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open PDF"), start, tr("PDF documents (*.pdf);;All files (*)"));
    if (!path.isEmpty())
        openPath(path);
}

void MainWindow::closeDocument() {
    m_viewport->clear();
    m_doc->close();
    updateWindowTitle();
    updateChromeState();
    showHome();
}

void MainWindow::updateWindowTitle() {
    if (m_doc->isLoaded())
        setWindowTitle(tr("%1 — Feather PDF").arg(m_doc->title()));
    else
        setWindowTitle(tr("Feather PDF"));
}

void MainWindow::updateChromeState() {
    const bool loaded = m_doc->isLoaded();

    m_closeAct->setEnabled(loaded);
    m_zoomInAct->setEnabled(loaded);
    m_zoomOutAct->setEnabled(loaded);
    m_zoomActualAct->setEnabled(loaded);
    m_fitWidthAct->setEnabled(loaded);
    m_fitPageAct->setEnabled(loaded);

    m_commandBar->setDocumentLoaded(loaded);

    // The document tab reflects the open file; view-switching (showHome /
    // showDocument) owns which tab is active and the document-only chrome.
    m_tabStrip->setDocumentTitle(loaded ? m_doc->title() : QString());

    updatePageIndicator();
    updateZoomIndicator();
}

void MainWindow::updatePageIndicator() {
    if (!m_doc->isLoaded()) {
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
    if (!m_doc->isLoaded()) {
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
