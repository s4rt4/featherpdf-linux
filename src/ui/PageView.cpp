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
#include <QPdfDocumentRenderOptions>
#include <QPdfLink>
#include <QPdfPageRenderer>
#include <QPdfSearchModel>
#include <QScrollBar>
#include <QWheelEvent>
#include <algorithm>

namespace {
constexpr int kMargin = 18;       // top/bottom/side margin around the page column
constexpr int kSpacing = 14;      // gap between rows
constexpr int kPairGap = 4;       // gap between the two pages of a facing row
constexpr int kMaxCache = 48;     // rendered pages kept in memory
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
            [this](int, QSize, const QImage& image, QPdfDocumentRenderOptions, quint64 id) {
                // Drop results from a superseded generation (zoom/rotation/edit).
                const quint64 g = m_reqGen.take(id);
                const int slot = m_reqSlot.take(id);
                if (image.isNull() || g != m_gen || slot < 0)
                    return;
                m_pending.remove(slot);
                QPixmap pm = QPixmap::fromImage(image);
                pm.setDevicePixelRatio(devicePixelRatioF());
                m_cache.insert(slot, pm);
                m_lru.removeAll(slot);
                m_lru.append(slot);
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
    m_reqGen.clear();
    ++m_gen;
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
    // Identity arrangement until the façade pushes its edit model.
    const int n = m_pageSizes.size();
    m_order.resize(n);
    for (int i = 0; i < n; ++i)
        m_order[i] = i;
    m_rot.assign(n, 0);

    relayout();
    verticalScrollBar()->setValue(0);
    emit zoomChanged(m_zoom);
    emit currentPageChanged(0);
    viewport()->update();
}

void PageView::buildRows() {
    m_rows.clear();
    const int n = pageCount();
    m_pageRow.assign(n, -1);
    if (n == 0) {
        m_maxRowWPoints = 1.0;
        return;
    }

    if (m_mode == LayoutMode::TwoUp) {
        for (int i = 0; i < n; i += 2) {
            QList<int> row{i};
            if (i + 1 < n)
                row.append(i + 1);
            m_rows.append(row);
        }
    } else if (m_mode == LayoutMode::SinglePage) {
        m_rows.append(QList<int>{std::clamp(m_currentPage, 0, n - 1)});
    } else { // Continuous
        for (int i = 0; i < n; ++i)
            m_rows.append(QList<int>{i});
    }

    m_maxRowWPoints = 1.0;
    for (int r = 0; r < m_rows.size(); ++r) {
        double w = 0;
        for (int slot : m_rows[r]) {
            m_pageRow[slot] = r;
            const int rot = rotationOf(slot);
            const QSizeF& s = sizeOf(slot);
            w += (rot == 90 || rot == 270) ? s.height() : s.width(); // effective width
        }
        m_maxRowWPoints = std::max(m_maxRowWPoints, w);
    }
}

double PageView::rowHeightPx(int row) const {
    double h = 0;
    for (int p : m_rows[row])
        h = std::max(h, pageHeightPx(p));
    return h;
}

double PageView::rowContentWidthPx(int row) const {
    double w = 0;
    for (int p : m_rows[row])
        w += pageWidthPx(p);
    return w + (m_rows[row].size() - 1) * kPairGap;
}

double PageView::contentWidthPx() const {
    return m_maxRowWPoints * m_zoom + (kPairGap + 2 * kMargin);
}

void PageView::relayout() {
    buildRows();
    const int rows = m_rows.size();
    m_rowTop.resize(rows);
    double y = kMargin;
    for (int r = 0; r < rows; ++r) {
        m_rowTop[r] = y;
        y += rowHeightPx(r) + kSpacing;
    }
    const double contentH = (rows > 0) ? (m_rowTop[rows - 1] + rowHeightPx(rows - 1) + kMargin) : 0.0;
    const double contentW = contentWidthPx();

    const int vpH = viewport()->height();
    const int vpW = viewport()->width();
    verticalScrollBar()->setRange(0, std::max(0, int(std::ceil(contentH)) - vpH));
    verticalScrollBar()->setPageStep(vpH);
    horizontalScrollBar()->setRange(0, std::max(0, int(std::ceil(contentW)) - vpW));
    horizontalScrollBar()->setPageStep(vpW);
}

int PageView::rowAtContentY(double y) const {
    const int n = m_rows.size();
    if (n == 0)
        return 0;
    int lo = 0, hi = n - 1, ans = 0;
    while (lo <= hi) {
        const int mid = (lo + hi) / 2;
        if (m_rowTop[mid] <= y) {
            ans = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return ans;
}

QRect PageView::pageRect(int page) const {
    if (page < 0 || page >= m_pageRow.size())
        return {};
    const int r = m_pageRow[page];
    if (r < 0)
        return {};
    const double offY = verticalScrollBar()->value();
    const double offX = horizontalScrollBar()->value();
    const double centerSpace = std::max(contentWidthPx(), double(viewport()->width()));
    double x = (centerSpace - rowContentWidthPx(r)) / 2.0 - offX;
    for (int p : m_rows[r]) {
        const double pw = pageWidthPx(p);
        if (p == page)
            return QRect(qRound(x), qRound(m_rowTop[r] - offY), qRound(pw), qRound(pageHeightPx(p)));
        x += pw + kPairGap;
    }
    return {};
}

void PageView::updateCurrentPage() {
    if (m_rows.isEmpty())
        return;
    const double focusY = verticalScrollBar()->value() + viewport()->height() * 0.4;
    const int r = rowAtContentY(focusY);
    const int p = m_rows[r].isEmpty() ? 0 : m_rows[r].first();
    if (p != m_currentPage) {
        m_currentPage = p;
        emit currentPageChanged(p);
    }
}

QSize PageView::pixelSize(int page) const {
    const qreal dpr = devicePixelRatioF();
    return QSize(qRound(pageWidthPx(page) * dpr), qRound(pageHeightPx(page) * dpr));
}

void PageView::request(int slot) {
    if (!m_doc || m_pending.contains(slot) || m_cache.contains(slot))
        return;
    const int orig = originalOf(slot);
    if (orig < 0)
        return;
    m_pending.insert(slot);
    QPdfDocumentRenderOptions opts;
    switch (rotationOf(slot)) {
    case 90: opts.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise90); break;
    case 180: opts.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise180); break;
    case 270: opts.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise270); break;
    default: break;
    }
    const quint64 id = m_renderer->requestPage(orig, pixelSize(slot), opts);
    m_reqGen.insert(id, m_gen);
    m_reqSlot.insert(id, slot);
}

