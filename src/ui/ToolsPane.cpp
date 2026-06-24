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

#include "ui/ToolsPane.h"

#include "ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

// One tool row: accent-tinted icon chip + label + optional expand chevron.
// Hover background comes from the #ToolsPane QSS. Clicking emits activated().
class ToolRow : public QWidget {
    Q_OBJECT

public:
    ToolRow(QString id, const QString& label, QString iconName, bool expandable, QWidget* parent)
        : QWidget(parent), m_id(std::move(id)), m_iconName(std::move(iconName)) {
        setObjectName("ToolRow");
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_StyledBackground, true);

        setToolTip(label); // shown when collapsed to icon-only

        m_row = new QHBoxLayout(this);
        m_row->setContentsMargins(16, 9, 16, 9);
        m_row->setSpacing(11);

        m_chip = new QLabel(this);
        m_chip->setObjectName("ToolChip");
        m_chip->setFixedSize(26, 26);
        m_chip->setAlignment(Qt::AlignCenter);
        m_row->addWidget(m_chip);

        m_name = new QLabel(label, this);
        m_row->addWidget(m_name, 1);

        if (expandable) {
            m_chev = new QLabel(this);
            m_chev->setFixedSize(14, 14);
            m_chev->setAlignment(Qt::AlignCenter);
            m_row->addWidget(m_chev);
        }
    }

    // Icon-only: hide the label and chevron, centre the chip.
    void setCompact(bool compact) {
        m_name->setVisible(!compact);
        if (m_chev)
            m_chev->setVisible(!compact);
        m_row->setContentsMargins(compact ? 13 : 16, 9, compact ? 13 : 16, 9);
    }

    void refresh() {
        const auto& pal = Theme::instance().palette();
        m_chip->setStyleSheet(
            QStringLiteral("background:%1; border-radius:7px;")
                .arg(pal.accentTint.name()));
        m_chip->setPixmap(Theme::instance().icon(m_iconName, pal.accent).pixmap(15, 15));
        if (m_chev)
            m_chev->setPixmap(Theme::instance().icon("chevron-right", pal.dim).pixmap(14, 14));
    }

    QString id() const { return m_id; }

signals:
    void activated(const QString& id);

protected:
    void mouseReleaseEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton && rect().contains(e->pos()))
            emit activated(m_id);
        QWidget::mouseReleaseEvent(e);
    }

private:
    QString m_id;
    QString m_iconName;
    QHBoxLayout* m_row = nullptr;
    QLabel* m_chip = nullptr;
    QLabel* m_name = nullptr;
    QLabel* m_chev = nullptr;
};

ToolsPane::ToolsPane(QWidget* parent) : QWidget(parent) {
    setObjectName("ToolsPane");
    setFixedWidth(232);

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    // Header: the TOOLS label and a collapse/expand toggle.
    auto* header = new QWidget(this);
    auto* headerRow = new QHBoxLayout(header);
    headerRow->setContentsMargins(16, 12, 8, 8);
    headerRow->setSpacing(0);
    m_head = new QLabel(tr("TOOLS"), header);
    m_head->setObjectName("PaneHead");
    headerRow->addWidget(m_head);
    headerRow->addStretch(1);
    m_toggle = new QToolButton(header);
    m_toggle->setObjectName("ToolsCollapse");
    m_toggle->setCursor(Qt::PointingHandCursor);
    m_toggle->setAutoRaise(true);
    m_toggle->setFixedSize(26, 26);
    m_toggle->setToolTip(tr("Collapse the tools pane"));
    connect(m_toggle, &QToolButton::clicked, this, [this] { setCollapsed(!m_collapsed); });
    headerRow->addWidget(m_toggle);
    col->addWidget(header);

    struct Def {
        const char* id;
        const char* label;
        const char* icon;
        bool expandable;
    };
    const Def defs[] = {
        {"export", QT_TR_NOOP("Export"), "export", true},
        {"create", QT_TR_NOOP("Create"), "create", true},
        {"edit", QT_TR_NOOP("Edit"), "edit", false},
        {"comment", QT_TR_NOOP("Comment"), "comment", false},
        {"combine", QT_TR_NOOP("Combine"), "combine", true},
        {"split", QT_TR_NOOP("Split"), "split", false},
        {"compare", QT_TR_NOOP("Compare"), "compare", false},
        {"optimize", QT_TR_NOOP("Optimize"), "optimize", false},
        {"cmyk", QT_TR_NOOP("RGB to CMYK"), "cmyk", false},
        {"watermark", QT_TR_NOOP("Watermark"), "watermark", false},
        {"bates", QT_TR_NOOP("Bates numbering"), "bates", false},
        {"protect", QT_TR_NOOP("Protect"), "lock", false},
        {"flatten", QT_TR_NOOP("Flatten"), "layers", false},
        {"organize", QT_TR_NOOP("Organize"), "organize", true},
        {"redact", QT_TR_NOOP("Redact"), "redact", false},
        {"sign", QT_TR_NOOP("Sign"), "sign", false},
    };
    for (const Def& d : defs) {
        auto* r = new ToolRow(d.id, tr(d.label), d.icon, d.expandable, this);
        connect(r, &ToolRow::activated, this, &ToolsPane::toolActivated);
        m_rows.append(r);
        col->addWidget(r);
    }

    col->addStretch(1);

    m_customize = new QPushButton(tr("Customize tools"), this);
    m_customize->setObjectName("CustomizeTools");
    m_customize->setCursor(Qt::PointingHandCursor);
    connect(m_customize, &QPushButton::clicked, this, &ToolsPane::customizeRequested);
    col->addWidget(m_customize);

    refreshIcons();
    connect(&Theme::instance(), &Theme::changed, this, &ToolsPane::refreshIcons);
}

void ToolsPane::setCollapsed(bool collapsed) {
    m_collapsed = collapsed;
    setFixedWidth(collapsed ? 52 : 232);
    m_head->setVisible(!collapsed);
    for (ToolRow* r : m_rows)
        r->setCompact(collapsed);
    m_customize->setText(collapsed ? QString() : tr("Customize tools"));
    m_customize->setToolTip(tr("Customize tools"));
    m_toggle->setToolTip(collapsed ? tr("Expand the tools pane") : tr("Collapse the tools pane"));
    refreshIcons(); // re-point the toggle chevron
}

void ToolsPane::refreshIcons() {
    const auto& pal = Theme::instance().palette();
    for (ToolRow* r : m_rows)
        r->refresh();
    if (m_customize) {
        m_customize->setIcon(Theme::instance().icon("sliders", pal.dim));
        m_customize->setIconSize(QSize(14, 14));
    }
    if (m_toggle) {
        // Pane sits on the right: chevron-right collapses, chevron-left expands.
        m_toggle->setIcon(
            Theme::instance().icon(m_collapsed ? "chevron-left" : "chevron-right", pal.dim));
        m_toggle->setIconSize(QSize(15, 15));
    }
}

#include "ToolsPane.moc"
