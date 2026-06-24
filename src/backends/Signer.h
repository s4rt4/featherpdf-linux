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

#include <QList>
#include <QRectF>
#include <QString>
#include <QStringList>

// The digital-signature backend (Poppler-Qt6 over its NSS crypto backend).
// Signs a document with a certificate from the user's NSS database and verifies
// the signatures already in a document.
class Signer {
public:
    // Display names (NSS nicknames) of certificates available for signing. Empty
    // if none are set up in the NSS database.
    static QStringList availableCertificates();

    // Sign `inputPath` with the named certificate, writing `outputPath`. `rect`
    // is the signature's position on `page` (0-based) in page points (origin
    // bottom-left). `password` unlocks the key (often empty for a local NSS DB).
    // Returns true on success; on failure fills *error.
    static bool sign(const QString& inputPath, const QString& outputPath,
                     const QString& certName, const QString& password, const QString& reason,
                     const QString& location, int page, const QRectF& rect, QString* error);

    struct SignatureInfo {
        QString signer;
        QString status;   // human-readable validity
        bool valid = false;
        QString time;     // signing time, localized
        QString reason;
        QString location;
    };

    // The signatures present in `path`, each validated.
    static QList<SignatureInfo> verify(const QString& path);
};
