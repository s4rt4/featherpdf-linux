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

#include "ui/DocsDialog.h"

#include "ui/Theme.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace {
// The documentation body for a language, as rich text.
QString docHtml(const QString& lang) {
    if (lang == QStringLiteral("id")) {
        return QStringLiteral(
            "<h2>Selamat datang di Feather PDF</h2>"
            "<p>Alat PDF ringan dan lengkap untuk Linux. Berikut fitur utamanya.</p>"
            "<h3>Membuka &amp; melihat</h3><ul>"
            "<li><b>Buka</b> lewat tombol Open di Home, <i>Open Recent</i>, atau seret PDF ke jendela.</li>"
            "<li>Zoom dengan Ctrl+scroll atau tombol +/−; <i>Fit width</i>/<i>Fit page</i> di menu View.</li>"
            "<li>Mode halaman: Single / Continuous / Two pages (menu View ▸ Page Layout).</li>"
            "<li><b>Cari</b> dengan Ctrl+F. <b>F11</b> untuk mode baca imersif. Klik nomor halaman untuk lompat.</li></ul>"
            "<h3>Panel samping</h3><ul>"
            "<li><b>Thumbnails</b>, <b>Outline</b> (bookmark), <b>Annotations</b>, <b>Attachments</b> "
            "(klik ganda untuk simpan), <b>Layers</b>, dan <b>Forms</b>.</li></ul>"
            "<h3>Operasi halaman</h3><ul>"
            "<li>Putar (Ctrl+[ / Ctrl+]), hapus, dan susun ulang dengan menyeret thumbnail. Undo/Redo didukung.</li>"
            "<li><b>Combine</b>: gabung beberapa PDF (Tools ▸ Combine).</li>"
            "<li>Simpan lossless lewat Save / Save As.</li></ul>"
            "<h3>Keamanan</h3><ul>"
            "<li><b>Proteksi password</b> (AES-256) &amp; <b>izin</b>: File ▸ Protect with Password.</li>"
            "<li><b>Hapus password</b>: File ▸ Remove Password (untuk dokumen terenkripsi).</li>"
            "<li><b>Redaction</b>: Tools ▸ Redact — gambar kotak, lalu Apply untuk menghapus konten permanen.</li></ul>"
            "<h3>Anotasi</h3><ul>"
            "<li>Tools ▸ Comment: <b>Highlight</b>, <b>Note</b>, dan <b>Draw</b> (freehand) dengan pilihan warna.</li>"
            "<li>Klik kanan sebuah mark untuk menghapusnya sebelum disimpan.</li></ul>"
            "<h3>Formulir, tanda tangan &amp; OCR</h3><ul>"
            "<li><b>Isi formulir</b> di panel Forms, lalu Save Form.</li>"
            "<li><b>Tanda tangan digital</b>: Document ▸ Sign; verifikasi di Document ▸ Signatures.</li>"
            "<li><b>OCR</b>: Document ▸ Recognize Text — buat PDF scan jadi bisa dicari.</li></ul>"
            "<h3>Membuat PDF</h3><ul>"
            "<li>File ▸ Create PDF from… — dari gambar atau dokumen Office (lewat LibreOffice).</li></ul>"
            "<h3>Cetak</h3><ul>"
            "<li>File ▸ Print — dialog dengan pratinjau, rentang halaman, subset ganjil/genap, grayscale.</li></ul>");
    }
    return QStringLiteral(
        "<h2>Welcome to Feather PDF</h2>"
        "<p>A light, full-featured PDF tool for Linux. Here are the main features.</p>"
        "<h3>Opening &amp; viewing</h3><ul>"
        "<li><b>Open</b> from the Home button, <i>Open Recent</i>, or drag a PDF onto the window.</li>"
        "<li>Zoom with Ctrl+scroll or the +/− buttons; <i>Fit width</i>/<i>Fit page</i> in the View menu.</li>"
        "<li>Page modes: Single / Continuous / Two pages (View ▸ Page Layout).</li>"
        "<li><b>Search</b> with Ctrl+F. <b>F11</b> for immersive reading. Click the page number to jump.</li></ul>"
        "<h3>Side panels</h3><ul>"
        "<li><b>Thumbnails</b>, <b>Outline</b> (bookmarks), <b>Annotations</b>, <b>Attachments</b> "
        "(double-click to save), <b>Layers</b>, and <b>Forms</b>.</li></ul>"
        "<h3>Page operations</h3><ul>"
        "<li>Rotate (Ctrl+[ / Ctrl+]), delete, and reorder by dragging thumbnails. Undo/Redo supported.</li>"
        "<li><b>Combine</b>: merge several PDFs (Tools ▸ Combine).</li>"
        "<li>Lossless Save / Save As.</li></ul>"
        "<h3>Security</h3><ul>"
        "<li><b>Password protection</b> (AES-256) &amp; <b>permissions</b>: File ▸ Protect with Password.</li>"
        "<li><b>Remove password</b>: File ▸ Remove Password (for encrypted documents).</li>"
        "<li><b>Redaction</b>: Tools ▸ Redact — draw boxes, then Apply to permanently remove content.</li></ul>"
        "<h3>Annotations</h3><ul>"
        "<li>Tools ▸ Comment: <b>Highlight</b>, <b>Note</b>, and <b>Draw</b> (freehand) with colour choices.</li>"
        "<li>Right-click a mark to remove it before saving.</li></ul>"
        "<h3>Forms, signatures &amp; OCR</h3><ul>"
        "<li><b>Fill forms</b> in the Forms panel, then Save Form.</li>"
        "<li><b>Digital signatures</b>: Document ▸ Sign; verify in Document ▸ Signatures.</li>"
        "<li><b>OCR</b>: Document ▸ Recognize Text — make scanned PDFs searchable.</li></ul>"
        "<h3>Creating PDFs</h3><ul>"
        "<li>File ▸ Create PDF from… — from images or office documents (via LibreOffice).</li></ul>"
        "<h3>Printing</h3><ul>"
        "<li>File ▸ Print — a dialog with preview, page ranges, odd/even subset, and grayscale.</li></ul>");
}
} // namespace

