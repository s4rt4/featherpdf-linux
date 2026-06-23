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

#include "ui/PageView.h"

#include "ui/Theme.h"

#include <QKeyEvent>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfLink>
#include <QPdfPageRenderer>
#include <QPdfSearchModel>
#include <QScrollBar>
#include <QWheelEvent>
#include <algorithm>

namespace {
constexpr int kMargin = 18;       // top/bottom/side margin around the page column
constexpr int kSpacing = 14;      // gap between pages
constexpr int kMaxCache = 40;     // rendered pages kept in memory
constexpr double kZoomStep = 1.2;
constexpr double kMinZoom = 0.1;
constexpr double kMaxZoom = 8.0;
} // namespace

PageView::PageView(QWidget* parent) : QAbstractScrollArea(parent) {
    setFrameShape(QFrame::NoFrame);
    viewport()->setMouseTracking(true);
    verticalScrollBar()->setSingleStep(48);
    horizontalScrollBar()->setSingleStep(48);

    m_renderer = new QPdfPageRenderer(this);
    m_renderer->setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);
    connect(m_renderer, &QPdfPageRenderer::pageRendered, this,
            [this](int page, QSize size, const QImage& image, QPdfDocumentRenderOptions, quint64) {
                if (image.isNull() || size != pixelSize(page)) // drop stale (pre-zoom) results
                    return;
                m_pending.remove(page);
                QPixmap pm = QPixmap::fromImage(image);
                pm.setDevicePixelRatio(devicePixelRatioF());
                m_cache.insert(page, pm);
                m_lru.removeAll(page);
                m_lru.append(page);
                ++m_renderedCount;
                dropStaleCache();
                viewport()->update();
            });

    m_search = new QPdfSearchModel(this);
    auto emitCount = [this] {
        emit searchResultsChanged(searchResultCount());
        viewport()->update();
    };
    connect(m_search, &QAbstractItemModel::rowsInserted, this, emitCount);
    connect(m_search, &QAbstractItemModel::rowsRemoved, this, emitCount);
    connect(m_search, &QAbstractItemModel::modelReset, this, emitCount);
    connect(m_search, &QAbstractItemModel::layoutChanged, this, emitCount);
}

PageView::~PageView() = default;

void PageView::setDocument(QPdfDocument* doc) {
    m_doc = doc;
    m_cache.clear();
    m_lru.clear();
    m_pending.clear();
    m_currentPage = 0;
    m_currentResult = -1;
    m_renderedCount = 0;

    m_renderer->setDocument(doc);
    m_search->setSearchString(QString());
    m_search->setDocument(doc);

    // Read every page size once (cheap — no rendering). This is what lets the
    // whole layout be arithmetic, so opening is instant at any page count.
    m_pageSizes.clear();
    m_maxPageWPoints = 1.0;
    if (doc) {
        const int n = doc->pageCount();
        m_pageSizes.reserve(n);
        for (int i = 0; i < n; ++i) {
            const QSizeF s = doc->pagePointSize(i);
            m_pageSizes.append(s);
            m_maxPageWPoints = std::max(m_maxPageWPoints, s.width());
        }
    }

    relayout();
    verticalScrollBar()->setValue(0);
    emit zoomChanged(m_zoom);
    emit currentPageChanged(0);
    viewport()->update();
}

double PageView::contentWidthPx() const {
    return m_maxPageWPoints * m_zoom + 2 * kMargin;
}

void PageView::relayout() {
    const int n = pageCount();
    m_pageTop.resize(n);
    double y = kMargin;
    for (int i = 0; i < n; ++i) {
        m_pageTop[i] = y;
        y += pageHeightPx(i) + kSpacing;
    }
    const double contentH = (n > 0) ? (m_pageTop[n - 1] + pageHeightPx(n - 1) + kMargin) : 0.0;
    const double contentW = contentWidthPx();

    const int vpH = viewport()->height();
    const int vpW = viewport()->width();
    verticalScrollBar()->setRange(0, std::max(0, int(std::ceil(contentH)) - vpH));
    verticalScrollBar()->setPageStep(vpH);
    horizontalScrollBar()->setRange(0, std::max(0, int(std::ceil(contentW)) - vpW));
    horizontalScrollBar()->setPageStep(vpW);
}

