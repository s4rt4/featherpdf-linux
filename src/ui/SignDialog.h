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

#include <QDialog>
#include <QString>
#include <QStringList>

class QComboBox;
class QLineEdit;

// Collects the choices for digitally signing the document: which certificate,
// an optional reason and location, and the key password. The signature is placed
// on the current page by the caller.
class SignDialog : public QDialog {
    Q_OBJECT

public:
    SignDialog(const QStringList& certificates, QWidget* parent = nullptr);

    QString certificate() const;
    QString reason() const;
    QString location() const;
    QString password() const;

private:
    QComboBox* m_cert = nullptr;
    QLineEdit* m_reason = nullptr;
    QLineEdit* m_location = nullptr;
    QLineEdit* m_password = nullptr;
};
