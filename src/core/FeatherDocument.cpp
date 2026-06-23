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

#include "core/FeatherDocument.h"

#include <QFileInfo>
#include <QPdfDocument>

FeatherDocument::FeatherDocument(QObject* parent)
    : QObject(parent), m_pdf(new QPdfDocument(this)) {}

FeatherDocument::~FeatherDocument() = default;

FeatherDocument::LoadResult FeatherDocument::load(const QString& path) {
    const QPdfDocument::Error err = m_pdf->load(path);

    LoadResult result;
    switch (err) {
    case QPdfDocument::Error::None:
        result = LoadResult::Ok;
        break;
    case QPdfDocument::Error::FileNotFound:
        result = LoadResult::FileNotFound;
        break;
    case QPdfDocument::Error::InvalidFileFormat:
        result = LoadResult::InvalidFormat;
        break;
    case QPdfDocument::Error::IncorrectPassword:
        result = LoadResult::PasswordRequired;
        break;
    case QPdfDocument::Error::UnsupportedSecurityScheme:
        result = LoadResult::Unsupported;
        break;
    case QPdfDocument::Error::DataNotYetAvailable:
    case QPdfDocument::Error::Unknown:
    default:
        result = LoadResult::UnknownError;
        break;
    }

    if (result == LoadResult::Ok) {
        m_filePath = path;
        m_rotations.assign(m_pdf->pageCount(), 0);
        setModified(false);
        emit loaded();
    } else {
        m_filePath.clear();
    }
    return result;
}

int FeatherDocument::rotation(int page) const {
    return (page >= 0 && page < m_rotations.size()) ? m_rotations[page] : 0;
}

void FeatherDocument::rotatePage(int page, int deltaDegrees) {
    if (page < 0 || page >= m_rotations.size())
        return;
    int next = (m_rotations[page] + deltaDegrees) % 360;
    if (next < 0)
        next += 360;
    if (next == m_rotations[page])
        return;
    m_rotations[page] = next;
    setModified(true);
    emit pageEdited(page);
}

void FeatherDocument::markSaved() {
    setModified(false);
}

void FeatherDocument::setModified(bool modified) {
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit modifiedChanged(m_modified);
}

void FeatherDocument::close() {
    if (!isLoaded())
        return;
    m_pdf->close();
    m_filePath.clear();
    m_rotations.clear();
    setModified(false);
    emit closed();
}

bool FeatherDocument::isLoaded() const {
    return m_pdf->status() == QPdfDocument::Status::Ready;
}

QString FeatherDocument::fileName() const {
    return m_filePath.isEmpty() ? QString() : QFileInfo(m_filePath).fileName();
}

QString FeatherDocument::title() const {
    const QString meta = m_pdf->metaData(QPdfDocument::MetaDataField::Title).toString().trimmed();
    return meta.isEmpty() ? fileName() : meta;
}

int FeatherDocument::pageCount() const {
    return m_pdf->pageCount();
}

QString FeatherDocument::describe(LoadResult result) {
    switch (result) {
    case LoadResult::Ok:
        return QObject::tr("The document opened successfully.");
    case LoadResult::FileNotFound:
        return QObject::tr("That file could not be found. It may have been moved or deleted.");
    case LoadResult::InvalidFormat:
        return QObject::tr("This file isn't a valid PDF, or it is damaged.");
    case LoadResult::PasswordRequired:
        return QObject::tr("This PDF is password-protected. Enter the password to open it.");
    case LoadResult::Unsupported:
        return QObject::tr("This PDF uses a security scheme Feather can't open yet.");
    case LoadResult::UnknownError:
        break;
    }
    return QObject::tr("The document couldn't be opened.");
}
