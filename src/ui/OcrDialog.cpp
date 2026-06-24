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

#include "ui/OcrDialog.h"

#include "ui/Theme.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHash>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

OcrDialog::OcrDialog(const QStringList& languageCodes, QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Recognize Text"));

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
    setStyleSheet(
        QStringLiteral(
            "QLabel#Hint { color:%2; }"
            "QComboBox { background:%1; border:1px solid %3; border-radius:8px; padding:6px 9px;"
            " color:%4; }"
            "QComboBox:focus { border:1px solid %5; }"
            "QComboBox::drop-down { border:none; width:26px; }"
            "QComboBox::down-arrow { image:url(%6); width:13px; height:13px; }"
            "QComboBox QAbstractItemView { background:%1; border:1px solid %3; border-radius:8px;"
            " padding:4px; outline:0; selection-background-color:%7; selection-color:%4; }")
            .arg(css(p.surface), css(p.dim), css(ctlBorder), css(p.text), css(p.accent), chev,
                 css(p.accentTint)));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 20, 22, 18);
    root->setSpacing(14);

    auto* hint = new QLabel(
        tr("Add a searchable, selectable text layer over the scanned pages. The page images "
           "are unchanged. This can take a while on long documents."),
        this);
    hint->setObjectName(QStringLiteral("Hint"));
    hint->setWordWrap(true);
    root->addWidget(hint);

    static const QHash<QString, QString> names{{"eng", tr("English")}, {"ind", tr("Indonesian")}};
    m_lang = new QComboBox(this);
    for (const QString& code : languageCodes)
        m_lang->addItem(names.value(code, code), code);
    m_lang->setMinimumWidth(220);

    auto* form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->addRow(tr("Language"), m_lang);
    root->addLayout(form);

    root->addStretch(1);

    auto* buttons = new QDialogButtonBox(this);
    auto* run = buttons->addButton(tr("Recognize"), QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    run->setObjectName(QStringLiteral("Share"));
    run->setCursor(Qt::PointingHandCursor);
    run->setEnabled(m_lang->count() > 0);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    resize(420, 0);
}

QString OcrDialog::language() const {
    return m_lang->currentData().toString();
}
