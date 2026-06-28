# Feather PDF

**Light on the system, full-featured on PDF.**

Feather PDF is an open-source, native PDF application for Linux, aiming at
Adobe Acrobat–class capability without a web view. It wears Acrobat's *bones* —
a tabbed workspace, a per-document command toolbar, a contextual Tools pane —
with a quieter, modern GNOME *skin*.

> Status: early development (milestone **M0** — foundation & viewer). The core
> viewer is usable; editing milestones are on the roadmap below.

## Philosophy

Orchestrate mature libraries behind one clean façade; don't reinvent a PDF
engine.

- **Render (pixels):** PDFium, via Qt's `QtPdf`
- **Semantic layer (text, search, forms, annotations):** Poppler-Qt6
- **Lossless structure (merge, split, rotate, encrypt, metadata):** QPDF
- OCR with Tesseract · conversion with LibreOffice · shaping with HarfBuzz /
  FreeType · signing with OpenSSL

Feather deliberately avoids in-process AGPL libraries (MuPDF, Ghostscript) to
stay GPLv3.

## Highlights so far

**Reading & navigation**
- **Custom render pipeline** — pages are laid out arithmetically and rendered
  off the UI thread into an LRU cache, so even a 4000-page document opens
  instantly and scrolls smoothly.
- **Tabbed workspace** — one tab per document, each remembering its place.
- **Home start screen** — recent files, drag-and-drop, one big Open.
- **Thumbnails & outline** — lazy off-thread thumbnails and an **editable**
  bookmark tree (add / rename / delete).
- **Find** — live match count with highlight and next/previous navigation.
- **Immersive reading mode** and **native light/dark theming** (feather-teal
  accent, symbolic icons).

**Pages** — rotate, delete, drag-reorder, **insert** pages from another PDF,
**extract** a page range, and **crop** margins — all lossless via QPDF.
Combine, split, compare, **granular optimize** (audit by category, image
downsample, font unembed), watermark, Bates numbering, RGB→CMYK.

**Annotations** — highlight, sticky note, freehand ink, **underline**,
**strike-through**, **rectangle**, **line**, **arrow**, and **text boxes**,
with a comment sidebar. Import/export form data as **XFDF**.

**Forms** — fill text, checkbox, and dropdown fields, and **author** new ones
(text, checkbox, dropdown, radio, push button) by drawing them on the page;
move or delete existing fields.

**Editing** — edit the text you've added; add, edit, and remove **hyperlinks**;
an **Open in LibreOffice Draw** bridge for heavier layout work.

**Review tools** — **measure** distances, perimeters, and areas; **snapshot** a
page region to the clipboard or a PNG; a **stamp library** (Approved / Draft /
Confidential / For Review, with date and name).

**Signing** — sign with a **text or graphical (image) appearance**, verify
existing signatures (PAdES/NSS), and attach an optional **RFC 3161 trusted
timestamp**.

**More** — OCR (Tesseract) and image→PDF, with optional **deskew / despeckle /
binarize** pre-processing and **automatic language detection**; office/image
conversion (LibreOffice), **export back to Word/ODT/RTF/text** or **to
PNG/JPEG/TIFF images**, AES-256 encryption, redaction (manual or **pattern-based
Find & Redact**), **sanitize / remove hidden info**, watermarks,
**headers/footers & page numbers**, **visual or word-level compare**,
**PDF/A-1b tagging + veraPDF preflight**, and flattening. A **batch / action
wizard** composes any of these operations into a pipeline and runs it over many
files at once.

**Command line** — the same binary runs **headless** when given a sub-command,
for scripts, pipelines, and servers (no display needed):

```sh
feather-pdf merge out.pdf a.pdf b.pdf
feather-pdf extract in.pdf out.pdf --pages 1-3,5
feather-pdf encrypt in.pdf out.pdf --password secret --no-print
feather-pdf optimize in.pdf out.pdf --dpi 150
feather-pdf ocr scan.pdf searchable.pdf --lang eng
feather-pdf --help          # merge · split · rotate · watermark · bates · sanitize · images · info · …
```

## Building

Feather targets **Fedora / GNOME (Wayland)** first. On Fedora 43:

```sh
sudo dnf install -y gcc-c++ cmake ninja-build \
    qt6-qtbase-devel qt6-qtpdf-devel poppler-qt6-devel qt6-qtsvg-devel \
    qt6-qttools-devel clang-tools-extra

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/src/feather-pdf path/to/document.pdf
```

Requires a C++20 compiler and Qt 6.5 or newer.

## Roadmap

| Milestone | Focus | Status |
|-----------|-------|--------|
| M0 | Foundation & viewer | ✅ Done |
| M1 | Page operations (lossless, via QPDF) | ✅ Done |
| M2 | Security & redaction | ✅ Done |
| M3 | Annotations (+ XFDF form data) | ✅ Done |
| M4 | Forms — fill **and** author | ✅ Done |
| M5 | OCR & image→PDF | ✅ Done |
| M6 | Signatures (PAdES) | ✅ Done |
| M7 | Conversion (LibreOffice, images) | ✅ Done |
| M8 | Editing — text you added ✅ · existing-text reflow & images *(the frontier)* | ◐ In progress |
| M9 | Polish, plugins, i18n, CLI | 🔜 Next |

## License

Feather PDF is licensed under the **GNU General Public License v3.0**.
See [LICENSE](LICENSE).
