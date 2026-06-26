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
    start.topics.append(
        {"reading-mode", "Reading mode", "Mode baca",
         sect("en", "<p>An immersive, distraction-free view of the page.</p>",
              "<p>Hide every toolbar and panel to read with just the page and a floating pill.</p>",
              "<p>The chrome slides away; the floating pill (bottom-centre) keeps page navigation, "
              "zoom, and a reading-mode toggle within reach.</p>",
              "<ul><li>Press <b>F11</b>, pick <b>View ▸ Immersive Reading</b>, or click the book icon "
              "on the floating pill.</li><li>Leave with <b>Esc</b>, <b>F11</b>, or the pill's exit "
              "icon. Click the page number on the pill to jump to a page.</li></ul>",
              "<p>The pill stays on top of the page; if it ever hides, move the mouse to the bottom "
              "of the window.</p>"),
         sect("id", "<p>Tampilan membaca yang imersif tanpa gangguan.</p>",
              "<p>Sembunyikan semua toolbar dan panel agar hanya ada halaman dan pill melayang.</p>",
              "<p>Chrome menyingkir; pill melayang (tengah-bawah) tetap menyediakan navigasi halaman, "
              "zoom, dan tombol mode baca.</p>",
              "<ul><li>Tekan <b>F11</b>, pilih <b>View ▸ Immersive Reading</b>, atau klik ikon buku di "
              "pill.</li><li>Keluar dengan <b>Esc</b>, <b>F11</b>, atau ikon keluar di pill. Klik nomor "
              "halaman di pill untuk melompat.</li></ul>",
              "<p>Pill tetap di atas halaman; jika tersembunyi, arahkan mouse ke bawah jendela.</p>")});
    start.topics.append(
        {"preferences", "Preferences", "Preferensi",
         sect("en", "<p>App-wide settings, in one place.</p>",
              "<p>Set the appearance and how new documents open, once.</p>",
              "<p>Choices are saved and restored on the next launch (theme, default layout/zoom, the "
              "Tools-pane contents, and recent files all persist).</p>",
              "<ul><li>Open <b>Edit ▸ Preferences…</b> (<b>Ctrl+,</b>).</li>"
              "<li><b>Appearance</b>: follow the system theme, or force Light / Dark.</li>"
              "<li><b>Default page layout</b> and <b>Default zoom</b> apply to documents you open "
              "next.</li></ul>",
              "<p>The default layout/zoom only take effect once you set them, so the out-of-the-box "
              "behaviour is unchanged until you choose.</p>"),
         sect("id", "<p>Pengaturan aplikasi, dalam satu tempat.</p>",
              "<p>Atur tampilan dan cara dokumen baru dibuka, sekali saja.</p>",
              "<p>Pilihan disimpan dan dipulihkan saat membuka lagi (tema, layout/zoom default, isi "
              "Tools pane, dan berkas terbaru semua persisten).</p>",
              "<ul><li>Buka <b>Edit ▸ Preferences…</b> (<b>Ctrl+,</b>).</li>"
              "<li><b>Appearance</b>: ikuti tema sistem, atau paksa Light / Dark.</li>"
              "<li><b>Default page layout</b> dan <b>Default zoom</b> berlaku untuk dokumen yang "
              "dibuka berikutnya.</li></ul>",
              "<p>Default layout/zoom baru berlaku setelah kamu mengaturnya, jadi perilaku bawaan tak "
              "berubah sampai kamu memilih.</p>")});
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
        {"links", "Hyperlinks", "Tautan",
         sect("en", "<p>Add, edit, and remove clickable links.</p>",
              "<p>Place a URL link anywhere on a page, change where existing links point, or delete "
              "them.</p>",
              "<p>Links are /Link annotations written with QPDF; a new one is mapped onto the page "
              "rotation-aware, and edits are applied in a single rewrite.</p>",
              "<ul><li><b>Document ▸ Add Link</b>, draw the clickable area, then type the URL.</li>"
              "<li><b>Document ▸ Edit Links</b> lists every link - change a URL or tick Delete, then "
              "Apply.</li></ul>",
              "<p>Links covering existing text turn that text into a clickable region; internal "
              "(go-to-page) links are listed but shown read-only.</p>"),
         sect("id", "<p>Tambah, sunting, dan hapus tautan yang bisa diklik.</p>",
              "<p>Letakkan tautan URL di mana saja pada halaman, ubah tujuan tautan yang ada, atau "
              "hapus.</p>",
              "<p>Tautan adalah anotasi /Link yang ditulis dengan QPDF; tautan baru dipetakan ke "
              "halaman dengan sadar-rotasi, dan suntingan diterapkan dalam satu penulisan ulang.</p>",
              "<ul><li><b>Document ▸ Add Link</b>, gambar area klik, lalu ketik URL-nya.</li>"
              "<li><b>Document ▸ Edit Links</b> menampilkan tiap tautan - ubah URL atau centang "
              "Delete, lalu Apply.</li></ul>",
              "<p>Tautan di atas teks menjadikan teks itu area yang bisa diklik; tautan internal "
              "(ke-halaman) ditampilkan tapi hanya-baca.</p>")});
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
        {"export", "Export to editable", "Ekspor ke dokumen",
         sect("en", "<p>Turn a PDF back into an editable document.</p>",
              "<p>Export the open PDF to Word (.docx), OpenDocument Text (.odt), Rich Text (.rtf), "
              "or plain text (.txt).</p>",
              "<p>Plain text is extracted in-process with Poppler - clean and exact. The Word/ODT/RTF "
              "formats are produced by LibreOffice's PDF import, which rebuilds an editable document "
              "from the page layout.</p>",
              "<ul><li><b>File ▸ Export to…</b> (or Tools ▸ Export). Choose the format from the Save "
              "dialog's file type and save.</li></ul>",
              "<p>Word/ODT/RTF need LibreOffice installed and approximate the original layout - "
              "spacing can shift and some lines may repeat, so review the result. Plain text needs "
              "nothing extra. Scanned PDFs without a text layer export little; run OCR first.</p>"),
         sect("id", "<p>Ubah PDF kembali menjadi dokumen yang bisa disunting.</p>",
              "<p>Ekspor PDF terbuka ke Word (.docx), OpenDocument Text (.odt), Rich Text (.rtf), "
              "atau teks biasa (.txt).</p>",
              "<p>Teks biasa diekstrak dalam proses dengan Poppler - bersih dan tepat. Format "
              "Word/ODT/RTF dibuat oleh impor PDF LibreOffice, yang menyusun ulang dokumen yang bisa "
              "disunting dari tata letak halaman.</p>",
              "<ul><li><b>File ▸ Export to…</b> (atau Tools ▸ Export). Pilih format dari jenis berkas "
              "di dialog Simpan lalu simpan.</li></ul>",
              "<p>Word/ODT/RTF butuh LibreOffice terpasang dan hanya mendekati tata letak asli - "
              "spasi bisa bergeser dan sebagian baris bisa berulang, jadi periksa hasilnya. Teks "
              "biasa tidak butuh apa-apa. PDF hasil pindai tanpa lapisan teks mengekspor sedikit; "
              "jalankan OCR dulu.</p>")});
    editing.topics.append(
        {"export-images", "Export pages as images", "Ekspor halaman jadi gambar",
         sect("en", "<p>Save pages as PNG, JPEG, or TIFF pictures.</p>",
              "<p>Render whole pages to image files at a resolution you choose.</p>",
              "<p>Pages are rendered in-process with the same engine used on screen (PDFium), so "
              "no external tools are needed; each image is flattened onto white.</p>",
              "<ul><li><b>File ▸ Export Pages as Images…</b>. Pick a format, a DPI, and an optional "
              "page range (blank = every page), then choose a folder. Files are named "
              "<i>name-01.png</i>, <i>name-02.png</i>, …</li></ul>",
              "<p>Higher DPI means sharper, larger files. The range honours any rotation, deletion, "
              "or reordering you've made this session. This renders pages as pictures; to pull out "
              "the photos already embedded in a PDF, use <b>File ▸ Extract Embedded Images…</b> "
              "(needs the pdfimages tool from Poppler).</p>"),
         sect("id", "<p>Simpan halaman sebagai gambar PNG, JPEG, atau TIFF.</p>",
              "<p>Render halaman penuh ke berkas gambar pada resolusi pilihanmu.</p>",
              "<p>Halaman di-render dalam proses dengan mesin yang sama seperti di layar (PDFium), "
              "jadi tak butuh alat luar; tiap gambar diratakan ke latar putih.</p>",
              "<ul><li><b>File ▸ Export Pages as Images…</b>. Pilih format, DPI, dan rentang halaman "
              "opsional (kosong = semua halaman), lalu pilih folder. Berkas dinamai "
              "<i>nama-01.png</i>, <i>nama-02.png</i>, …</li></ul>",
              "<p>DPI lebih tinggi berarti lebih tajam dan berkas lebih besar. Rentang menghormati "
              "rotasi, penghapusan, atau pengurutan ulang pada sesi ini. Ini me-render halaman jadi "
              "gambar; untuk mengambil foto yang sudah tertanam di PDF, pakai <b>File ▸ Extract "
              "Embedded Images…</b> (butuh alat pdfimages dari Poppler).</p>")});
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
    editing.topics.append(
        {"export", "Save &amp; export", "Simpan &amp; ekspor",
         sect("en", "<p>Write your edits to disk, in place or as a copy.</p>",
              "<p>Keep your changes - page edits, and the structural results other tools produce.</p>",
              "<p>Page edits (rotate/delete/reorder) are saved losslessly by QPDF. Most other tools "
              "write a new file and open it, leaving the original untouched until you save.</p>",
              "<ul><li><b>Save</b> (Ctrl+S) writes back to the current file; <b>Save As</b> "
              "(Ctrl+Shift+S), or the <b>Export</b> tool, writes a copy.</li>"
              "<li>A dot on the tab marks unsaved page edits.</li></ul>",
              "<p>If Save is disabled, no document is open. Encrypted originals are re-saved without "
              "their password unless you protect the copy again.</p>"),
         sect("id", "<p>Tulis suntinganmu ke disk, di tempat atau sebagai salinan.</p>",
              "<p>Simpan perubahan - suntingan halaman, dan hasil struktural dari tool lain.</p>",
              "<p>Suntingan halaman (putar/hapus/susun) disimpan lossless oleh QPDF. Kebanyakan tool "
              "lain menulis berkas baru lalu membukanya, membiarkan aslinya utuh sampai kamu "
              "menyimpan.</p>",
              "<ul><li><b>Save</b> (Ctrl+S) menulis ke berkas saat ini; <b>Save As</b> "
              "(Ctrl+Shift+S), atau tool <b>Export</b>, menulis salinan.</li>"
              "<li>Titik di tab menandai suntingan halaman yang belum disimpan.</li></ul>",
              "<p>Jika Save nonaktif, tak ada dokumen terbuka.</p>")});
    editing.topics.append(
        {"split", "Split", "Pisah",
         sect("en", "<p>Break one PDF into several files.</p>",
              "<p>Produce separate files - one per page, every N pages, or by page ranges.</p>",
              "<p>Pages are copied losslessly by QPDF into the chosen pieces, named after the source "
              "in an output folder.</p>",
              "<ul><li>Open the <b>Split</b> tool, pick a mode (each page / every N / ranges like "
              "<i>1-3, 5, 8-10</i>), and choose an output folder.</li></ul>",
              "<p>To pull out just a few pages without making many files, use <b>Extract Pages</b> "
              "(Organize pages) instead.</p>"),
         sect("id", "<p>Pecah satu PDF menjadi beberapa berkas.</p>",
              "<p>Hasilkan berkas terpisah - per halaman, tiap N halaman, atau per rentang.</p>",
              "<p>Halaman disalin lossless oleh QPDF ke potongan terpilih, dinamai sesuai sumber di "
              "folder keluaran.</p>",
              "<ul><li>Buka tool <b>Split</b>, pilih mode (per halaman / tiap N / rentang seperti "
              "<i>1-3, 5, 8-10</i>), lalu pilih folder keluaran.</li></ul>",
              "<p>Untuk mengambil beberapa halaman saja tanpa banyak berkas, pakai <b>Extract "
              "Pages</b> (Menata halaman).</p>")});
    editing.topics.append(
        {"properties", "Document properties", "Properti dokumen",
         sect("en", "<p>View and edit the document's metadata.</p>",
              "<p>See the title, author, subject, keywords, and producer, and change them.</p>",
              "<p>Metadata is read and written in the document's info dictionary; page and file "
              "statistics are shown read-only.</p>",
              "<ul><li>Open <b>Document ▸ Properties…</b>, edit the fields, and save.</li></ul>",
              "<p>Some viewers cache metadata; reopen the file to confirm changes.</p>"),
         sect("id", "<p>Lihat dan ubah metadata dokumen.</p>",
              "<p>Lihat judul, penulis, subjek, kata kunci, dan produser, lalu ubah.</p>",
              "<p>Metadata dibaca dan ditulis di info dictionary dokumen; statistik halaman dan "
              "berkas ditampilkan hanya-baca.</p>",
              "<ul><li>Buka <b>Document ▸ Properties…</b>, sunting field, lalu simpan.</li></ul>",
              "<p>Beberapa penampil menyimpan metadata di cache; buka ulang berkas untuk "
              "memastikan.</p>")});
    g.append(editing);

    // ── Output &amp; optimization ────────────────────────────────────────────
    Group output{"Output &amp; optimization", "Keluaran &amp; optimasi", {}};
    output.topics.append(
        {"watermark", "Watermark", "Watermark",
         sect("en", "<p>Stamp a text or image watermark across the pages.</p>",
              "<p>Mark a document as Draft/Confidential, or brand it with a logo.</p>",
              "<p>The mark is drawn on each page with an adjustable opacity, rotation, and position; "
              "the result is written to a new file.</p>",
              "<ul><li>Open the <b>Watermark</b> tool, choose text or an image, set opacity and "
              "placement, and save.</li></ul>",
              "<p>For an irreversible mark, <b>Flatten</b> the result so the watermark can't be "
              "removed as a layer.</p>"),
         sect("id", "<p>Bubuhkan watermark teks atau gambar di seluruh halaman.</p>",
              "<p>Tandai dokumen sebagai Draft/Rahasia, atau beri logo.</p>",
              "<p>Tanda digambar di tiap halaman dengan opasitas, rotasi, dan posisi yang bisa diatur; "
              "hasilnya ditulis ke berkas baru.</p>",
              "<ul><li>Buka tool <b>Watermark</b>, pilih teks atau gambar, atur opasitas dan posisi, "
              "lalu simpan.</li></ul>",
              "<p>Untuk tanda permanen, <b>Flatten</b> hasilnya agar watermark tak bisa dilepas "
              "sebagai lapisan.</p>")});
    output.topics.append(
        {"bates", "Bates numbering", "Penomoran Bates",
         sect("en", "<p>Stamp sequential identifiers on every page.</p>",
              "<p>Number pages for legal or archival sets, with an optional prefix and suffix.</p>",
              "<p>Each page gets a running number at a chosen corner; the numbered copy is written to "
              "a new file.</p>",
              "<ul><li>Open the <b>Bates numbering</b> tool, set the start number, digits, prefix/"
              "suffix, and position, then save.</li></ul>",
              "<p>Run it on the final assembled document so the numbering stays continuous.</p>"),
         sect("id", "<p>Bubuhkan penanda berurut di tiap halaman.</p>",
              "<p>Nomori halaman untuk berkas hukum/arsip, dengan awalan dan akhiran opsional.</p>",
              "<p>Tiap halaman mendapat nomor berjalan di sudut pilihan; salinan bernomor ditulis ke "
              "berkas baru.</p>",
              "<ul><li>Buka tool <b>Bates numbering</b>, atur nomor awal, jumlah digit, awalan/"
              "akhiran, dan posisi, lalu simpan.</li></ul>",
              "<p>Jalankan pada dokumen final agar penomoran tetap kontinu.</p>")});
    output.topics.append(
        {"optimize", "Optimize", "Optimasi",
         sect("en", "<p>Rewrite the file more compactly.</p>",
              "<p>Shrink the file size without changing how it looks.</p>",
              "<p>QPDF packs objects into object streams and recompresses streams losslessly; the "
              "before/after sizes are reported.</p>",
              "<ul><li>Open the <b>Optimize</b> tool (or Document ▸ Optimize…) and save the result."
              "</li></ul>",
              "<p>Savings vary - already-compressed or image-heavy PDFs may barely shrink. For scans, "
              "OCR doesn't reduce size.</p>"),
         sect("id", "<p>Tulis ulang berkas lebih ringkas.</p>",
              "<p>Perkecil ukuran berkas tanpa mengubah tampilannya.</p>",
              "<p>QPDF mengemas objek ke object stream dan mengompres ulang stream secara lossless; "
              "ukuran sebelum/sesudah dilaporkan.</p>",
              "<ul><li>Buka tool <b>Optimize</b> (atau Document ▸ Optimize…) lalu simpan hasilnya."
              "</li></ul>",
              "<p>Penghematan bervariasi - PDF yang sudah terkompres atau penuh gambar mungkin nyaris "
              "tak menyusut.</p>")});
    output.topics.append(
        {"cmyk", "RGB to CMYK", "RGB ke CMYK",
         sect("en", "<p>Convert the document's colours to CMYK for print.</p>",
              "<p>Prepare a file for a commercial press that expects CMYK separations.</p>",
              "<p>Conversion runs through Ghostscript with a CMYK colour profile; the converted copy "
              "is written to a new file.</p>",
              "<ul><li>Open the <b>RGB to CMYK</b> tool and save the result.</li></ul>",
              "<p>This needs Ghostscript installed. Colours can shift, as CMYK has a smaller gamut "
              "than RGB - check a proof.</p>"),
         sect("id", "<p>Ubah warna dokumen ke CMYK untuk cetak.</p>",
              "<p>Siapkan berkas untuk percetakan komersial yang memerlukan separasi CMYK.</p>",
              "<p>Konversi lewat Ghostscript dengan profil warna CMYK; salinan terkonversi ditulis ke "
              "berkas baru.</p>",
              "<ul><li>Buka tool <b>RGB to CMYK</b> lalu simpan hasilnya.</li></ul>",
              "<p>Butuh Ghostscript terpasang. Warna bisa bergeser karena gamut CMYK lebih kecil dari "
              "RGB - periksa proof.</p>")});
    output.topics.append(
        {"flatten", "Flatten", "Ratakan",
         sect("en", "<p>Merge annotations, form fields, and layers into the page.</p>",
              "<p>Make markup and filled fields permanent so they can't be edited or removed.</p>",
              "<p>Lossless flattening bakes the appearances into the page content; a raster option "
              "renders each page to an image for the strongest guarantee.</p>",
              "<ul><li>Open the <b>Flatten</b> tool, choose lossless or raster, and save.</li></ul>",
              "<p>Flattening is one-way - keep the original if you might need to edit again. Raster "
              "flattening makes text unselectable.</p>"),
         sect("id", "<p>Gabungkan anotasi, field formulir, dan layer ke halaman.</p>",
              "<p>Jadikan markup dan field terisi permanen agar tak bisa diedit atau dihapus.</p>",
              "<p>Perataan lossless membakukan tampilan ke konten halaman; opsi raster me-render tiap "
              "halaman ke gambar untuk jaminan terkuat.</p>",
              "<ul><li>Buka tool <b>Flatten</b>, pilih lossless atau raster, lalu simpan.</li></ul>",
              "<p>Perataan satu arah - simpan aslinya bila mungkin perlu diedit lagi. Perataan raster "
              "membuat teks tak bisa diseleksi.</p>")});
    g.append(output);

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
    security.topics.append(
        {"find-redact", "Find & Redact", "Cari & Redaksi",
         sect("en", "<p>Find sensitive text by pattern and redact it.</p>",
              "<p>Search the whole document for emails, phone numbers, ID/card numbers, or your own "
              "regular expression, and mark every hit for redaction.</p>",
              "<p>Text positions come from the document's own text layer; matching words are turned "
              "into redaction boxes you can review before they're applied with the normal "
              "rasterising redaction engine.</p>",
              "<ul><li><b>Document ▸ Find &amp; Redact</b>. Tick the patterns (or type a regex), press "
              "Find, review the marks, then <b>Apply Redaction</b>.</li></ul>",
              "<p>Patterns are deliberately broad - check the marks before applying, and remove any "
              "false positives by right-clicking them. A scanned PDF has no text layer, so run OCR "
              "first.</p>"),
         sect("id", "<p>Temukan teks sensitif berdasarkan pola lalu redaksi.</p>",
              "<p>Cari email, nomor telepon, nomor ID/kartu, atau ekspresi reguler buatanmu di "
              "seluruh dokumen, dan tandai tiap temuan untuk diredaksi.</p>",
              "<p>Posisi teks diambil dari lapisan teks dokumen; kata yang cocok dijadikan kotak "
              "redaksi yang bisa kamu tinjau sebelum diterapkan dengan mesin redaksi biasa.</p>",
              "<ul><li><b>Document ▸ Find &amp; Redact</b>. Centang pola (atau ketik regex), tekan "
              "Find, tinjau tanda, lalu <b>Apply Redaction</b>.</li></ul>",
              "<p>Pola sengaja dibuat luas - periksa tanda sebelum menerapkan, dan buang yang salah "
              "lewat klik-kanan. PDF hasil pindai tak punya lapisan teks, jalankan OCR dulu.</p>")});
    security.topics.append(
        {"sanitize", "Remove hidden information", "Hapus info tersembunyi",
         sect("en", "<p>Strip what isn't visible before you share.</p>",
              "<p>Remove document metadata, embedded files, and scripts that travel inside a PDF "
              "without showing on the page.</p>",
              "<p>The catalog and trailer are rewritten without the chosen pieces - the Info "
              "dictionary and XMP packet, the embedded-files name tree and file-attachment "
              "annotations, and JavaScript names plus document/page actions.</p>",
              "<ul><li><b>Document ▸ Remove Hidden Information</b>. Tick what to strip, then Clean; "
              "the cleaned copy opens and a note says how much was removed.</li></ul>",
              "<p>This is about invisible data, not visible content - to remove something you can "
              "see on the page, use Redact instead. Visible text and graphics are untouched.</p>"),
         sect("id", "<p>Buang yang tak terlihat sebelum kamu membagikan.</p>",
              "<p>Hapus metadata dokumen, berkas tertanam, dan skrip yang ikut di dalam PDF tanpa "
              "tampil di halaman.</p>",
              "<p>Katalog dan trailer ditulis ulang tanpa bagian terpilih - dictionary Info dan paket "
              "XMP, name tree berkas tertanam dan anotasi lampiran, serta nama JavaScript plus aksi "
              "dokumen/halaman.</p>",
              "<ul><li><b>Document ▸ Remove Hidden Information</b>. Centang yang ingin dibuang, lalu "
              "Clean; salinan bersih terbuka dan catatan menyebut berapa yang dihapus.</li></ul>",
              "<p>Ini soal data tak terlihat, bukan konten terlihat - untuk menghapus yang tampak di "
              "halaman, pakai Redact. Teks dan grafik terlihat tak disentuh.</p>")});
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
    review.topics.append(
        {"compare", "Compare", "Bandingkan",
         sect("en", "<p>See what changed between two PDFs.</p>",
              "<p>Spot added, removed, or moved content across two versions of a document.</p>",
              "<p>Both files are rendered and their pages compared; differences are highlighted "
              "side by side.</p>",
              "<ul><li>Open the <b>Compare</b> tool (or Document ▸ Compare with…) and pick the other "
              "PDF.</li></ul>",
              "<p>Comparison is visual - heavily reflowed documents show many differences even for "
              "small edits.</p>"),
         sect("id", "<p>Lihat apa yang berubah antara dua PDF.</p>",
              "<p>Temukan konten yang ditambah, dihapus, atau dipindah pada dua versi dokumen.</p>",
              "<p>Kedua berkas di-render dan halamannya dibandingkan; perbedaan disorot "
              "berdampingan.</p>",
              "<ul><li>Buka tool <b>Compare</b> (atau Document ▸ Compare with…) lalu pilih PDF "
              "lainnya.</li></ul>",
              "<p>Perbandingan bersifat visual - dokumen yang banyak reflow akan menampilkan banyak "
              "perbedaan walau suntingannya kecil.</p>")});
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

    applyThemeStyles();
    // Persistent widget: restyle (and re-render the article) when the theme flips.
    connect(&Theme::instance(), &Theme::changed, this, [this] {
        applyThemeStyles();
        showTopic(m_currentId);
    });

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
        showTopic(id);
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

