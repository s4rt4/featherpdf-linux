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

#include "ui/Viewport.h"

#include "core/FeatherDocument.h"
#include "ui/PageView.h"

#include <QVBoxLayout>

Viewport::Viewport(QWidget* parent) : QWidget(parent), m_view(new PageView(this)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    connect(m_view, &PageView::zoomChanged, this,
            [this](double f) { emit zoomChanged(f); });
    connect(m_view, &PageView::currentPageChanged, this,
            [this](int page) { emit currentPageChanged(page); });
    connect(m_view, &PageView::searchResultsChanged, this,
            [this](int count) { emit searchResultsChanged(count); });
    connect(m_view, &PageView::layoutModeChanged, this,
            [this](PageView::LayoutMode m) { emit layoutModeChanged(m); });
}

Viewport::~Viewport() = default;

void Viewport::setDocument(FeatherDocument* doc) {
    if (m_doc)
        disconnect(m_doc, nullptr, this, nullptr);
    m_doc = doc;
    m_view->setDocument(doc ? doc->pdf() : nullptr);
    if (doc) {
        m_view->setArrangement(doc->pageOrder(), doc->rotations());
        // Follow façade edits: a single slot's rotation, or a structure change.
        connect(doc, &FeatherDocument::pageEdited, this,
                [this, doc](int slot) { m_view->setRotation(slot, doc->rotation(slot)); });
        connect(doc, &FeatherDocument::arrangementChanged, this, [this, doc] {
            m_view->setArrangement(doc->pageOrder(), doc->rotations());
            emit pageCountChanged(pageCount());
            emit currentPageChanged(currentPage());
        });
    }
    emit pageCountChanged(pageCount());
    emit currentPageChanged(currentPage());
    emit zoomChanged(zoomFactor());
}

void Viewport::clear() {
    m_doc = nullptr;
    m_view->setDocument(nullptr);
    emit pageCountChanged(0);
}

bool Viewport::hasDocument() const {
    return m_doc != nullptr && m_doc->isLoaded();
}

double Viewport::zoomFactor() const {
    return m_view->zoom();
}

void Viewport::zoomIn() {
    m_view->zoomIn();
}

void Viewport::zoomOut() {
    m_view->zoomOut();
}

void Viewport::zoomActualSize() {
    m_view->zoomActualSize();
}

void Viewport::fitToWidth() {
    m_view->fitToWidth();
}

void Viewport::fitWholePage() {
    m_view->fitWholePage();
}

void Viewport::setZoomFactor(double factor) {
    m_view->setZoom(factor);
}

void Viewport::setLayoutMode(PageView::LayoutMode mode) {
    m_view->setLayoutMode(mode);
}

PageView::LayoutMode Viewport::layoutMode() const {
    return m_view->layoutMode();
}

int Viewport::currentPage() const {
    return m_view->currentPage();
}

int Viewport::pageCount() const {
    return m_doc ? m_doc->pageCount() : 0;
}

void Viewport::goToPage(int page) {
    m_view->goToPage(page);
}

int Viewport::search(const QString& query) {
    return m_view->search(query);
}

void Viewport::clearSearch() {
    m_view->clearSearch();
}

void Viewport::showSearchResult(int index) {
    m_view->showSearchResult(index);
}

int Viewport::searchResultCount() const {
    return m_view->searchResultCount();
}