int PageView::pageAtContentY(double y) const {
    const int n = pageCount();
    if (n == 0)
        return 0;
    // Largest i with m_pageTop[i] <= y.
    int lo = 0, hi = n - 1, ans = 0;
    while (lo <= hi) {
        const int mid = (lo + hi) / 2;
        if (m_pageTop[mid] <= y) {
            ans = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return ans;
}

QRect PageView::pageRect(int page) const {
    const double offY = verticalScrollBar()->value();
    const double offX = horizontalScrollBar()->value();
    const double pageW = pageWidthPx(page);
    const double pageH = pageHeightPx(page);
    const double centerSpace = std::max(contentWidthPx(), double(viewport()->width()));
    const double x = (centerSpace - pageW) / 2.0 - offX;
    const double y = m_pageTop[page] - offY;
    return QRect(qRound(x), qRound(y), qRound(pageW), qRound(pageH));
}

void PageView::updateCurrentPage() {
    if (pageCount() == 0)
        return;
    const double focusY = verticalScrollBar()->value() + viewport()->height() * 0.4;
    const int p = pageAtContentY(focusY);
    if (p != m_currentPage) {
        m_currentPage = p;
        emit currentPageChanged(p);
    }
}

QSize PageView::pixelSize(int page) const {
    const qreal dpr = devicePixelRatioF();
    return QSize(qRound(pageWidthPx(page) * dpr), qRound(pageHeightPx(page) * dpr));
}

void PageView::request(int page) {
    if (!m_doc || m_pending.contains(page) || m_cache.contains(page))
        return;
    m_pending.insert(page);
    m_renderer->requestPage(page, pixelSize(page));
}

void PageView::dropStaleCache() {
    while (m_lru.size() > kMaxCache) {
        const int victim = m_lru.takeFirst();
        m_cache.remove(victim);
    }
}

void PageView::paintEvent(QPaintEvent*) {
    const auto& pal = Theme::instance().palette();
    QPainter p(viewport());
    p.fillRect(viewport()->rect(), pal.sunken);

    if (!m_doc || pageCount() == 0)
        return;

    const double offY = verticalScrollBar()->value();
    const int vpH = viewport()->height();
    int first = pageAtContentY(offY);
    while (first > 0 && m_pageTop[first] > offY) // step back to catch a partial page
        --first;

    const QColor pageColor(255, 255, 255);
    for (int i = first; i < pageCount(); ++i) {
        const QRect r = pageRect(i);
        if (r.top() > vpH)
            break; // past the bottom of the viewport
        if (r.bottom() < 0)
            continue;

        // The page's soft shadow — the only prominent shadow (ui-guidelines §4).
        p.setPen(Qt::NoPen);
        for (int s = 6; s >= 1; --s) {
            p.setBrush(QColor(0, 0, 0, 6));
            p.drawRoundedRect(r.adjusted(-s, -s + 2, s, s + 2), 3, 3);
        }

        // Page background, then the rendered pixmap (or a blank page until ready).
        p.fillRect(r, pageColor);
        if (auto it = m_cache.constFind(i); it != m_cache.constEnd()) {
            p.drawPixmap(r, *it);
            m_lru.removeAll(i);
            m_lru.append(i);
        } else {
            request(i);
        }

        // Search highlights on this page (drawn here — there is no QPdfView).
        const QList<QPdfLink> results = m_search->resultsOnPage(i);
        if (!results.isEmpty()) {
            QColor hi = pal.accent;
            hi.setAlpha(70);
            p.setPen(Qt::NoPen);
            p.setBrush(hi);
            for (const QPdfLink& link : results)
                for (const QRectF& rp : link.rectangles())
                    p.drawRect(QRectF(r.x() + rp.x() * m_zoom, r.y() + rp.y() * m_zoom,
                                      rp.width() * m_zoom, rp.height() * m_zoom));
        }

        // Hairline page border.
        p.setBrush(Qt::NoBrush);
        p.setPen(pal.hairline);
        p.drawRect(r);
    }

    // The active match, emphasised.
    if (m_currentResult >= 0) {
        const QPdfLink link = m_search->resultAtIndex(m_currentResult);
        if (link.page() >= first - 1) {
            const QRect r = pageRect(link.page());
            QColor hi = pal.accent;
            hi.setAlpha(150);
            p.setBrush(hi);
            p.setPen(QPen(pal.accent, 1));
            for (const QRectF& rp : link.rectangles())
                p.drawRect(QRectF(r.x() + rp.x() * m_zoom, r.y() + rp.y() * m_zoom,
                                  rp.width() * m_zoom, rp.height() * m_zoom));
        }
    }

    if (m_hud) {
        const QString text =
            QStringLiteral("page %1/%2  zoom %3%  dpr %4  cache %5  pending %6  rendered %7")
                .arg(m_currentPage + 1)
                .arg(pageCount())
                .arg(qRound(m_zoom * 100))
                .arg(devicePixelRatioF(), 0, 'f', 2)
                .arg(m_cache.size())
                .arg(m_pending.size())
                .arg(m_renderedCount);
        QFont f("monospace", 9);
        p.setFont(f);
        const QRect box(8, 8, QFontMetrics(f).horizontalAdvance(text) + 16, 22);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 160));
        p.drawRoundedRect(box, 5, 5);
        p.setPen(QColor(255, 255, 255));
        p.drawText(box, Qt::AlignCenter, text);
    }
}

