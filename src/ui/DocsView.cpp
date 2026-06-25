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

#include "ui/DocsView.h"

#include "ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

struct Topic {
    QString id;
    QString titleEn, titleId;
    QString bodyEn, bodyId;
};
struct Group {
    QString titleEn, titleId;
    QList<Topic> topics;
};

// Wrap five content blocks with localized section headings.
QString sect(const QString& lang, const QString& what, const QString& purpose, const QString& how,
             const QString& use, const QString& trouble) {
    const bool id = lang == QStringLiteral("id");
    const auto h = [](const QString& t) { return QStringLiteral("<h4>%1</h4>").arg(t); };
    return h(id ? QStringLiteral("Apa ini") : QStringLiteral("What it is")) + what +
           h(id ? QStringLiteral("Fungsinya") : QStringLiteral("Purpose")) + purpose +
           h(id ? QStringLiteral("Cara kerja") : QStringLiteral("How it works")) + how +
           h(id ? QStringLiteral("Cara memakai") : QStringLiteral("How to use")) + use +
           h(id ? QStringLiteral("Pemecahan masalah") : QStringLiteral("Troubleshooting")) + trouble;
}

QList<Group> buildDocs() {
    QList<Group> g;

    // ── Getting started ──────────────────────────────────────────────────────
    Group start{"Getting started", "Memulai", {}};
    start.topics.append(
        {"overview", "Overview", "Ringkasan",
         sect("en",
              "<p>Feather PDF is a native, full-featured PDF tool for Linux — light on the system, "
              "powered by mature libraries (PDFium, Poppler, QPDF, Tesseract, LibreOffice).</p>",
              "<p>One app to read, organize, secure, annotate, fill, sign, recognize, and create "
              "PDFs without a web engine.</p>",
              "<p>Each capability is a thin UI over a trusted backend; edits are lossless where "
              "possible (page structure via QPDF) and rasterized only when safety requires it "
              "(redaction).</p>",
              "<p>Open a file from Home, then use the command bar, the left panels, the right Tools "
              "pane, and the menus. This documentation explains every feature.</p>",
              "<p>If the app won't start, run it from a terminal to see messages. Most features work "
              "offline; OCR, conversion, and signing rely on external tools described in their "
              "sections.</p>"),
         sect("id",
              "<p>Feather PDF adalah alat PDF native dan lengkap untuk Linux - ringan, ditenagai "
              "pustaka matang (PDFium, Poppler, QPDF, Tesseract, LibreOffice).</p>",
              "<p>Satu aplikasi untuk membaca, menata, mengamankan, menganotasi, mengisi, "
              "menandatangani, mengenali teks, dan membuat PDF tanpa web engine.</p>",
              "<p>Tiap kemampuan adalah lapisan UI tipis di atas backend tepercaya; penyuntingan "
              "lossless bila memungkinkan (struktur halaman via QPDF) dan baru dirasterisasi bila "
              "keamanan menuntut (redaksi).</p>",
              "<p>Buka berkas dari Home, lalu gunakan command bar, panel kiri, Tools pane kanan, dan "
              "menu. Dokumentasi ini menjelaskan setiap fitur.</p>",
              "<p>Jika aplikasi tak mau mulai, jalankan dari terminal untuk melihat pesan. Sebagian "
              "besar fitur jalan offline; OCR, konversi, dan tanda tangan butuh alat eksternal yang "
              "dijelaskan di bagiannya.</p>")});
    start.topics.append(
        {"open-view", "Opening &amp; viewing", "Membuka &amp; melihat",
         sect("en",
              "<p>The viewer renders pages with PDFium and lays them out instantly at any page "
              "count.</p>",
              "<p>Read comfortably: zoom, fit, page layouts, and an immersive mode.</p>",
              "<p>Pages render off the UI thread into a cache, so even a 4000-page file opens "
              "instantly and scrolls smoothly.</p>",
              "<ul><li>Open from the Home <b>Open</b> button, <b>File ▸ Open Recent</b>, or by "
              "dragging a PDF onto the window.</li><li>Zoom with <b>Ctrl+scroll</b> or the +/− "
              "buttons; <b>View ▸ Fit Width/Page</b>.</li><li><b>View ▸ Page Layout</b>: Single, "
              "Continuous, or Two Pages.</li><li><b>F11</b> toggles immersive reading; click the "
              "page counter to go to a page.</li></ul>",
              "<p>If a file won't open, it may be damaged or password-protected - you'll be prompted "
              "for a password if it's encrypted (see Security).</p>"),
         sect("id",
              "<p>Penampil me-render halaman dengan PDFium dan menata tata letaknya seketika berapa "
              "pun jumlah halaman.</p>",
              "<p>Membaca nyaman: zoom, fit, tata letak halaman, dan mode imersif.</p>",
              "<p>Halaman di-render di luar thread UI ke dalam cache, jadi berkas 4000 halaman pun "
              "terbuka seketika dan mulus saat di-scroll.</p>",
              "<ul><li>Buka dari tombol <b>Open</b> di Home, <b>File ▸ Open Recent</b>, atau seret "
              "PDF ke jendela.</li><li>Zoom dengan <b>Ctrl+scroll</b> atau tombol +/−; <b>View ▸ Fit "
              "Width/Page</b>.</li><li><b>View ▸ Page Layout</b>: Single, Continuous, atau Two "
              "Pages.</li><li><b>F11</b> mode baca imersif; klik penghitung halaman untuk lompat ke "
              "halaman.</li></ul>",
              "<p>Jika berkas tak terbuka, mungkin rusak atau terproteksi password - kamu akan "
              "diminta password bila terenkripsi (lihat Keamanan).</p>")});
    start.topics.append(
        {"search", "Search", "Pencarian",
         sect("en", "<p>Full-text search across the document.</p>",
              "<p>Find every occurrence of a word or phrase and jump between them.</p>",
              "<p>Matches are found with Poppler's text model and highlighted on the pages.</p>",
              "<ul><li>Press <b>Ctrl+F</b> (or the search icon in the tab strip).</li><li>Type your "
              "query; use <b>Enter</b>/<b>Shift+Enter</b> for next/previous. <b>Esc</b> closes "
              "it.</li></ul>",
              "<p>If nothing is found in a scanned PDF, it has no text layer - run <b>Recognize Text "
              "(OCR)</b> first.</p>"),
         sect("id", "<p>Pencarian teks penuh di seluruh dokumen.</p>",
              "<p>Temukan setiap kemunculan kata/frasa dan lompat antar hasil.</p>",
              "<p>Hasil ditemukan dengan model teks Poppler dan disorot di halaman.</p>",
              "<ul><li>Tekan <b>Ctrl+F</b> (atau ikon cari di tab strip).</li><li>Ketik kueri; "
              "<b>Enter</b>/<b>Shift+Enter</b> untuk berikut/sebelumnya. <b>Esc</b> menutup.</li></ul>",
              "<p>Jika tak ada hasil pada PDF hasil scan, berkas itu tak punya lapisan teks - "
              "jalankan <b>Recognize Text (OCR)</b> dulu.</p>")});
    g.append(start);

    // ── Panels ───────────────────────────────────────────────────────────────
    Group panels{"Side panels", "Panel samping", {}};
    panels.topics.append(
        {"thumbnails", "Thumbnails", "Thumbnails",
         sect("en", "<p>A scrollable strip of page previews in the left rail.</p>",
              "<p>Navigate quickly and reorder pages.</p>",
              "<p>Previews render lazily off-thread; the current page is framed in the accent "
              "colour.</p>",
              "<ul><li>Open the <b>Thumbnails</b> rail icon. Click a page to jump to it.</li><li>Drag "
              "a thumbnail to reorder pages (undoable).</li></ul>",
              "<p>If thumbnails look blank briefly, they're still rendering; they appear as you "
              "scroll.</p>"),
         sect("id", "<p>Deretan pratinjau halaman yang bisa di-scroll di rail kiri.</p>",
              "<p>Navigasi cepat dan menyusun ulang halaman.</p>",
              "<p>Pratinjau di-render lazily di luar thread; halaman aktif diberi bingkai warna "
              "aksen.</p>",
              "<ul><li>Buka ikon rail <b>Thumbnails</b>. Klik halaman untuk lompat.</li><li>Seret "
              "thumbnail untuk menyusun ulang (bisa di-undo).</li></ul>",
              "<p>Jika thumbnail sempat kosong, itu masih di-render; akan muncul saat kamu "
              "scroll.</p>")});
    panels.topics.append(
        {"outline", "Outline", "Outline",
         sect("en", "<p>The document's bookmark/heading tree - now editable.</p>",
              "<p>Jump to chapters and sections, and add, rename, or delete bookmarks.</p>",
              "<p>Read from the PDF's bookmarks (Poppler); edits are written back into the "
              "/Outlines structure with QPDF.</p>",
              "<ul><li>Open the <b>Outline</b> rail icon and click an entry to jump.</li>"
              "<li>Toolbar icons: <b>+</b> adds a bookmark to the current page, the pencil renames "
              "the selected one, <b>×</b> deletes it, and the disk icon <b>saves</b> the outline.</li>"
              "<li>Double-click an entry to rename it in place.</li></ul>",
              "<p>An empty outline means the PDF has no bookmarks - use <b>+</b> to start one.</p>"),
         sect("id", "<p>Pohon bookmark/heading dokumen - kini bisa diedit.</p>",
              "<p>Lompat ke bab/bagian, serta tambah, ganti nama, atau hapus bookmark.</p>",
              "<p>Dibaca dari bookmark PDF (Poppler); suntingan ditulis kembali ke struktur "
              "/Outlines dengan QPDF.</p>",
              "<ul><li>Buka ikon rail <b>Outline</b> lalu klik entri untuk melompat.</li>"
              "<li>Ikon toolbar: <b>+</b> menambah bookmark ke halaman aktif, pensil mengganti nama, "
              "<b>×</b> menghapus, dan ikon disk <b>menyimpan</b> outline.</li>"
              "<li>Klik ganda entri untuk mengganti namanya langsung.</li></ul>",
              "<p>Outline kosong berarti PDF tak punya bookmark - pakai <b>+</b> untuk memulai.</p>")});
    panels.topics.append(
        {"attachments", "Attachments", "Attachments",
         sect("en", "<p>Files embedded inside the PDF.</p>",
              "<p>View and extract attached files.</p>",
              "<p>Listed via Poppler; data is read on demand.</p>",
              "<ul><li>Open the <b>Attachments</b> rail icon. <b>Double-click</b> an item to save "
              "it.</li></ul>",
              "<p>An empty list means the PDF carries no embedded files.</p>"),
         sect("id", "<p>Berkas yang tertanam di dalam PDF.</p>",
              "<p>Lihat dan ekstrak berkas lampiran.</p>",
              "<p>Didaftar via Poppler; data dibaca saat dibutuhkan.</p>",
              "<ul><li>Buka ikon rail <b>Attachments</b>. <b>Klik ganda</b> item untuk "
              "menyimpannya.</li></ul>",
              "<p>Daftar kosong berarti PDF tak membawa berkas tertanam.</p>")});
    panels.topics.append(
        {"layers", "Layers", "Layers",
         sect("en", "<p>The document's optional content groups (OCG).</p>",
              "<p>See which layers a PDF defines and their visibility.</p>",
              "<p>Read-only: the viewer renders with PDFium, which can't toggle layer visibility, so "
              "the list reflects state without changing the render.</p>",
              "<ul><li>Open the <b>Layers</b> rail icon to review the layers.</li></ul>",
              "<p>Most PDFs have no layers, so the panel is usually empty.</p>"),
         sect("id", "<p>Optional content group (OCG) dokumen.</p>",
              "<p>Lihat layer apa saja yang didefinisikan PDF dan visibilitasnya.</p>",
              "<p>Hanya-baca: penampil me-render dengan PDFium yang tak bisa meng-toggle visibilitas "
              "layer, jadi daftar menampilkan status tanpa mengubah render.</p>",
              "<ul><li>Buka ikon rail <b>Layers</b> untuk meninjau layer.</li></ul>",
              "<p>Kebanyakan PDF tak punya layer, jadi panel biasanya kosong.</p>")});
    g.append(panels);

    // ── Editing ──────────────────────────────────────────────────────────────
    Group editing{"Editing &amp; assembly", "Mengedit &amp; merakit", {}};
    editing.topics.append(
        {"page-ops", "Organize pages", "Menata halaman",
         sect("en", "<p>Lossless page operations on the open document.</p>",
              "<p>Fix orientation, drop pages, reorder, and insert, extract, or crop pages.</p>",
              "<p>Rotate/delete/reorder are tracked per tab with undo/redo and written back "
              "losslessly by QPDF. Insert/extract/crop write a new copy (page objects are copied, "
              "not re-rendered) and open the result.</p>",
              "<ul><li>Rotate: <b>Ctrl+[</b> / <b>Ctrl+]</b> or the Document menu.</li>"
              "<li>Delete: Document ▸ Delete Page. Reorder: drag thumbnails.</li>"
              "<li><b>Insert Pages…</b> pulls pages from another PDF to a chosen position.</li>"
              "<li><b>Extract Pages…</b> saves a page range (e.g. <i>1-3, 5</i>) to a new PDF.</li>"
              "<li><b>Crop Pages…</b> trims margins (mm) on all or selected pages.</li>"
              "<li><b>Ctrl+Z</b>/<b>Ctrl+Y</b> undo/redo; then <b>Save</b> or <b>Save As</b>.</li></ul>",
              "<p>If Save is disabled, no document is open. Edits aren't on disk until you save.</p>"),
         sect("id", "<p>Operasi halaman lossless pada dokumen terbuka.</p>",
              "<p>Perbaiki orientasi, buang halaman, susun ulang, serta sisip, ekstrak, atau crop "
              "halaman.</p>",
              "<p>Putar/hapus/susun-ulang dilacak per tab dengan undo/redo dan ditulis balik lossless "
              "oleh QPDF. Sisip/ekstrak/crop menulis salinan baru (objek halaman disalin, bukan "
              "di-render ulang) lalu membuka hasilnya.</p>",
              "<ul><li>Putar: <b>Ctrl+[</b> / <b>Ctrl+]</b> atau menu Document.</li>"
              "<li>Hapus: Document ▸ Delete Page. Susun ulang: seret thumbnail.</li>"
              "<li><b>Insert Pages…</b> menyisipkan halaman dari PDF lain ke posisi pilihan.</li>"
              "<li><b>Extract Pages…</b> menyimpan rentang halaman (mis. <i>1-3, 5</i>) ke PDF baru.</li>"
              "<li><b>Crop Pages…</b> memangkas margin (mm) di semua atau sebagian halaman.</li>"
              "<li><b>Ctrl+Z</b>/<b>Ctrl+Y</b> undo/redo; lalu <b>Save</b> atau <b>Save As</b>.</li></ul>",
              "<p>Jika Save nonaktif, tak ada dokumen terbuka. Perubahan belum tersimpan sampai kamu "
              "menyimpan.</p>")});
    editing.topics.append(
        {"edit-text", "Edit text", "Edit teks",
         sect("en", "<p>Edit the text you added with the Text tool, and a bridge to LibreOffice "
              "Draw for heavier edits.</p>",
              "<p>Fix a typo in a text box you placed, or hand the file to Draw to move body text "
              "and images.</p>",
              "<p>Text boxes are /FreeText annotations Feather owns end to end, so it can rewrite "
              "their words and appearance (QPDF). Editing the document's original body text with "
              "reflow is still in development; LibreOffice Draw is the interim tool for that.</p>",
              "<ul><li>Tools ▸ <b>Edit text</b> (or Document ▸ Edit Text…) lists your text boxes - "
              "pick one to change its words or delete it.</li>"
              "<li>Document ▸ <b>Open in LibreOffice Draw</b> opens the PDF in Draw for heavy "
              "layout, body-text, and image edits.</li></ul>",
              "<p>\"No editable text boxes\" means you haven't added any - use the <b>Text</b> "
              "annotation tool first. The Draw bridge needs LibreOffice installed.</p>"),
         sect("id", "<p>Edit teks yang kamu tambahkan dengan alat Text, plus jembatan ke "
              "LibreOffice Draw untuk edit berat.</p>",
              "<p>Perbaiki salah ketik di kotak teks yang kamu pasang, atau serahkan berkas ke Draw "
              "untuk memindah teks isi dan gambar.</p>",
              "<p>Kotak teks adalah anotasi /FreeText yang dimiliki Feather sepenuhnya, jadi kata dan "
              "tampilannya bisa ditulis ulang (QPDF). Mengedit teks isi asli dokumen dengan reflow "
              "masih dikembangkan; LibreOffice Draw adalah alat sementara untuk itu.</p>",
              "<ul><li>Tools ▸ <b>Edit text</b> (atau Document ▸ Edit Text…) mendaftar kotak teksmu - "
              "pilih satu untuk mengubah kata atau menghapusnya.</li>"
              "<li>Document ▸ <b>Open in LibreOffice Draw</b> membuka PDF di Draw untuk edit tata "
              "letak, teks isi, dan gambar yang berat.</li></ul>",
              "<p>\"No editable text boxes\" berarti kamu belum menambah apa pun - pakai alat anotasi "
              "<b>Text</b> dulu. Jembatan Draw butuh LibreOffice terpasang.</p>")});
    editing.topics.append(
        {"combine", "Combine", "Combine",
         sect("en", "<p>Merge several PDFs into one.</p>",
              "<p>Assemble multiple documents in any order.</p>",
              "<p>QPDF copies the pages losslessly (foreign-object copy) into a single file.</p>",
              "<ul><li><b>Tools ▸ Combine</b>. Add files, drag to reorder, then Combine - the result "
              "opens in a new tab.</li></ul>",
              "<p>Need at least two files. A file that fails to open is skipped.</p>"),
         sect("id", "<p>Gabungkan beberapa PDF jadi satu.</p>",
              "<p>Rakit beberapa dokumen dalam urutan bebas.</p>",
              "<p>QPDF menyalin halaman secara lossless (salin objek asing) ke satu berkas.</p>",
              "<ul><li><b>Tools ▸ Combine</b>. Tambah berkas, seret untuk mengurutkan, lalu Combine - "
              "hasil terbuka di tab baru.</li></ul>",
              "<p>Butuh minimal dua berkas. Berkas yang gagal dibuka dilewati.</p>")});
    editing.topics.append(
        {"create", "Create PDF", "Membuat PDF",
         sect("en", "<p>Turn images or office documents into a PDF.</p>",
              "<p>Make a PDF from PNG/JPEG images or from Word/Excel/etc.</p>",
              "<p>Images become pages natively (QPdfWriter); office files are converted by "
              "LibreOffice headless in an isolated profile.</p>",
              "<ul><li><b>File ▸ Create PDF from…</b> (or Tools ▸ Create). Pick images to combine, or "
              "a single document to convert.</li></ul>",
              "<p>Office conversion needs LibreOffice installed; the first run can take several "
              "seconds. If it fails, open the file in LibreOffice to check it's valid.</p>"),
         sect("id", "<p>Ubah gambar atau dokumen Office menjadi PDF.</p>",
              "<p>Buat PDF dari gambar PNG/JPEG atau dari Word/Excel/dll.</p>",
              "<p>Gambar jadi halaman secara native (QPdfWriter); berkas Office dikonversi oleh "
              "LibreOffice headless dengan profil terisolasi.</p>",
              "<ul><li><b>File ▸ Create PDF from…</b> (atau Tools ▸ Create). Pilih gambar untuk "
              "digabung, atau satu dokumen untuk dikonversi.</li></ul>",
              "<p>Konversi Office butuh LibreOffice terpasang; jalan pertama bisa beberapa detik. "
              "Jika gagal, buka berkas di LibreOffice untuk memastikan valid.</p>")});
    editing.topics.append(
        {"print", "Print", "Cetak",
         sect("en", "<p>A custom print dialog with a live preview.</p>",
              "<p>Print to a printer or to a PDF file with fine control.</p>",
              "<p>Printers are discovered on a background thread so the dialog opens instantly; "
              "pages are rendered to the chosen device honouring your edits.</p>",
              "<ul><li><b>File ▸ Print</b> (Ctrl+P). Choose the printer or Print to File, a page "
              "range, odd/even subset, copies, and grayscale.</li></ul>",
              "<p>If a real printer is slow to appear, it's still being discovered. Printing to a "
              "real printer briefly loads its driver on first use.</p>"),
         sect("id", "<p>Dialog cetak kustom dengan pratinjau langsung.</p>",
              "<p>Cetak ke printer atau ke berkas PDF dengan kontrol detail.</p>",
              "<p>Printer ditemukan di thread latar agar dialog terbuka seketika; halaman di-render "
              "ke perangkat terpilih sesuai penyuntinganmu.</p>",
              "<ul><li><b>File ▸ Print</b> (Ctrl+P). Pilih printer atau Print to File, rentang "
              "halaman, subset ganjil/genap, salinan, dan grayscale.</li></ul>",
              "<p>Jika printer asli lambat muncul, ia masih ditemukan. Mencetak ke printer asli "
              "sebentar memuat driver-nya saat pertama kali.</p>")});
    g.append(editing);

    // ── Security ─────────────────────────────────────────────────────────────
    Group security{"Security", "Keamanan", {}};
    security.topics.append(
        {"protect", "Protect with password", "Proteksi password",
         sect("en", "<p>Encrypt the document and restrict what recipients may do.</p>",
              "<p>Require a password to open, and/or limit printing, copying, and editing.</p>",
              "<p>QPDF writes AES-256 (PDF 2.0 R6) encryption. Restricting a permission generates a "
              "separate owner password so the limit is enforced.</p>",
              "<ul><li><b>File ▸ Protect with Password</b>. Set an open password and/or untick "
              "permissions, then save the protected copy.</li></ul>",
              "<p>The password can't be recovered - keep it safe. The encrypted copy can be reopened "
              "in Feather PDF by entering the password.</p>"),
         sect("id", "<p>Enkripsi dokumen dan batasi yang boleh dilakukan penerima.</p>",
              "<p>Wajibkan password untuk membuka, dan/atau batasi cetak, salin, dan edit.</p>",
              "<p>QPDF menulis enkripsi AES-256 (PDF 2.0 R6). Membatasi izin menghasilkan owner "
              "password terpisah agar batasan berlaku.</p>",
              "<ul><li><b>File ▸ Protect with Password</b>. Setel password buka dan/atau hilangkan "
              "centang izin, lalu simpan salinan terproteksi.</li></ul>",
              "<p>Password tak bisa dipulihkan - simpan baik-baik. Salinan terenkripsi bisa dibuka "
              "lagi di Feather PDF dengan memasukkan password.</p>")});
    security.topics.append(
        {"remove-password", "Remove password", "Hapus password",
         sect("en", "<p>Save an unprotected copy of an encrypted document.</p>",
              "<p>Drop the encryption once you've opened the file.</p>",
              "<p>QPDF rewrites the document without an encryption layer, using the password you "
              "opened it with.</p>",
              "<ul><li>Open the encrypted file (enter its password), then <b>File ▸ Remove "
              "Password</b>.</li></ul>",
              "<p>The menu item is only enabled for documents that are actually encrypted.</p>"),
         sect("id", "<p>Simpan salinan tanpa proteksi dari dokumen terenkripsi.</p>",
              "<p>Lepaskan enkripsi setelah berkas terbuka.</p>",
              "<p>QPDF menulis ulang dokumen tanpa lapisan enkripsi, memakai password yang dipakai "
              "membuka.</p>",
              "<ul><li>Buka berkas terenkripsi (masukkan password), lalu <b>File ▸ Remove "
              "Password</b>.</li></ul>",
              "<p>Menu ini hanya aktif untuk dokumen yang memang terenkripsi.</p>")});
    security.topics.append(
        {"redact", "Redaction", "Redaksi",
         sect("en", "<p>Permanently remove sensitive content, not just cover it.</p>",
              "<p>Black out text or images so they're truly gone from the file.</p>",
              "<p>Each redacted page is flattened to an image with the marked areas painted black, so "
              "the underlying text and graphics no longer exist. Other pages stay lossless.</p>",
              "<ul><li><b>Tools ▸ Redact</b>. Drag boxes over anything to remove, then <b>Apply "
              "Redaction</b> and save.</li></ul>",
              "<p>Trade-off: a redacted page becomes image-only, so its remaining text is no longer "
              "selectable. Always verify the saved file before sharing.</p>"),
         sect("id", "<p>Hapus konten sensitif secara permanen, bukan sekadar menutupinya.</p>",
              "<p>Hitamkan teks atau gambar agar benar-benar hilang dari berkas.</p>",
              "<p>Tiap halaman yang diredaksi diratakan jadi gambar dengan area bertanda dihitamkan, "
              "sehingga teks dan grafik di bawahnya tak lagi ada. Halaman lain tetap lossless.</p>",
              "<ul><li><b>Tools ▸ Redact</b>. Seret kotak di atas yang ingin dihapus, lalu <b>Apply "
              "Redaction</b> dan simpan.</li></ul>",
              "<p>Konsekuensi: halaman teredaksi jadi image-only, sehingga teksnya tak bisa "
              "diseleksi lagi. Selalu verifikasi berkas tersimpan sebelum dibagikan.</p>")});
    g.append(security);

    // ── Review & recognition ─────────────────────────────────────────────────
    Group review{"Review &amp; recognition", "Tinjauan &amp; pengenalan", {}};
    review.topics.append(
        {"annotate", "Annotations", "Anotasi",
         sect("en", "<p>Highlight, note, freehand ink, underline, strike-through, rectangle, line, "
              "arrow, and text boxes.</p>",
              "<p>Mark up a document for review and save the marks into the file.</p>",
              "<p>Built with QPDF and given an explicit appearance stream, so the marks render in "
              "any viewer (including this one on reopen). Form values can also be exchanged as "
              "<b>XFDF</b>.</p>",
              "<ul><li><b>Tools ▸ Comment</b>, then pick a tool from the bar: Highlight, Note, Draw, "
              "Underline, Strikeout, Rectangle, Line, Arrow, or Text. Drag on the page (Note clicks; "
              "Text prompts for words), pick a colour, then <b>Save Annotations</b>.</li>"
              "<li>Right-click a mark to remove it before saving.</li>"
              "<li><b>File ▸ Form Data</b> exports/imports field values as XFDF.</li></ul>",
              "<p>Annotations are written to a new file; the original is untouched until you save.</p>"),
         sect("id", "<p>Sorotan, catatan, tinta bebas, garis bawah, coret, kotak, garis, panah, dan "
              "kotak teks.</p>",
              "<p>Tandai dokumen untuk ditinjau dan simpan tanda ke dalam berkas.</p>",
              "<p>Dibangun dengan QPDF dan diberi appearance stream eksplisit, sehingga tanda "
              "ter-render di penampil mana pun (termasuk ini saat dibuka ulang). Nilai form juga bisa "
              "dipertukarkan sebagai <b>XFDF</b>.</p>",
              "<ul><li><b>Tools ▸ Comment</b>, lalu pilih alat di bar: Highlight, Note, Draw, "
              "Underline, Strikeout, Rectangle, Line, Arrow, atau Text. Seret di halaman (Note diklik; "
              "Text meminta kata), pilih warna, lalu <b>Save Annotations</b>.</li>"
              "<li>Klik kanan tanda untuk menghapusnya sebelum disimpan.</li>"
              "<li><b>File ▸ Form Data</b> mengekspor/impor nilai field sebagai XFDF.</li></ul>",
              "<p>Anotasi ditulis ke berkas baru; berkas asli tak tersentuh sampai kamu menyimpan.</p>")});
    review.topics.append(
        {"forms", "Forms", "Formulir",
         sect("en", "<p>Fill interactive AcroForm fields - and author new ones.</p>",
              "<p>Complete existing fields, and add, move, or delete fields of your own.</p>",
              "<p>Filling reads/writes with Poppler (it regenerates appearances on save). Authoring "
              "writes the field with QPDF and a generated appearance, so it shows and fills "
              "everywhere.</p>",
              "<ul><li>Open the <b>Forms</b> tool/rail icon, edit fields, then <b>Save Form</b>.</li>"
              "<li><b>Add field</b> (the + button, or Document ▸ Add Form Field…): choose a type - "
              "Text, Check box, Dropdown, Radio group, or Push button - then draw it on the page.</li>"
              "<li>Each field row has <b>Move</b> (draw a new spot) and <b>×</b> (delete).</li></ul>",
              "<p>If the panel is empty, the PDF has no fields yet - add one. A radio group needs at "
              "least two options.</p>"),
         sect("id", "<p>Isi field AcroForm interaktif - dan buat yang baru.</p>",
              "<p>Lengkapi field yang ada, serta tambah, pindah, atau hapus field buatanmu.</p>",
              "<p>Pengisian dibaca/ditulis dengan Poppler (meregenerasi tampilan saat menyimpan). "
              "Pembuatan menulis field dengan QPDF beserta appearance, sehingga tampil dan terisi di "
              "mana saja.</p>",
              "<ul><li>Buka ikon tool/rail <b>Forms</b>, sunting field, lalu <b>Save Form</b>.</li>"
              "<li><b>Add field</b> (tombol +, atau Document ▸ Add Form Field…): pilih tipe - Text, "
              "Check box, Dropdown, Radio group, atau Push button - lalu gambar di halaman.</li>"
              "<li>Tiap baris field punya <b>Move</b> (gambar posisi baru) dan <b>×</b> (hapus).</li></ul>",
              "<p>Jika panel kosong, PDF belum punya field - tambah satu. Grup radio butuh minimal "
              "dua opsi.</p>")});
    review.topics.append(
        {"sign", "Digital signatures", "Tanda tangan digital",
         sect("en", "<p>Sign documents and verify existing signatures.</p>",
              "<p>Prove a document is authentic and unchanged.</p>",
              "<p>Signing uses a certificate from your system's NSS database via Poppler; "
              "verification reports each signature's validity.</p>",
              "<ul><li><b>Document ▸ Sign</b>: pick a certificate, reason, and location.</li><li>"
              "<b>Document ▸ Signatures</b>: review signers and validity.</li></ul>",
              "<p>Signing needs a certificate in your NSS store (<code>~/.pki/nssdb</code>); if none "
              "exist the app says so. Import a PKCS#12 certificate first.</p>"),
         sect("id", "<p>Tanda tangani dokumen dan verifikasi tanda tangan yang ada.</p>",
              "<p>Buktikan dokumen asli dan tak berubah.</p>",
              "<p>Penandatanganan memakai sertifikat dari NSS database sistemmu via Poppler; "
              "verifikasi melaporkan keabsahan tiap tanda tangan.</p>",
              "<ul><li><b>Document ▸ Sign</b>: pilih sertifikat, alasan, dan lokasi.</li><li>"
              "<b>Document ▸ Signatures</b>: tinjau penandatangan dan keabsahan.</li></ul>",
              "<p>Penandatanganan butuh sertifikat di NSS store (<code>~/.pki/nssdb</code>); jika tak "
              "ada, aplikasi memberi tahu. Impor sertifikat PKCS#12 dulu.</p>")});
    review.topics.append(
        {"ocr", "Recognize text (OCR)", "Pengenalan teks (OCR)",
         sect("en", "<p>Make a scanned PDF searchable and selectable.</p>",
              "<p>Add a hidden text layer over page images without changing how they look.</p>",
              "<p>Each page is rendered to an image, Tesseract finds the words and their boxes, and "
              "the text is written invisibly at those positions via QPDF.</p>",
              "<ul><li><b>Document ▸ Recognize Text (OCR)</b>. Pick a language, then save - the "
              "result opens with searchable text.</li></ul>",
              "<p>OCR needs Tesseract installed (with the language packs). It can be slow on long "
              "documents; accuracy depends on scan quality.</p>"),
         sect("id", "<p>Buat PDF hasil scan bisa dicari dan diseleksi.</p>",
              "<p>Tambahkan lapisan teks tersembunyi di atas gambar halaman tanpa mengubah "
              "tampilannya.</p>",
              "<p>Tiap halaman di-render jadi gambar, Tesseract menemukan kata dan kotaknya, lalu "
              "teks ditulis tak terlihat di posisi itu via QPDF.</p>",
              "<ul><li><b>Document ▸ Recognize Text (OCR)</b>. Pilih bahasa, lalu simpan - hasilnya "
              "terbuka dengan teks yang bisa dicari.</li></ul>",
              "<p>OCR butuh Tesseract terpasang (beserta paket bahasa). Bisa lambat pada dokumen "
              "panjang; akurasi tergantung kualitas scan.</p>")});
    g.append(review);

    return g;
}

