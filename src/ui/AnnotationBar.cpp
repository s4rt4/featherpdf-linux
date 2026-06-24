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

#include "ui/AnnotationBar.h"

#include "ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

AnnotationBar::AnnotationBar(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("AnnotationBar"));

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(14, 7, 12, 7);
    row->setSpacing(10);

    m_highlightTool = new QPushButton(tr("Highlight"), this);
    m_noteTool = new QPushButton(tr("Note"), this);
    for (QPushButton* b : {m_highlightTool, m_noteTool}) {
        b->setObjectName(QStringLiteral("GhostBtn"));
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
    }
    m_highlightTool->setChecked(true);

    m_label = new QLabel(this);
    m_label->setObjectName(QStringLiteral("AnnotLabel"));

    m_save = new QPushButton(tr("Save Annotations"), this);
    m_save->setObjectName(QStringLiteral("Share")); // accent-filled primary
    m_save->setCursor(Qt::PointingHandCursor);

    m_clear = new QPushButton(tr("Clear"), this);
    m_clear->setObjectName(QStringLiteral("GhostBtn"));
    m_clear->setCursor(Qt::PointingHandCursor);

    auto* done = new QPushButton(tr("Done"), this);
    done->setObjectName(QStringLiteral("GhostBtn"));
    done->setCursor(Qt::PointingHandCursor);

    row->addWidget(m_highlightTool);
    row->addWidget(m_noteTool);
    row->addSpacing(6);
    row->addWidget(m_label);
    row->addStretch(1);
    row->addWidget(m_clear);
    row->addWidget(m_save);
    row->addWidget(done);

    connect(m_save, &QPushButton::clicked, this, &AnnotationBar::saveRequested);
    connect(m_clear, &QPushButton::clicked, this, &AnnotationBar::clearRequested);
    connect(done, &QPushButton::clicked, this, &AnnotationBar::doneRequested);
    connect(m_highlightTool, &QPushButton::clicked, this, [this] {
        m_highlightTool->setChecked(true);
        m_noteTool->setChecked(false);
        emit toolChanged(0);
    });
    connect(m_noteTool, &QPushButton::clicked, this, [this] {
        m_noteTool->setChecked(true);
        m_highlightTool->setChecked(false);
        emit toolChanged(1);
    });

    const Theme::Palette& p = Theme::instance().palette();
    const auto css = [](const QColor& c) {
        return c.alpha() == 255 ? c.name(QColor::HexRgb)
                                : QStringLiteral("rgba(%1,%2,%3,%4)")
                                      .arg(c.red())
                                      .arg(c.green())
                                      .arg(c.blue())
                                      .arg(QString::number(c.alphaF(), 'f', 3));
    };
    setStyleSheet(
        QStringLiteral("#AnnotationBar { background:%1; border-bottom:1px solid %2; }"
                       "#AnnotLabel { color:%3; }"
                       "#AnnotationBar QPushButton#GhostBtn:checked { background:%4;"
                       " border:1px solid %5; color:%5; }")
            .arg(css(p.surface), css(p.hairline), css(p.text), css(p.accentTint),
                 css(p.accent)));

    setCount(0);
}

void AnnotationBar::setCount(int count) {
    m_label->setText(count == 0
                         ? tr("Pick a tool, then drag to highlight or click to drop a note.")
                         : tr("%n annotation(s). Save to write them into the document.", "", count));
    m_save->setEnabled(count > 0);
    m_clear->setEnabled(count > 0);
}
