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

#include "ui/SignDialog.h"

#include "ui/Theme.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

SignDialog::SignDialog(const QStringList& certificates, QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Sign Document"));

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
    const QString chevDown = Theme::instance().symbolicIconPath(QStringLiteral("chevron-down"), p.dim);
    setStyleSheet(
        QStringLiteral(
            "QLabel#Hint { color:%2; }"
            "QLineEdit, QComboBox { background:%1; border:1px solid %3; border-radius:8px;"
            " padding:6px 9px; color:%4; selection-background-color:%5; selection-color:#FFFFFF; }"
            "QLineEdit:focus, QComboBox:focus { border:1px solid %5; }"
            "QComboBox::drop-down { border:none; width:26px; }"
            "QComboBox::down-arrow { image:url(%6); width:13px; height:13px; }"
            "QComboBox QAbstractItemView { background:%1; border:1px solid %3; border-radius:8px;"
            " padding:4px; outline:0; selection-background-color:%7; selection-color:%4; }")
            .arg(css(p.surface), css(p.dim), css(ctlBorder), css(p.text), css(p.accent), chevDown,
                 css(p.accentTint)));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 20, 22, 18);
    root->setSpacing(14);

    auto* hint = new QLabel(
        tr("Sign with a certificate from your system. The signature is placed on the current "
           "page; anyone can then verify the document is intact."),
        this);
    hint->setObjectName(QStringLiteral("Hint"));
    hint->setWordWrap(true);
    root->addWidget(hint);

    auto* form = new QFormLayout;
    form->setSpacing(10);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_cert = new QComboBox(this);
    m_cert->addItems(certificates);
    m_cert->setMinimumWidth(280);
    m_reason = new QLineEdit(this);
    m_reason->setPlaceholderText(tr("e.g. Approved"));
    m_location = new QLineEdit(this);
    m_location->setPlaceholderText(tr("e.g. Jakarta"));
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText(tr("Leave empty if the key has no password"));
    form->addRow(tr("Certificate"), m_cert);
    form->addRow(tr("Reason"), m_reason);
    form->addRow(tr("Location"), m_location);
    form->addRow(tr("Password"), m_password);
    root->addLayout(form);

    root->addStretch(1);

    auto* buttons = new QDialogButtonBox(this);
    auto* sign = buttons->addButton(tr("Sign"), QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    sign->setObjectName(QStringLiteral("Share"));
    sign->setCursor(Qt::PointingHandCursor);
    sign->setEnabled(!certificates.isEmpty());
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    resize(440, 0);
}

QString SignDialog::certificate() const { return m_cert->currentText(); }
QString SignDialog::reason() const { return m_reason->text().trimmed(); }
QString SignDialog::location() const { return m_location->text().trimmed(); }
QString SignDialog::password() const { return m_password->text(); }
