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

#include <QObject>
#include <QString>

class QPdfDocument;

// FeatherDocument is the single façade over the PDF backends. Modules talk to
// this object, never to a backend directly — so page indexing and (later)
// coordinate conversion live in exactly one place. For M0 it wraps the QtPdf /
// PDFium document used for rendering; Poppler (semantic layer) and QPDF
// (lossless writer) are folded in behind this same interface in later milestones.
class FeatherDocument : public QObject {
    Q_OBJECT

public:
    explicit FeatherDocument(QObject* parent = nullptr);
    ~FeatherDocument() override;

    enum class LoadResult {
        Ok,
        FileNotFound,
        InvalidFormat,
        PasswordRequired,
        Unsupported,
        UnknownError,
    };

    // Loads a PDF from disk, replacing any currently open document.
    LoadResult load(const QString& path);
    void close();

    bool isLoaded() const;
    QString filePath() const { return m_filePath; }
    QString fileName() const;       // base name, e.g. "Annual-Report.pdf"
    QString title() const;          // metadata title, falls back to file name
    int pageCount() const;

    // The rendering backend handle. The viewport (QtPdf) renders from this.
    // This accessor is the *one* sanctioned exception to "modules never touch a
    // backend" — only the render layer may use it.
    QPdfDocument* pdf() const { return m_pdf; }

    // Human-readable message for a load failure, ready for a dialog/toast.
    static QString describe(LoadResult result);

signals:
    void loaded();
    void closed();

private:
    QPdfDocument* m_pdf;
    QString m_filePath;
};