void PageView::resizeEvent(QResizeEvent*) {
    relayout();
    updateCurrentPage();
}

void PageView::scrollContentsBy(int, int) {
    updateCurrentPage();
    viewport()->update();
}

void PageView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const int dy = event->angleDelta().y();
        if (dy != 0)
            setZoom(m_zoom * (dy > 0 ? kZoomStep : 1.0 / kZoomStep));
        event->accept();
        return;
    }
    QAbstractScrollArea::wheelEvent(event);
}

void PageView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_D && (event->modifiers() & Qt::ControlModifier) &&
        (event->modifiers() & Qt::ShiftModifier)) {
        setHudVisible(!m_hud);
        return;
    }
    QAbstractScrollArea::keyPressEvent(event);
}

void PageView::goToPage(int page) {
    if (pageCount() == 0)
        return;
    page = std::clamp(page, 0, pageCount() - 1);
    verticalScrollBar()->setValue(qRound(m_pageTop[page] - kMargin));
}

void PageView::setZoom(double zoom) {
    zoom = std::clamp(zoom, kMinZoom, kMaxZoom);
    if (qFuzzyCompare(zoom, m_zoom))
        return;

    // Preserve the reading position: keep whatever is at the viewport top there.
    const double topY = verticalScrollBar()->value();
    const int anchor = pageAtContentY(topY);
    const double frac =
        pageHeightPx(anchor) > 0 ? (topY - m_pageTop[anchor]) / pageHeightPx(anchor) : 0.0;

    m_zoom = zoom;
    m_cache.clear(); // pixmaps are the wrong size now; stale renders self-drop
    m_lru.clear();
    m_pending.clear();
    relayout();

    if (anchor < pageCount())
        verticalScrollBar()->setValue(qRound(m_pageTop[anchor] + frac * pageHeightPx(anchor)));

    emit zoomChanged(m_zoom);
    updateCurrentPage();
    viewport()->update();
}

void PageView::zoomIn() {
    setZoom(m_zoom * kZoomStep);
}

void PageView::zoomOut() {
    setZoom(m_zoom / kZoomStep);
}

void PageView::zoomActualSize() {
    setZoom(1.0);
}

void PageView::fitToWidth() {
    if (pageCount() == 0)
        return;
    setZoom((viewport()->width() - 2.0 * kMargin) / m_maxPageWPoints);
}

void PageView::fitWholePage() {
    if (pageCount() == 0)
        return;
    const QSizeF s = m_pageSizes[m_currentPage];
    const double zW = (viewport()->width() - 2.0 * kMargin) / s.width();
    const double zH = (viewport()->height() - 2.0 * kMargin) / s.height();
    setZoom(std::min(zW, zH));
}

int PageView::search(const QString& query) {
    m_currentResult = -1;
    m_search->setSearchString(query);
    return searchResultCount();
}

void PageView::clearSearch() {
    m_currentResult = -1;
    m_search->setSearchString(QString());
    viewport()->update();
}

void PageView::showSearchResult(int index) {
    const int count = searchResultCount();
    if (count <= 0)
        return;
    m_currentResult = ((index % count) + count) % count;
    const QPdfLink link = m_search->resultAtIndex(m_currentResult);
    if (link.page() >= 0) {
        const double y = m_pageTop[link.page()] + link.location().y() * m_zoom - 40;
        verticalScrollBar()->setValue(qRound(std::max(0.0, y)));
    }
    viewport()->update();
}

int PageView::searchResultCount() const {
    return m_search->rowCount(QModelIndex());
}

void PageView::setHudVisible(bool on) {
    m_hud = on;
    viewport()->update();
}
