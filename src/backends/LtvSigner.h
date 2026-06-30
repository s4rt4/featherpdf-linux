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

#include <QString>

// Long-Term Validation (LTV) for signed PDFs.
//
// A digital signature is only as trustworthy as the certificate behind it, and
// certificates expire (or get revoked). LTV embeds the proof a verifier needs to
// confirm the signature was valid *at signing time* directly into the document, so
// it keeps validating years later without reaching back out to a certificate
// authority. The proof lives in a Document Security Store (PDF /DSS): the signer's
// certificate chain, plus OCSP/CRL revocation responses when they can be fetched.
//
// The whole point is that the existing signatures must stay intact, so everything
// is written as an *incremental update* appended to the file. The bytes the
// signatures cover are never touched.
class LtvSigner {
public:
    struct Result {
        int signatures = 0; // signatures we added validation data for
        int certs = 0;      // certificates embedded in the DSS
        int ocsps = 0;      // OCSP responses embedded
        int crls = 0;       // CRLs embedded
    };

    // Add a Document Security Store to `inputPath`, writing the result to
    // `outputPath`. Every signature already in the document gets its certificate
    // chain embedded; OCSP/CRL responses are fetched and embedded too when the
    // certificates advertise them and the network is reachable (their absence is
    // not an error — a certificate-only DSS is still useful LTV material).
    // Returns false with a human-readable message in *error on failure (e.g. the
    // document has no signatures). *result, when given, reports what was embedded.
    static bool addValidationInfo(const QString& inputPath, const QString& outputPath,
                                  QString* error, Result* result = nullptr);
};
