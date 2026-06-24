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

#include <QAbstractScrollArea>
#include <QHash>
#include <QList>
#include <QSet>
#include <QSizeF>

class QPdfDocument;
class QPdfPageRenderer;
class QPdfSearchModel;

// The custom continuous page view — the render pipeline (saran notes; replaces
// QPdfView's internals).
//
// Why custom: QPdfView lays out *every* page synchronously on setDocument, which
// freezes the UI (and the compositor) on a 4000-page document. PageView instead
// computes the whole layout *arithmetically* from cached page sizes — opening is
// instant regardless of page count — and renders only on-screen pages, off the
// UI thread (QPdfPageRenderer), into an LRU cache. Scrolling never blocks.
//
// It is a QAbstractScrollArea: geometry is points × zoom, the active page is
// found by binary-searching cumulative offsets (never by reading widget
// geometry), and zoom preserves the reading position. Search matches are painted
// here too, since there is no QPdfView to do it.
class PageView : public QAbstractScrollArea {
    Q_OBJECT

public:
    // How pages are arranged in the viewport.
    enum class LayoutMode { SinglePage, Continuous, TwoUp };

    explicit PageView(QWidget* parent = nullptr);
    ~PageView() override;

    void setDocument(QPdfDocument* doc);

    // The page arrangement from the façade: order[i] is the original page shown
    // in display slot i, rot[i] its rotation. setRotation updates one slot.
    void setArrangement(const QVector<int>& order, const QVector<int>& rotations);
    void setRotation(int slot, int degrees);

    int pageCount() const { return m_order.size(); } // display slots
    int currentPage() const { return m_currentPage; }
    void goToPage(int page); // 0-based, scroll to its top

    LayoutMode layoutMode() const { return m_mode; }
    void setLayoutMode(LayoutMode mode);

    double zoom() const { return m_zoom; }
    void setZoom(double zoom);          // preserves the reading position
    void zoomIn();
    void zoomOut();
    void zoomActualSize();
    void fitToWidth();
    void fitWholePage();

    // Search (painted here). The model is owned by the view.
    int search(const QString& query);  // sets query, returns current count
    void clearSearch();
    void showSearchResult(int index);  // wraps; scrolls to and emphasizes it
    int searchResultCount() const;

    void setHudVisible(bool on);
    bool hudVisible() const { return m_hud; }

    // Redaction: drag rectangles over content to mark it for removal. Marks are
    // stored as fractions of each DISPLAYED page (so rotation needs no special
    // case — the apply step renders the page as shown and paints the fractions).
    void setRedactionMode(bool on);
    bool redactionMode() const { return m_redactMode; }
    // slot → normalized rects (each in [0,1] of the displayed page).
    QHash<int, QList<QRectF>> redactionMarks() const { return m_redactions; }
    int redactionCount() const; // total rectangles across all pages
    void clearRedactions();

signals:
    void currentPageChanged(int page);
    void zoomChanged(double zoom);
    void searchResultsChanged(int count);
    void layoutModeChanged(LayoutMode mode);
    void redactionsChanged(int count);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override; // viewport mouse in redact mode

private:
    // Layout (all in logical px unless noted). Pages are grouped into rows — one
    // slot per row for single/continuous, two per row for facing. Sizes are the
    // *effective* size, with width/height swapped for 90°/270° rotation. Indices
    // below are DISPLAY SLOTS; the page they render is m_order[slot].
    int originalOf(int slot) const {
        return (slot >= 0 && slot < m_order.size()) ? m_order[slot] : -1;
    }
    int rotationOf(int slot) const {
        return (slot >= 0 && slot < m_rot.size()) ? m_rot[slot] : 0;
    }
    const QSizeF& sizeOf(int slot) const {
        static const QSizeF kFallback(1, 1);
        const int o = originalOf(slot);
        return (o >= 0 && o < m_pageSizes.size()) ? m_pageSizes[o] : kFallback;
    }
    double pageWidthPx(int slot) const {
        const int r = rotationOf(slot);
        const QSizeF& s = sizeOf(slot);
        return ((r == 90 || r == 270) ? s.height() : s.width()) * m_zoom;
    }
    double pageHeightPx(int slot) const {
        const int r = rotationOf(slot);
        const QSizeF& s = sizeOf(slot);
        return ((r == 90 || r == 270) ? s.width() : s.height()) * m_zoom;
    }
    void buildRows();
    double rowHeightPx(int row) const;
    double rowContentWidthPx(int row) const; // sum of page widths + inner gap
    double contentWidthPx() const;
    void relayout();              // rebuild rows + cumulative tops + scrollbars
    int rowAtContentY(double y) const;  // binary search on m_rowTop
    QRect pageRect(int page) const;     // page rect in viewport coordinates
    void updateCurrentPage();

    // Rendering.
    void request(int page);
    void dropStaleCache();        // keep the cache near the viewport, bounded
    // Mark renders stale. dropCache=false keeps the old pixmaps so they can be
    // drawn scaled as a preview (no white flash on zoom) until the crisp render
    // at the new size arrives.
    void invalidateRenders(bool dropCache = true);
    QSize pixelSize(int page) const;

    QPdfDocument* m_doc = nullptr;
    QPdfPageRenderer* m_renderer = nullptr;
    QPdfSearchModel* m_search = nullptr;
    int m_currentResult = -1;

    QList<QSizeF> m_pageSizes;    // points, per ORIGINAL page (unrotated)
    QVector<int> m_order;         // display slot → original page index
    QVector<int> m_rot;           // display slot → rotation (degrees clockwise)
    QList<QList<int>> m_rows;     // display slots grouped into rows
    QList<int> m_pageRow;         // page → row index
    QList<double> m_rowTop;       // cumulative row top offset, logical px
    double m_maxPageWPoints = 1.0;
    double m_maxRowWPoints = 1.0; // widest row (two pages, for fit-width)
    double m_zoom = 1.0;
    int m_currentPage = 0;
    LayoutMode m_mode = LayoutMode::Continuous;

    QHash<int, QPixmap> m_cache;  // page → rendered pixmap at the current zoom
    QList<int> m_lru;             // most-recent at back
    QSet<int> m_pending;          // in-flight render requests
    quint64 m_gen = 0;            // render generation; bumped on zoom/rotation
    QHash<quint64, quint64> m_reqGen;  // request id → generation it was made in
    QHash<quint64, int> m_reqSlot;     // request id → display slot it was for

    bool m_hud = false;
    int m_renderedCount = 0;      // HUD: total renders delivered

    // Redaction state.
    QPointF normInSlot(int slot, const QPoint& vpPt) const; // viewport px → [0,1]×[0,1]
    bool m_redactMode = false;
    QHash<int, QList<QRectF>> m_redactions; // slot → normalized rects
    bool m_dragging = false;
    int m_dragSlot = -1;
    QPointF m_dragStart;          // normalized start point
    QRectF m_dragNorm;            // current drag rect, normalized
};