void DocsView::applyThemeStyles() {
    const Theme::Palette& p = Theme::instance().palette();
    const auto css = [](const QColor& c) { return c.name(QColor::HexRgb); };
    setStyleSheet(
        QStringLiteral(
            "#DocsView { background:%1; }"
            "#DocsSidebar { background:%2; border-right:1px solid %3; }"
            "#DocsTitle { color:%4; font-size:15px; font-weight:700; }"
            "QTreeWidget#DocsToc { background:transparent; border:none; outline:0; }"
            "QTreeWidget#DocsToc::item { padding:5px 4px; color:%4; }"
            // Flat full-row highlight: the item AND the branch (arrow) cell share
            // one colour, so no stray box sits beside the text (same as Outline).
            "QTreeWidget#DocsToc::item:hover, QTreeWidget#DocsToc::branch:hover { background:%1; }"
            "QTreeWidget#DocsToc::item:selected, QTreeWidget#DocsToc::branch:selected"
            " { background:%5; }"
            "QTreeWidget#DocsToc::item:selected { color:%6; }"
            "QPushButton#DocsLang { background:transparent; border:1px solid %3; border-radius:7px;"
            " padding:4px 10px; color:%4; }"
            "QPushButton#DocsLang:checked { background:%5; border:1px solid %6; color:%6; }"
            "QTextBrowser#DocsArticle { background:%1; border:none; color:%4; }")
            .arg(css(p.canvas), css(p.surface), css(p.hairline), css(p.text), css(p.accentTint),
                 css(p.accent)));
}

void DocsView::showTopic(const QString& id) {
    if (!m_browser || id.isEmpty())
        return;
    const bool indo = m_lang == QStringLiteral("id");
    for (const Group& gr : docs())
        for (const Topic& t : gr.topics)
            if (t.id == id) {
                m_browser->setHtml(indo ? t.bodyId : t.bodyEn);
                applyFontSize(); // setHtml keeps the base font, but be explicit
                return;
            }
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