void PageView::dropStaleCache() {
    while (m_lru.size() > kMaxCache) {
        const int victim = m_lru.takeFirst();
        m_cache.remove(victim);
    }
}

void PageView::invalidateRenders() {
    // Everything in flight or cached is now wrong (zoom, rotation, or edit).
    ++m_gen;
    m_cache.clear();
    m_lru.clear();
    m_pending.clear();
    m_reqGen.clear();
    m_reqSlot.clear();
}

void PageView::setArrangement(const QVector<int>& order, const QVector<int>& rotations) {
    m_order = order;
    m_rot = rotations;
    m_rot.resize(m_order.size());
    m_currentPage = std::clamp(m_currentPage, 0, std::max(0, pageCount() - 1));
    invalidateRenders();
    relayout();
    updateCurrentPage();
    viewport()->update();
}

void PageView::setRotation(int slot, int degrees) {
    if (slot < 0 || slot >= m_rot.size())
        return;
    m_rot[slot] = ((degrees % 360) + 360) % 360;
    invalidateRenders(); // the slot's pixmap is now wrong
    relayout();
    viewport()->update();
}

void PageView::paintEvent(QPaintEvent*) {
    const auto& pal = Theme::instance().palette();
    QPainter p(viewport());
    p.fillRect(viewport()->rect(), pal.sunken);

    if (!m_doc || m_rows.isEmpty())
        return;

    const double offY = verticalScrollBar()->value();
    const double offX = horizontalScrollBar()->value();
    const int vpH = viewport()->height();
    const double centerSpace = std::max(contentWidthPx(), double(viewport()->width()));

    auto drawPage = [&](const QRect& r, int slot) {
        const int page = originalOf(slot); // search/highlights are keyed by the original page
        // The page's soft shadow — the only prominent shadow (ui-guidelines §4).
        p.setPen(Qt::NoPen);
        for (int s = 6; s >= 1; --s) {
            p.setBrush(QColor(0, 0, 0, 6));
            p.drawRoundedRect(r.adjusted(-s, -s + 2, s, s + 2), 3, 3);
        }
        p.fillRect(r, QColor(255, 255, 255));
        if (auto it = m_cache.constFind(slot); it != m_cache.constEnd()) {
            p.drawPixmap(r, *it);
            m_lru.removeAll(slot);
            m_lru.append(slot);
        } else {
            request(slot);
        }
        const QList<QPdfLink> results = m_search->resultsOnPage(page);
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
        p.setBrush(Qt::NoBrush);
        p.setPen(pal.hairline);
        p.drawRect(r);
    };

    int first = rowAtContentY(offY);
    while (first > 0 && m_rowTop[first] > offY)
        --first;

    for (int r = first; r < m_rows.size(); ++r) {
        const double rowYtop = m_rowTop[r] - offY;
        if (rowYtop > vpH)
            break;
        if (rowYtop + rowHeightPx(r) < 0)
            continue;
        double x = (centerSpace - rowContentWidthPx(r)) / 2.0 - offX;
        for (int page : m_rows[r]) {
            const QRect rect(qRound(x), qRound(rowYtop), qRound(pageWidthPx(page)),
                             qRound(pageHeightPx(page)));
            drawPage(rect, page);
            x += pageWidthPx(page) + kPairGap;
        }
    }

    // The active match, emphasised. link.page() is an original page; find the
    // display slot that shows it.
    if (m_currentResult >= 0) {
        const QPdfLink link = m_search->resultAtIndex(m_currentResult);
        const int slot = m_order.indexOf(link.page());
        const QRect r = slot >= 0 ? pageRect(slot) : QRect();
        if (!r.isNull()) {
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
    if (m_mode == LayoutMode::SinglePage) {
        m_currentPage = page;
        relayout();
        verticalScrollBar()->setValue(0);
        emit currentPageChanged(page);
        viewport()->update();
        return;
    }
    const int r = m_pageRow[page];
    verticalScrollBar()->setValue(qRound(m_rowTop[r] - kMargin));
}

void PageView::setLayoutMode(LayoutMode mode) {
    if (mode == m_mode)
        return;
    const int keep = m_currentPage;
    m_mode = mode;
    relayout();
    goToPage(keep);
    emit layoutModeChanged(mode);
    viewport()->update();
}

void PageView::setZoom(double zoom) {
    zoom = std::clamp(zoom, kMinZoom, kMaxZoom);
    if (qFuzzyCompare(zoom, m_zoom))
        return;

    // Preserve the reading position: keep whatever row sits at the viewport top.
    const double topY = verticalScrollBar()->value();
    const int r = rowAtContentY(topY);
    const double frac = rowHeightPx(r) > 0 ? (topY - m_rowTop[r]) / rowHeightPx(r) : 0.0;

    m_zoom = zoom;
    invalidateRenders(); // pixmaps are the wrong size now
    relayout();

    if (r < m_rowTop.size())
        verticalScrollBar()->setValue(qRound(m_rowTop[r] + frac * rowHeightPx(r)));

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
    setZoom((viewport()->width() - 2.0 * kMargin) / m_maxRowWPoints);
}

void PageView::fitWholePage() {
    if (pageCount() == 0)
        return;
    const QSizeF s = sizeOf(m_currentPage);
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
    const int slot = m_order.indexOf(link.page()); // original page → display slot
    if (slot >= 0) {
        if (m_mode == LayoutMode::SinglePage) {
            m_currentPage = slot;
            relayout();
            emit currentPageChanged(m_currentPage);
        }
        const int r = (slot < m_pageRow.size()) ? m_pageRow[slot] : 0;
        const double y = m_rowTop[std::max(0, r)] + link.location().y() * m_zoom - 40;
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
