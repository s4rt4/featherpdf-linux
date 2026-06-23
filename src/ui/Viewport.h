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

#include <QWidget>

class FeatherDocument;
class QPdfView;
class QPdfSearchModel;

// The document viewport: where the page floats on the canvas.
//
// M0 wraps QtPdf's QPdfView to validate the full stack end-to-end and ship a
// navigable, zoomable viewer immediately. The custom render service the saran
// notes call for — LRU cache, two-pass preview→full, render cancellation,
// integer device-pixel layout for fractional HiDPI, and a debug HUD — replaces
// the internals of this class in a later increment. Keeping that behind this
// interface means MainWindow never has to change when it does.
class Viewport : public QWidget {
    Q_OBJECT

public:
    explicit Viewport(QWidget* parent = nullptr);
    ~Viewport() override;

    void setDocument(FeatherDocument* doc);
    void clear();
    bool hasDocument() const;

    // Zoom — every path (buttons, Ctrl+wheel, fit) funnels through here so the
    // reading position is preserved consistently, per the saran notes.
    void zoomIn();
    void zoomOut();
    void zoomActualSize(); // 100%
    void fitToWidth();
    void fitWholePage();
    double zoomFactor() const;

    int currentPage() const;
    int pageCount() const;
    void goToPage(int page); // 0-based

    // Text search. The query highlights all matches on every page; the result
    // count updates live as the background search progresses (searchResults
    // Changed). showSearchResult cycles the highlighted/active match.
    int search(const QString& query); // sets the query, returns current count
    void clearSearch();
    void showSearchResult(int index); // wraps; navigates to and highlights it
    int searchResultCount() const;

signals:
    void zoomChanged(double factor);
    void currentPageChanged(int page); // 0-based
    void pageCountChanged(int count);
    void searchResultsChanged(int count);

private:
    void applyZoomFactor(double factor);
    void applyCanvasColor();

    FeatherDocument* m_doc = nullptr;
    QPdfView* m_view;
    QPdfSearchModel* m_searchModel = nullptr;

    static constexpr double kZoomStep = 1.2;
    static constexpr double kMinZoom = 0.1;
    static constexpr double kMaxZoom = 8.0;
};
