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

#include "ui/Theme.h"

#include <QApplication>
#include <QGuiApplication>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QStyleHints>
#include <QSvgRenderer>

namespace {

// Blend `over` at `alpha` (0..1) on top of `base` — used to bake the
// "color-mix(accent 12%, transparent)" soft fills the mockup uses for selected
// rows into an opaque token, so they look right on any surface.
QColor blend(const QColor& base, const QColor& over, double alpha) {
    return QColor::fromRgbF(base.redF() * (1 - alpha) + over.redF() * alpha,
                            base.greenF() * (1 - alpha) + over.greenF() * alpha,
                            base.blueF() * (1 - alpha) + over.blueF() * alpha);
}

// A QSS-safe color literal, preserving alpha when present.
QString css(const QColor& c) {
    if (c.alpha() == 255)
        return c.name(QColor::HexRgb);
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(QString::number(c.alphaF(), 'f', 3));
}

} // namespace

Theme& Theme::instance() {
    static Theme theme;
    return theme;
}

Theme::Theme(QObject* parent) : QObject(parent) {}

Theme::Palette Theme::paletteFor(Mode mode) {
    Palette p;
    if (mode == Mode::Light) {
        p.canvas = QColor(0xF6, 0xF5, 0xF4);
        p.surface = QColor(0xFF, 0xFF, 0xFF);
        p.sunken = QColor(0xEC, 0xEA, 0xE8);
        p.text = QColor(0x24, 0x1F, 0x31);
        p.dim = QColor(0x5E, 0x5C, 0x64);
        p.hairline = QColor(0, 0, 0, 26); // rgba(0,0,0,.10)
        p.accent = QColor(0x11, 0x7C, 0x6F);
        p.accentHover = QColor(0x0E, 0x6A, 0x5E);
        p.warning = QColor(0xE5, 0xA5, 0x0A);
        p.destructive = QColor(0xC0, 0x1C, 0x28);
        p.success = QColor(0x2E, 0xC2, 0x7E);
    } else {
        p.canvas = QColor(0x24, 0x24, 0x24);
        p.surface = QColor(0x30, 0x30, 0x30);
        p.sunken = QColor(0x16, 0x16, 0x16);
        p.text = QColor(0xFF, 0xFF, 0xFF);
        p.dim = QColor(0xA8, 0xA8, 0xA8);
        p.hairline = QColor(255, 255, 255, 31); // rgba(255,255,255,.12)
        p.accent = QColor(0x2D, 0xB3, 0x9E);
        p.accentHover = QColor(0x48, 0xC7, 0xB3);
        p.warning = QColor(0xF8, 0xC4, 0x4F);
        p.destructive = QColor(0xFF, 0x7B, 0x7B);
        p.success = QColor(0x7B, 0xE0, 0xB0);
    }
    // 12% accent over the surface — the soft selected-row fill (mockup .rbtn.on / .tool .ic).
    p.accentTint = blend(p.surface, p.accent, 0.12);
    return p;
}

void Theme::apply() {
    // Follow the system light/dark preference (ui-guidelines §2: never ignore it).
    auto* hints = QGuiApplication::styleHints();
    auto resolveFromSystem = [hints]() -> Mode {
        return hints->colorScheme() == Qt::ColorScheme::Dark ? Mode::Dark : Mode::Light;
    };
    if (m_followSystem)
        m_mode = resolveFromSystem();

    connect(hints, &QStyleHints::colorSchemeChanged, this, [this, resolveFromSystem](Qt::ColorScheme) {
        if (!m_followSystem)
            return;
        const Mode next = resolveFromSystem();
        if (next != m_mode) {
            m_mode = next;
            rebuild();
        }
    });

    rebuild();
}

void Theme::setMode(Mode mode) {
    m_followSystem = false; // explicit choice overrides the system until next apply()
    if (mode == m_mode) {
        m_palette = paletteFor(m_mode); // ensure populated on first call
        return;
    }
    m_mode = mode;
    rebuild();
}

void Theme::toggleMode() {
    setMode(m_mode == Mode::Light ? Mode::Dark : Mode::Light);
}