const QList<Group>& docs() {
    static const QList<Group> d = buildDocs();
    return d;
}
} // namespace

DocsView::DocsView(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("DocsView"));

    const Theme::Palette& p = Theme::instance().palette();
    const auto css = [](const QColor& c) { return c.name(QColor::HexRgb); };
    setStyleSheet(
        QStringLiteral(
            "#DocsView { background:%1; }"
            "#DocsSidebar { background:%2; border-right:1px solid %3; }"
            "#DocsTitle { color:%4; font-size:15px; font-weight:700; }"
            "QTreeWidget#DocsToc { background:transparent; border:none; outline:0; }"
            "QTreeWidget#DocsToc::item { padding:5px 4px; color:%4; }"
            "QTreeWidget#DocsToc::item:selected { background:%5; color:%6; border-radius:6px; }"
            "QPushButton#DocsLang { background:transparent; border:1px solid %3; border-radius:7px;"
            " padding:4px 10px; color:%4; }"
            "QPushButton#DocsLang:checked { background:%5; border:1px solid %6; color:%6; }"
            "QTextBrowser#DocsArticle { background:%1; border:none; color:%4; }")
            .arg(css(p.canvas), css(p.surface), css(p.hairline), css(p.text), css(p.accentTint),
                 css(p.accent)));

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Sidebar: title, language toggle, table of contents.
    auto* sidebar = new QWidget(this);
    sidebar->setObjectName(QStringLiteral("DocsSidebar"));
    sidebar->setFixedWidth(268);
    auto* side = new QVBoxLayout(sidebar);
    side->setContentsMargins(16, 18, 12, 14);
    side->setSpacing(12);

    auto* title = new QLabel(tr("Documentation"), sidebar);
    title->setObjectName(QStringLiteral("DocsTitle"));
    side->addWidget(title);

    auto* langRow = new QHBoxLayout;
    m_en = new QPushButton(QStringLiteral("English"), sidebar);
    m_id = new QPushButton(QStringLiteral("Indonesia"), sidebar);
    for (QPushButton* b : {m_en, m_id}) {
        b->setObjectName(QStringLiteral("DocsLang"));
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
    }
    langRow->addWidget(m_en);
    langRow->addWidget(m_id);
    langRow->addStretch(1);
    side->addLayout(langRow);

    m_toc = new QTreeWidget(sidebar);
    m_toc->setObjectName(QStringLiteral("DocsToc"));
    m_toc->setHeaderHidden(true);
    m_toc->setIndentation(12);
    m_toc->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    side->addWidget(m_toc, 1);

    // Text-size control for the article (the default can read small).
    auto* sizeRow = new QHBoxLayout;
    sizeRow->setContentsMargins(2, 6, 2, 0);
    sizeRow->setSpacing(8);
    auto* aSmall = new QLabel(QStringLiteral("A"), sidebar);
    aSmall->setStyleSheet(QStringLiteral("font-size:11px;"));
    auto* aBig = new QLabel(QStringLiteral("A"), sidebar);
    aBig->setStyleSheet(QStringLiteral("font-size:17px; font-weight:600;"));
    m_fontSlider = new QSlider(Qt::Horizontal, sidebar);
    m_fontSlider->setRange(10, 24);
    m_fontSlider->setCursor(Qt::PointingHandCursor);
    m_fontSlider->setToolTip(tr("Text size"));
    sizeRow->addWidget(aSmall);
    sizeRow->addWidget(m_fontSlider, 1);
    sizeRow->addWidget(aBig);
    side->addLayout(sizeRow);

    root->addWidget(sidebar);

    // Article.
    auto* host = new QWidget(this);
    auto* hostCol = new QVBoxLayout(host);
    hostCol->setContentsMargins(0, 0, 0, 0);
    m_browser = new QTextBrowser(host);
    m_browser->setObjectName(QStringLiteral("DocsArticle"));
    m_browser->setOpenExternalLinks(true);
    m_browser->document()->setDocumentMargin(28);
    hostCol->addWidget(m_browser);
    root->addWidget(host, 1);

    connect(m_en, &QPushButton::clicked, this, [this] { setLanguage(QStringLiteral("en")); });
    connect(m_id, &QPushButton::clicked, this, [this] { setLanguage(QStringLiteral("id")); });
    connect(m_toc, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* it) {
        if (!it)
            return;
        const QString id = it->data(0, Qt::UserRole).toString();
        if (id.isEmpty()) { // a group header - open it
            it->setExpanded(true);
            return;
        }
        m_currentId = id;
        const bool indo = m_lang == QStringLiteral("id");
        for (const Group& gr : docs())
            for (const Topic& t : gr.topics)
                if (t.id == id)
                    m_browser->setHtml(indo ? t.bodyId : t.bodyEn);
    });

    // Restore the saved text size, apply it, then persist on change.
    m_fontPt = QSettings().value(QStringLiteral("docs/fontPt"), 13).toInt();
    m_fontSlider->setValue(m_fontPt);
    applyFontSize();
    connect(m_fontSlider, &QSlider::valueChanged, this, [this](int v) {
        m_fontPt = v;
        applyFontSize();
        QSettings().setValue(QStringLiteral("docs/fontPt"), v);
    });

    setLanguage(QLocale().language() == QLocale::Indonesian ? QStringLiteral("id")
                                                            : QStringLiteral("en"));
}

