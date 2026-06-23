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

        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(16, 9, 16, 9);
        row->setSpacing(11);

        m_chip = new QLabel(this);
        m_chip->setObjectName("ToolChip");
        m_chip->setFixedSize(26, 26);
        m_chip->setAlignment(Qt::AlignCenter);
        row->addWidget(m_chip);

        auto* name = new QLabel(label, this);
        row->addWidget(name, 1);

        if (expandable) {
            m_chev = new QLabel(this);
            m_chev->setFixedSize(14, 14);
            m_chev->setAlignment(Qt::AlignCenter);
            row->addWidget(m_chev);
        }
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
    QLabel* m_chip = nullptr;
    QLabel* m_chev = nullptr;
};

ToolsPane::ToolsPane(QWidget* parent) : QWidget(parent) {
    setObjectName("ToolsPane");
    setFixedWidth(232);

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    auto* head = new QLabel(tr("TOOLS"), this);
    head->setObjectName("PaneHead");
    head->setContentsMargins(16, 14, 16, 8);
    col->addWidget(head);

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

    auto* customize = new QPushButton(tr("Customize tools"), this);
    customize->setObjectName("CustomizeTools");
    customize->setCursor(Qt::PointingHandCursor);
    connect(customize, &QPushButton::clicked, this, &ToolsPane::customizeRequested);
    col->addWidget(customize);

    refreshIcons();
    connect(&Theme::instance(), &Theme::changed, this, &ToolsPane::refreshIcons);
}

void ToolsPane::refreshIcons() {
    for (ToolRow* r : m_rows)
        r->refresh();
    // The footer's sliders icon is set on the button directly.
    if (auto* customize = findChild<QPushButton*>("CustomizeTools")) {
        customize->setIcon(Theme::instance().icon("sliders", Theme::instance().palette().dim));
        customize->setIconSize(QSize(14, 14));
    }
}

#include "ToolsPane.moc"