void Theme::rebuild() {
    m_palette = paletteFor(m_mode);
    if (auto* app = qobject_cast<QApplication*>(QApplication::instance()))
        app->setStyleSheet(styleSheet());
    emit changed();
}

QString Theme::styleSheet() const {
    const Palette& p = m_palette;

    // One global sheet, built from tokens. Custom shell widgets opt in by object
    // name (set in their constructors). Kept close to feather-mockup.html.
    QString qss;

    // Base surfaces and text.
    qss += QStringLiteral(
               "QMainWindow, QWidget#Shell { background:%1; }"
               "QWidget { color:%2; font-size:13px; }"
               "QToolTip { background:%3; color:%2; border:1px solid %4;"
               " border-radius:6px; padding:4px 7px; }")
               .arg(css(p.canvas), css(p.text), css(p.surface), css(p.hairline));

    // Menu bar — flat surface, hairline underline, soft hover (mockup .menu).
    qss += QStringLiteral(
               "QMenuBar { background:%1; border-bottom:1px solid %2; padding:2px 6px; }"
               "QMenuBar::item { background:transparent; color:%3; padding:3px 9px;"
               " border-radius:6px; }"
               "QMenuBar::item:selected { background:%4; color:%5; }")
               .arg(css(p.surface), css(p.hairline), css(p.dim), css(p.canvas), css(p.text));

    // Menus / popovers — surface card, hairline, accent-tint selection.
    qss += QStringLiteral(
               "QMenu { background:%1; border:1px solid %2; border-radius:12px; padding:6px; }"
               "QMenu::item { padding:6px 28px 6px 12px; border-radius:8px; color:%3; }"
               "QMenu::item:selected { background:%4; color:%5; }"
               "QMenu::item:disabled { color:%6; }"
               "QMenu::separator { height:1px; background:%2; margin:6px 8px; }")
               .arg(css(p.surface), css(p.hairline), css(p.text), css(p.accentTint),
                    css(p.text), css(p.dim));

    // Slim, calm scrollbars — chrome recedes (ui-guidelines §1).
    qss += QStringLiteral(
               "QScrollBar:vertical { background:transparent; width:12px; margin:0; }"
               "QScrollBar::handle:vertical { background:%1; border-radius:5px; min-height:32px;"
               " margin:2px; }"
               "QScrollBar::handle:vertical:hover { background:%2; }"
               "QScrollBar:horizontal { background:transparent; height:12px; margin:0; }"
               "QScrollBar::handle:horizontal { background:%1; border-radius:5px; min-width:32px;"
               " margin:2px; }"
               "QScrollBar::handle:horizontal:hover { background:%2; }"
               "QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0; }"
               "QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }")
               .arg(css(p.dim.lighter(140)), css(p.dim));

    // ── Shell regions (object-name scoped) ───────────────────────────────────

    // Command toolbar — surface, hairline underline, connects to the active tab.
    qss += QStringLiteral(
               "#CommandBar { background:%1; border-bottom:1px solid %2; }"
               "#CommandBar QToolButton { background:transparent; border:none;"
               " border-radius:7px; padding:0; }"
               "#CommandBar QToolButton:hover { background:%3; }"
               "#CommandBar QToolButton:pressed { background:%4; }"
               "#CommandBar QLabel#Counter, #CommandBar QLabel#Zoom {"
               " font-family:'Source Code Pro',monospace; font-size:13px; color:%5; }")
               .arg(css(p.surface), css(p.hairline), css(p.canvas), css(p.hairline), css(p.text));

    // Tab strip right-side controls (search · theme · menu).
    qss += QStringLiteral(
               "#TabStrip QToolButton#TabStripButton { background:transparent; border:none;"
               " border-radius:7px; }"
               "#TabStrip QToolButton#TabStripButton:hover { background:%1; }")
               .arg(css(p.hairline));

    // Hairline separators inside bars.
    qss += QStringLiteral("#CommandBar QFrame#Sep, #ToolsPane QFrame#Sep {"
                          " background:%1; border:none; }")
               .arg(css(p.hairline));

    // The single accent-filled action — Share (ui-guidelines §7).
    qss += QStringLiteral(
               "QPushButton#Share { background:%1; color:#FFFFFF; border:none;"
               " border-radius:8px; padding:7px 15px; font-weight:600; }"
               "QPushButton#Share:hover { background:%2; }")
               .arg(css(p.accent), css(p.accentHover));

    // Left navigation rail — surface with hairline edge; active button is accent.
    qss += QStringLiteral(
               "#NavigationRail { background:%1; border-right:1px solid %2; }"
               "#NavigationRail QToolButton { background:transparent; border:none;"
               " border-radius:8px; padding:0; color:%3; }"
               "#NavigationRail QToolButton:hover { background:%4; }"
               "#NavigationRail QToolButton:checked { background:%5; }")
               .arg(css(p.surface), css(p.hairline), css(p.dim), css(p.canvas), css(p.accentTint));

    // Right Tools pane — surface with hairline edge.
    qss += QStringLiteral(
               "#ToolsPane { background:%1; border-left:1px solid %2; }"
               "#ToolsPane QLabel#PaneHead { color:%3; font-size:11px; font-weight:700; }"
               "#ToolsPane QPushButton.toolrow { background:transparent; border:none;"
               " text-align:left; padding:0; color:%4; border-radius:8px; }"
               "#ToolsPane QPushButton.toolrow:hover { background:%5; }"
               "#ToolsPane QPushButton#CustomizeTools { background:transparent; border:none;"
               " text-align:left; color:%3; padding:11px 16px; }"
               "#ToolsPane QPushButton#CustomizeTools:hover { color:%4; }")
               .arg(css(p.surface), css(p.hairline), css(p.dim), css(p.text), css(p.canvas));

    // Left rail expandable panel (Thumbnails / Outline / …).
    qss += QStringLiteral(
               "#RailPanel { background:%1; border-right:1px solid %2; }"
               "#RailPanelHead { color:%3; font-size:11px; font-weight:700; letter-spacing:.5px; }"
               "#RailPanelPlaceholder { color:%3; font-size:13px; }"
               "#ThumbnailList { background:transparent; }"
               "#ThumbnailList::item:hover { background:%4; }")
               .arg(css(p.surface), css(p.hairline), css(p.dim), css(p.canvas));

    // Home start screen — canvas backdrop, big suggested action, recent cards.
    qss += QStringLiteral(
               "#HomeView { background:%1; }"
               "QPushButton#HomePrimary { background:%2; color:#FFFFFF; border:none;"
               " border-radius:10px; padding:11px 22px; font-size:14px; font-weight:600; }"
               "QPushButton#HomePrimary:hover { background:%3; }"
               "#RecentCard { background:%4; border:1px solid %5; border-radius:12px; }"
               "#RecentCard:hover { border:1px solid %2; }")
               .arg(css(p.canvas), css(p.accent), css(p.accentHover), css(p.surface),
                    css(p.hairline));

    return qss;
}