void DocsView::applyFontSize() {
    if (!m_browser)
        return;
    QFont f = m_browser->font();
    f.setPointSize(m_fontPt);
    m_browser->setFont(f);
    m_browser->document()->setDefaultFont(f); // base size HTML headings/paras inherit
}

void DocsView::setLanguage(const QString& lang) {
    m_lang = lang;
    const bool indo = lang == QStringLiteral("id");
    m_id->setChecked(indo);
    m_en->setChecked(!indo);
    rebuildToc();
}

void DocsView::rebuildToc() {
    const bool indo = m_lang == QStringLiteral("id");
    m_toc->clear();
    QTreeWidgetItem* toSelect = nullptr;
    for (const Group& gr : docs()) {
        auto* group = new QTreeWidgetItem(m_toc, {indo ? gr.titleId : gr.titleEn});
        QFont f = group->font(0);
        f.setBold(true);
        group->setFont(0, f);
        group->setFlags(group->flags() & ~Qt::ItemIsSelectable);
        group->setExpanded(true);
        for (const Topic& t : gr.topics) {
            auto* item = new QTreeWidgetItem(group, {indo ? t.titleId : t.titleEn});
            item->setData(0, Qt::UserRole, t.id);
            if (t.id == m_currentId || (m_currentId.isEmpty() && !toSelect))
                toSelect = item;
        }
    }
    if (toSelect)
        m_toc->setCurrentItem(toSelect); // also renders the article in the current language
}