DocsDialog::DocsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Documentation"));

    const Theme::Palette& p = Theme::instance().palette();
    const auto css = [](const QColor& c) { return c.name(QColor::HexRgb); };
    setStyleSheet(
        QStringLiteral(
            "QTextBrowser { background:%1; border:1px solid %2; border-radius:10px; color:%3; }"
            "#DocsLang:checked { background:%4; border:1px solid %5; color:%5; }")
            .arg(css(p.surface), css(p.hairline), css(p.text), css(p.accentTint), css(p.accent)));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 14);
    root->setSpacing(12);

    auto* langRow = new QHBoxLayout;
    m_en = new QPushButton(tr("English"), this);
    m_id = new QPushButton(tr("Indonesia"), this);
    for (QPushButton* b : {m_en, m_id}) {
        b->setObjectName(QStringLiteral("DocsLang"));
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
    }
    langRow->addWidget(m_en);
    langRow->addWidget(m_id);
    langRow->addStretch(1);
    root->addLayout(langRow);

    m_browser = new QTextBrowser(this);
    m_browser->setOpenExternalLinks(true);
    m_browser->setMinimumSize(560, 460);
    root->addWidget(m_browser, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    connect(m_en, &QPushButton::clicked, this, [this] { showLanguage(QStringLiteral("en")); });
    connect(m_id, &QPushButton::clicked, this, [this] { showLanguage(QStringLiteral("id")); });

    // Default to Indonesian if the system locale is Indonesian, else English.
    showLanguage(QLocale().language() == QLocale::Indonesian ? QStringLiteral("id")
                                                             : QStringLiteral("en"));
    resize(640, 560);
}

void DocsDialog::showLanguage(const QString& lang) {
    const bool id = lang == QStringLiteral("id");
    m_id->setChecked(id);
    m_en->setChecked(!id);
    m_browser->setHtml(docHtml(lang));
}
