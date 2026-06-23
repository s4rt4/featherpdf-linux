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

- **Custom render pipeline** — pages are laid out arithmetically and rendered
  off the UI thread into an LRU cache, so even a 4000-page document opens
  instantly and scrolls smoothly.
- **Tabbed workspace** — one tab per document, each remembering its place.
- **Home start screen** — recent files, drag-and-drop, one big Open.
- **Thumbnails & outline** — lazy off-thread thumbnails and a bookmark tree.
- **Find** — live match count with highlight and next/previous navigation.
- **Immersive reading mode** — the chrome slides away, leaving the page alone.
- **Native theming** — follows the system light/dark preference; feather-teal
  accent; symbolic icons.

## Building

Feather targets **Fedora / GNOME (Wayland)** first. On Fedora 43:

```sh
sudo dnf install -y gcc-c++ cmake ninja-build \
    qt6-qtbase-devel qt6-qtpdf-devel poppler-qt6-devel qt6-qtsvg-devel \
    clang-tools-extra

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/src/feather-pdf path/to/document.pdf
```

Requires a C++20 compiler and Qt 6.5 or newer.

## Roadmap

| Milestone | Focus |
|-----------|-------|
| **M0** | Foundation & viewer *(current)* |
| M1 | Page operations (lossless, via QPDF) |
| M2 | Security & redaction |
| M3 | Annotations |
| M4 | Forms |
| M5 | OCR |
| M6 | Signatures |
| M7 | Conversion |
| M8 | Edit existing text with reflow *(the frontier)* |
| M9 | Polish, plugins, i18n |

## License

Feather PDF is licensed under the **GNU General Public License v3.0**.
See [LICENSE](LICENSE).
