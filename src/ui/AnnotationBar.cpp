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

    auto* swatch = new QLabel(this);
    swatch->setObjectName(QStringLiteral("AnnotSwatch"));
    swatch->setFixedSize(14, 11);

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

    row->addWidget(swatch);
    row->addWidget(m_label);
    row->addStretch(1);
    row->addWidget(m_clear);
    row->addWidget(m_save);
    row->addWidget(done);

    connect(m_save, &QPushButton::clicked, this, &AnnotationBar::saveRequested);
    connect(m_clear, &QPushButton::clicked, this, &AnnotationBar::clearRequested);
    connect(done, &QPushButton::clicked, this, &AnnotationBar::doneRequested);

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
                       "#AnnotSwatch { background:#FFD600; border:1px solid %2; border-radius:3px; }")
            .arg(css(p.surface), css(p.hairline), css(p.text)));

    setCount(0);
}

void AnnotationBar::setCount(int count) {
    m_label->setText(count == 0
                         ? tr("Highlight mode — drag over text or anything you want to mark.")
                         : tr("%n highlight(s). Save to write them into the document.", "", count));
    m_save->setEnabled(count > 0);
    m_clear->setEnabled(count > 0);
}
