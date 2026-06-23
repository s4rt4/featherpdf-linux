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

    int pageCount() const { return m_pageSizes.size(); }
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

signals:
    void currentPageChanged(int page);
    void zoomChanged(double zoom);
    void searchResultsChanged(int count);
    void layoutModeChanged(LayoutMode mode);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    // Layout (all in logical px unless noted). Pages are grouped into rows — one
    // page per row for single/continuous, two per row for facing.
    double pageWidthPx(int page) const { return m_pageSizes[page].width() * m_zoom; }
    double pageHeightPx(int page) const { return m_pageSizes[page].height() * m_zoom; }
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
    QSize pixelSize(int page) const;

    QPdfDocument* m_doc = nullptr;
    QPdfPageRenderer* m_renderer = nullptr;
    QPdfSearchModel* m_search = nullptr;
    int m_currentResult = -1;

    QList<QSizeF> m_pageSizes;    // points, per page
    QList<QList<int>> m_rows;     // page indices grouped into rows
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

    bool m_hud = false;
    int m_renderedCount = 0;      // HUD: total renders delivered
};
