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
#include "ui/Theme.h"

#include <QPdfDocument>
#include <QPdfPageNavigator>
#include <QPdfView>
#include <QVBoxLayout>
#include <algorithm>

Viewport::Viewport(QWidget* parent) : QWidget(parent), m_view(new QPdfView(this)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    // Continuous scroll by default, with breathing room around each page so it
    // reads as paper floating on the canvas (§5 of the UI guidelines).
    m_view->setPageMode(QPdfView::PageMode::MultiPage);
    m_view->setZoomMode(QPdfView::ZoomMode::Custom);
    m_view->setPageSpacing(12);
    m_view->setDocumentMargins(QMargins(16, 16, 16, 16));

    connect(m_view, &QPdfView::zoomFactorChanged, this,
            [this](qreal f) { emit zoomChanged(static_cast<double>(f)); });

    if (auto* nav = m_view->pageNavigator()) {
        connect(nav, &QPdfPageNavigator::currentPageChanged, this,
                [this](int page) { emit currentPageChanged(page); });
    }

    // The canvas behind the page is `surface-sunken`, so white pages pop and
    // carry the only prominent shadow (ui-guidelines §1, §5.6).
    applyCanvasColor();
    connect(&Theme::instance(), &Theme::changed, this, &Viewport::applyCanvasColor);
}

void Viewport::applyCanvasColor() {
    const QColor sunken = Theme::instance().palette().sunken;
    m_view->setStyleSheet(QStringLiteral("QPdfView { background:%1; border:none; }")
                              .arg(sunken.name()));
}

Viewport::~Viewport() = default;

void Viewport::setDocument(FeatherDocument* doc) {
    m_doc = doc;
    m_view->setDocument(doc ? doc->pdf() : nullptr);
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
    return static_cast<double>(m_view->zoomFactor());
}

void Viewport::applyZoomFactor(double factor) {
    factor = std::clamp(factor, kMinZoom, kMaxZoom);
    m_view->setZoomMode(QPdfView::ZoomMode::Custom);
    m_view->setZoomFactor(factor);
}

void Viewport::zoomIn() {
    applyZoomFactor(zoomFactor() * kZoomStep);
}

void Viewport::zoomOut() {
    applyZoomFactor(zoomFactor() / kZoomStep);
}

void Viewport::zoomActualSize() {
    applyZoomFactor(1.0);
}

void Viewport::fitToWidth() {
    m_view->setZoomMode(QPdfView::ZoomMode::FitToWidth);
}

void Viewport::fitWholePage() {
    m_view->setZoomMode(QPdfView::ZoomMode::FitInView);
}

int Viewport::currentPage() const {
    auto* nav = m_view->pageNavigator();
    return nav ? nav->currentPage() : 0;
}

int Viewport::pageCount() const {
    return m_doc ? m_doc->pageCount() : 0;
}

void Viewport::goToPage(int page) {
    auto* nav = m_view->pageNavigator();
    if (!nav || pageCount() <= 0)
        return;
    page = std::clamp(page, 0, pageCount() - 1);
    // Jump straight to the page top — accurate long jumps must be instant, not
    // animated (saran §2). Animated smooth-scroll is unreliable across hundreds
    // of pages.
    nav->jump(page, QPointF(0, 0), nav->currentZoom());
}
