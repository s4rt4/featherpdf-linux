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

#include "ui/WatermarkDialog.h"

#include "ui/Theme.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

WatermarkDialog::WatermarkDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Watermark"));

    const Theme::Palette& p = Theme::instance().palette();
    QColor ctlBorder = p.text;
    ctlBorder.setAlpha(46);
    const auto css = [](const QColor& c) {
        return c.alpha() == 255 ? c.name(QColor::HexRgb)
                                : QStringLiteral("rgba(%1,%2,%3,%4)")
                                      .arg(c.red())
                                      .arg(c.green())
                                      .arg(c.blue())
                                      .arg(QString::number(c.alphaF(), 'f', 3));
    };
    const QString chev = Theme::instance().symbolicIconPath(QStringLiteral("chevron-down"), p.dim);
    const QString chevUp = Theme::instance().symbolicIconPath(QStringLiteral("chevron-up"), p.dim);
    setStyleSheet(
        QStringLiteral(
            "QLabel#Hint { color:%2; }"
            "QLineEdit, QSpinBox { background:%1; border:1px solid %3; border-radius:8px;"
            " padding:6px 9px; color:%4; selection-background-color:%5; selection-color:#FFFFFF; }"
            "QLineEdit:focus, QSpinBox:focus { border:1px solid %5; }"
            "QSpinBox::up-button, QSpinBox::down-button { background:transparent; border:none;"
            " width:18px; }"
            "QSpinBox::up-arrow { image:url(%6); width:11px; height:11px; }"
            "QSpinBox::down-arrow { image:url(%7); width:11px; height:11px; }")
            .arg(css(p.surface), css(p.dim), css(ctlBorder), css(p.text), css(p.accent), chevUp,
                 chev));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 20, 22, 18);
    root->setSpacing(14);

    auto* hint = new QLabel(tr("Stamp a centred, rotated label on every page."), this);
    hint->setObjectName(QStringLiteral("Hint"));
    hint->setWordWrap(true);
    root->addWidget(hint);

    m_text = new QLineEdit(tr("CONFIDENTIAL"), this);
    m_text->setMinimumWidth(260);

    // Colour swatches.
    m_colors = {QColor(120, 120, 120), QColor(200, 0, 0), QColor(40, 90, 200),
                QColor(30, 140, 70)};
    auto* swatchRow = new QHBoxLayout;
    swatchRow->setSpacing(8);
    const QString textColor = p.text.name();
    for (int i = 0; i < m_colors.size(); ++i) {
        auto* sw = new QPushButton(this);
        sw->setCheckable(true);
        sw->setCursor(Qt::PointingHandCursor);
        sw->setFixedSize(22, 22);
        sw->setStyleSheet(
            QStringLiteral("QPushButton { background:%1; border:1px solid rgba(0,0,0,0.25);"
                           " border-radius:5px; } QPushButton:checked { border:2px solid %2; }")
                .arg(m_colors[i].name(), textColor));
        connect(sw, &QPushButton::clicked, this, [this, i] { selectSwatch(i); });
        m_swatches.append(sw);
        swatchRow->addWidget(sw);
    }
    swatchRow->addStretch(1);

    m_opacity = new QSpinBox(this);
    m_opacity->setRange(5, 100);
    m_opacity->setValue(30);
    m_opacity->setSuffix(QStringLiteral(" %"));
    m_size = new QSpinBox(this);
    m_size->setRange(8, 200);
    m_size->setValue(64);
    m_size->setSuffix(QStringLiteral(" pt"));
    m_rotation = new QSpinBox(this);
    m_rotation->setRange(-90, 90);
    m_rotation->setValue(45);
    m_rotation->setSuffix(QStringLiteral("°"));

    auto* form = new QFormLayout;
    form->setSpacing(10);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->addRow(tr("Text"), m_text);
    form->addRow(tr("Colour"), swatchRow);
    form->addRow(tr("Opacity"), m_opacity);
    form->addRow(tr("Size"), m_size);
    form->addRow(tr("Rotation"), m_rotation);
    root->addLayout(form);
    root->addStretch(1);

    auto* buttons = new QDialogButtonBox(this);
    m_apply = buttons->addButton(tr("Add Watermark"), QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    m_apply->setObjectName(QStringLiteral("Share"));
    m_apply->setCursor(Qt::PointingHandCursor);
    connect(m_text, &QLineEdit::textChanged, this,
            [this](const QString& t) { m_apply->setEnabled(!t.trimmed().isEmpty()); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    selectSwatch(0);
    resize(440, 0);
}

void WatermarkDialog::selectSwatch(int i) {
    for (int j = 0; j < m_swatches.size(); ++j)
        m_swatches[j]->setChecked(j == i);
    if (i >= 0 && i < m_colors.size())
        m_color = m_colors[i];
}

QString WatermarkDialog::text() const { return m_text->text().trimmed(); }
double WatermarkDialog::opacity() const { return m_opacity->value() / 100.0; }
double WatermarkDialog::fontSize() const { return m_size->value(); }
double WatermarkDialog::rotation() const { return m_rotation->value(); }
