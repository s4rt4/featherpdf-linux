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
#include "ui/Viewport.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>
#include <QToolBar>

namespace {
constexpr auto kRecentFilesKey = "recentFiles";
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_doc(new FeatherDocument(this)), m_viewport(new Viewport(this)) {
    setCentralWidget(m_viewport);

    buildActions();
    buildMenus();
    buildToolBar();
    buildStatusBar();

    connect(m_viewport, &Viewport::currentPageChanged, this, &MainWindow::updatePageIndicator);
    connect(m_viewport, &Viewport::pageCountChanged, this, &MainWindow::updatePageIndicator);
    connect(m_viewport, &Viewport::zoomChanged, this, &MainWindow::updateZoomIndicator);
    connect(m_doc, &FeatherDocument::loaded, this, [this] {
        m_viewport->setDocument(m_doc);
        updateWindowTitle();
        updateActionsEnabled();
        updatePageIndicator();
        updateZoomIndicator();
    });

    resize(1100, 760);
    updateWindowTitle();
    updateActionsEnabled();
    updatePageIndicator();
    updateZoomIndicator();
    rebuildRecentMenu();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildActions() {
    m_openAct = new QAction(tr("&Open…"), this);
    m_openAct->setShortcut(QKeySequence::Open);
    m_openAct->setIcon(QIcon::fromTheme("document-open"));
    connect(m_openAct, &QAction::triggered, this, &MainWindow::openFileDialog);

    m_closeAct = new QAction(tr("&Close"), this);
    m_closeAct->setShortcut(QKeySequence::Close);
    connect(m_closeAct, &QAction::triggered, this, &MainWindow::closeDocument);

    m_quitAct = new QAction(tr("&Quit"), this);
    m_quitAct->setShortcut(QKeySequence::Quit);
    connect(m_quitAct, &QAction::triggered, qApp, &QApplication::quit);

    m_zoomInAct = new QAction(tr("Zoom &In"), this);
    m_zoomInAct->setShortcut(QKeySequence::ZoomIn);
    m_zoomInAct->setIcon(QIcon::fromTheme("zoom-in"));
    connect(m_zoomInAct, &QAction::triggered, m_viewport, &Viewport::zoomIn);

    m_zoomOutAct = new QAction(tr("Zoom &Out"), this);
    m_zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    m_zoomOutAct->setIcon(QIcon::fromTheme("zoom-out"));
    connect(m_zoomOutAct, &QAction::triggered, m_viewport, &Viewport::zoomOut);

    m_zoomActualAct = new QAction(tr("&Actual Size"), this);
    m_zoomActualAct->setShortcut(Qt::CTRL | Qt::Key_0);
    connect(m_zoomActualAct, &QAction::triggered, m_viewport, &Viewport::zoomActualSize);

    m_fitWidthAct = new QAction(tr("Fit &Width"), this);
    m_fitWidthAct->setShortcut(Qt::CTRL | Qt::Key_1);
    connect(m_fitWidthAct, &QAction::triggered, m_viewport, &Viewport::fitToWidth);

    m_fitPageAct = new QAction(tr("Fit &Page"), this);
    m_fitPageAct->setShortcut(Qt::CTRL | Qt::Key_2);
    connect(m_fitPageAct, &QAction::triggered, m_viewport, &Viewport::fitWholePage);

    m_aboutAct = new QAction(tr("&About Feather PDF"), this);
    connect(m_aboutAct, &QAction::triggered, this, [this] {
        QMessageBox::about(
            this, tr("About Feather PDF"),
            tr("<h3>Feather PDF %1</h3>"
               "<p>Light on the system, full-featured on PDF.</p>"
               "<p>A native, open-source PDF tool for Linux.</p>"
               "<p>Licensed under the GNU General Public License v3.</p>")
                .arg(QStringLiteral(FEATHERPDF_VERSION)));
    });
}

void MainWindow::buildMenus() {
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_openAct);
    m_recentMenu = fileMenu->addMenu(tr("Open &Recent"));
    fileMenu->addSeparator();
    fileMenu->addAction(m_closeAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_quitAct);

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_zoomInAct);
    viewMenu->addAction(m_zoomOutAct);
    viewMenu->addAction(m_zoomActualAct);
    viewMenu->addSeparator();
    viewMenu->addAction(m_fitWidthAct);
    viewMenu->addAction(m_fitPageAct);

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_aboutAct);
}

void MainWindow::buildToolBar() {
    QToolBar* tb = addToolBar(tr("Main"));
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tb->addAction(m_openAct);
    tb->addSeparator();
    tb->addAction(m_zoomOutAct);
    tb->addAction(m_zoomInAct);
    tb->addAction(m_fitWidthAct);
}

void MainWindow::buildStatusBar() {
    m_pageLabel = new QLabel(this);
    m_zoomLabel = new QLabel(this);
    // Page counter and zoom are precise technical values → monospace (typography §3).
    QFont mono = m_pageLabel->font();
    mono.setStyleHint(QFont::Monospace);
    mono.setFamily("monospace");
    m_pageLabel->setFont(mono);
    m_zoomLabel->setFont(mono);
    statusBar()->addPermanentWidget(m_zoomLabel);
    statusBar()->addPermanentWidget(m_pageLabel);
}

bool MainWindow::openPath(const QString& path) {
    const QString absolute = QFileInfo(path).absoluteFilePath();
    // Loading large documents blocks the UI thread today (the custom render
    // pipeline will move it off-thread); a busy cursor is the honest feedback.
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
    const QString start =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open PDF"), start, tr("PDF documents (*.pdf);;All files (*)"));
    if (!path.isEmpty())
        openPath(path);
}

void MainWindow::closeDocument() {
    m_viewport->clear();
    m_doc->close();
    updateWindowTitle();
    updateActionsEnabled();
    updatePageIndicator();
    updateZoomIndicator();
}

void MainWindow::updateWindowTitle() {
    if (m_doc->isLoaded())
        setWindowTitle(tr("%1 — Feather PDF").arg(m_doc->title()));
    else
        setWindowTitle(tr("Feather PDF"));
}

void MainWindow::updateActionsEnabled() {
    const bool loaded = m_doc->isLoaded();
    m_closeAct->setEnabled(loaded);
    m_zoomInAct->setEnabled(loaded);
    m_zoomOutAct->setEnabled(loaded);
    m_zoomActualAct->setEnabled(loaded);
    m_fitWidthAct->setEnabled(loaded);
    m_fitPageAct->setEnabled(loaded);
}

void MainWindow::updatePageIndicator() {
    if (!m_doc->isLoaded()) {
        m_pageLabel->clear();
        return;
    }
    // currentPage() is 0-based; show 1-based to the user.
    m_pageLabel->setText(
        tr("%1 / %2").arg(m_viewport->currentPage() + 1).arg(m_viewport->pageCount()));
}

void MainWindow::updateZoomIndicator() {
    if (!m_doc->isLoaded()) {
        m_zoomLabel->clear();
        return;
    }
    m_zoomLabel->setText(tr("%1%").arg(qRound(m_viewport->zoomFactor() * 100.0)));
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