QIcon Theme::icon(const QString& name, QColor color) const {
    if (!color.isValid())
        color = m_palette.text;

    // Cache keyed by name+color+dpr so repeated lookups are cheap.
    static QHash<QString, QIcon> cache;
    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    const QString key = name + '|' + color.name(QColor::HexArgb) + '|' + QString::number(dpr, 'f', 2);
    if (auto it = cache.constFind(key); it != cache.constEnd())
        return *it;

    // Render the lucide SVG (strokes use currentColor → render as a mask, then
    // fill with the requested token). Generous base size keeps small draws crisp.
    constexpr int kBase = 64;
    const QString path = QStringLiteral(":/icons/%1.svg").arg(name);
    QSvgRenderer renderer(path);

    QPixmap pm(QSize(kBase, kBase) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    if (renderer.isValid()) {
        QPainter rp(&pm);
        renderer.render(&rp, QRectF(0, 0, kBase, kBase));
        rp.end();

        QPainter tint(&pm);
        tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
        tint.fillRect(pm.rect(), color);
        tint.end();
    }

    QIcon ic(pm);
    cache.insert(key, ic);
    return ic;
}

QPixmap Theme::brandLogo(int size) const {
    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    QSvgRenderer renderer(QStringLiteral(":/icons/feather-logo.svg"));
    QPixmap pm(QSize(size, size) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    if (renderer.isValid()) {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        renderer.render(&p, QRectF(0, 0, size, size));
    }
    return pm;
}
