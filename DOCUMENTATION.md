# Feather PDF — User Documentation

**Light on the system, full-featured on PDF.**

Feather PDF is a native, full-featured PDF tool for Linux, powered by mature
libraries (PDFium, Poppler, QPDF, Tesseract, LibreOffice) without a web engine.
It reads, organizes, secures, annotates, fills, signs, recognizes, and creates
PDFs. Edits are lossless where possible (page structure via QPDF) and rasterized
only when safety requires it (redaction).

> This is the same content as the **in-app Help ▸ Documentation** tab, which is
> available in **English and Indonesian** with an adjustable text size. Most
> features work offline; OCR, conversion, CMYK, and signing rely on external
> tools noted in their sections.

How the workspace is laid out: open a file from **Home**, then use the **command
bar** (top), the **left rail panels** (Thumbnails, Outline, Attachments, Layers,
Forms), the **right Tools pane**, and the **menus**. A floating pill at the
bottom-centre handles page navigation and zoom.

---

## Getting started

### Overview
One app for the whole PDF lifecycle. Each capability is a thin UI over a trusted
backend, so results are predictable and (where possible) lossless.

### Opening & viewing
The viewer renders with PDFium and lays pages out instantly at any size, even for
very large documents (pages render off the UI thread into a cache, so scrolling
never blocks).
- Open from **Home**, **File ▸ Open** (Ctrl+O), drag-and-drop, or a recent file.
- Zoom with **Ctrl++/Ctrl+-**, Ctrl+scroll, or **Fit width / Fit page / Actual
  size** (View menu). Switch **page layout** between Single page, Continuous, and
  Two-up.

### Search
Full-text search across the document.
- Press **Ctrl+F** (or the search icon in the tab strip), type your query, and
  use **Enter / Shift+Enter** for next/previous; **Esc** closes it.
- If a scanned PDF finds nothing, it has no text layer — run **Recognize Text
  (OCR)** first.

### Reading mode
An immersive, distraction-free view: the chrome slides away, leaving the page and
a floating pill.
- Enter with **F11**, **View ▸ Immersive Reading**, or the book icon on the pill.
- Leave with **Esc**, **F11**, or the pill's exit icon. Click the page number on
  the pill to jump to a page.

### Preferences
App-wide settings in one place — **Edit ▸ Preferences…** (Ctrl+,).
- **Appearance**: follow the system theme, or force Light / Dark.
- **Default page layout** and **Default zoom** apply to documents you open next
  (they take effect only once you set them).
- Choices persist across launches, along with the Tools-pane contents and recent
  files.

---

## Side panels

### Thumbnails
A lazily-rendered, off-thread thumbnail of every page; click to navigate, and
**drag to reorder** pages.

### Outline
The document's bookmark/heading tree — **editable**.
- Click an entry to jump.
- Toolbar icons: **+** adds a bookmark to the current page, the pencil renames the
  selected one, **×** deletes it, and the disk icon **saves** the outline.
- Double-click an entry to rename it in place. Edits are written back into the
  `/Outlines` structure with QPDF.

### Attachments
Files embedded inside the PDF. Open the panel and **double-click** an item to save
it. Empty means the PDF carries no embedded files.

### Layers
The document's optional content groups (OCG), read-only — the PDFium viewer can't
toggle layer visibility, so the list reflects state without changing the render.

---

## Editing & assembly

### Organize pages
Lossless page operations on the open document.
- **Rotate** (Ctrl+[ / Ctrl+]), **Delete Page**, and **reorder** by dragging
  thumbnails — tracked per tab with undo/redo (Ctrl+Z / Ctrl+Y).
- **Insert Pages…** pulls pages from another PDF to a chosen position.
- **Extract Pages…** saves a page range (e.g. `1-3, 5`) to a new PDF.
- **Crop Pages…** trims margins (in mm) on all or selected pages.
- Then **Save** or **Save As**.

### Edit text
Edit the text you added with the Text tool, plus a bridge to LibreOffice Draw for
heavier edits.
- **Tools ▸ Edit text** (or Document ▸ Edit Text…) lists your text boxes — pick
  one to change its words or delete it.
- **Document ▸ Open in LibreOffice Draw** opens the PDF in Draw for heavy layout,
  body-text, and image edits Feather can't yet do natively (existing body-text
  reflow is still in development).

### Combine
Merge several PDFs into one. Add files, set their order, and save — pages are
copied losslessly with QPDF.

### Create PDF
Make a PDF from office documents (via LibreOffice) or from images. Pick the
source files and an output path.

### Print
A custom print dialog that opens instantly (printers are discovered in the
background). **File ▸ Print** (Ctrl+P): choose a printer or Print to File, page
range, odd/even subset, copies, and grayscale.

### Save & export
Write your edits to disk, in place or as a copy.
- **Save** (Ctrl+S) writes back to the current file; **Save As** (Ctrl+Shift+S),
  or the **Export** tool, writes a copy.
- Page edits (rotate/delete/reorder) save losslessly via QPDF; most other tools
  write a new file and open it, leaving the original untouched until you save.
- A dot on the tab marks unsaved page edits.

### Split
Break one PDF into several files — one per page, every N pages, or by page ranges
(`1-3, 5, 8-10`). Pages are copied losslessly into an output folder. To pull out
just a few pages, use **Extract Pages** instead.

### Document properties
View and edit the document's metadata — **Document ▸ Properties…**. Edit the
title, author, subject, keywords, and producer; page/file statistics are shown
read-only.

---

## Output & optimization

### Watermark
Stamp a text or image watermark across the pages with adjustable opacity,
rotation, and position. For an irreversible mark, **Flatten** the result.

### Bates numbering
Stamp sequential identifiers on every page (with an optional prefix/suffix and a
chosen corner) — for legal or archival sets. Run it on the final assembled
document so numbering stays continuous.

### Optimize
Rewrite the file more compactly (QPDF packs objects into object streams and
recompresses losslessly); the before/after sizes are reported. Savings vary —
already-compressed or image-heavy PDFs may barely shrink.

### RGB to CMYK
Convert the document's colours to CMYK for a commercial press (via Ghostscript,
which must be installed). Colours can shift, since CMYK has a smaller gamut than
RGB — check a proof.

### Flatten
Merge annotations, form fields, and layers into the page so they can't be edited
or removed. **Lossless** flattening bakes appearances into the page content; a
**raster** option renders each page to an image for the strongest guarantee (and
makes text unselectable). Flattening is one-way — keep the original.

---

## Security

### Protect with password
Encrypt the document (AES-256) and restrict what recipients may do.
- **File ▸ Protect…**: set an open password, and/or limit printing, copying, and
  editing.
- Restrictions are enforced with an owner password; an open password alone just
  gates opening.

### Remove password
Strip encryption from a document you can already open — **File ▸ Remove
Protection…** — writing an unprotected copy.

### Redaction
Permanently remove sensitive content (not just hide it).
- **Tools ▸ Redact**, drag boxes over the content, then apply. Right-click a box
  to remove it first.
- Redacted pages are **rasterized** so the underlying text/graphics are physically
  gone, then saved to a new file.

---

## Review & recognition

### Annotations
Highlight, sticky note, freehand ink, **underline**, **strike-through**,
**rectangle**, **line**, **arrow**, and **text boxes**.
- **Tools ▸ Comment**, pick a tool from the bar, drag on the page (Note clicks;
  Text prompts for words), choose a colour, then **Save Annotations**.
- Right-click a mark to remove it before saving. Marks get an explicit appearance
  so they render in any viewer.
- **File ▸ Form Data** exports/imports form field values as **XFDF**.

### Forms
Fill interactive AcroForm fields — and author new ones.
- Open the **Forms** tool/panel, edit fields, then **Save Form**.
- **Add field** (the + button, or Document ▸ Add Form Field…): choose a type —
  Text, Check box, Dropdown, Radio group, or Push button — then draw it on the
  page.
- Each field row has **Move** (draw a new spot) and **×** (delete). A radio group
  needs at least two options.

### Digital signatures
Sign documents and verify existing signatures.
- **Document ▸ Sign**: pick a certificate (from your system's NSS store,
  `~/.pki/nssdb`), reason, and location.
- **Document ▸ Signatures**: review signers and validity.

### Recognize text (OCR)
Turn a scanned PDF into searchable, selectable text.
- **Document ▸ Recognize Text (OCR)** (or the **Recognize text** tool): pick a
  language and save — each page is rendered, Tesseract finds the words, and an
  invisible text layer is written at those positions via QPDF.
- Needs Tesseract installed (with language packs). Accuracy depends on scan
  quality.

### Compare
See what changed between two PDFs — **Compare** tool (or Document ▸ Compare
with…). Both files are rendered and their pages compared, with differences
highlighted side by side. Comparison is visual, so heavily reflowed documents
show many differences even for small edits.

---

## Keyboard shortcuts

| Action | Shortcut |
|--------|----------|
| Open | Ctrl+O |
| Save / Save As | Ctrl+S / Ctrl+Shift+S |
| Print | Ctrl+P |
| Find | Ctrl+F |
| Preferences | Ctrl+, |
| Undo / Redo | Ctrl+Z / Ctrl+Y |
| Rotate left / right | Ctrl+[ / Ctrl+] |
| Zoom in / out | Ctrl++ / Ctrl+- |
| Reading (immersive) mode | F11 (Esc to leave) |

---

## Customizing the Tools pane

The right-hand **Tools pane** lists one-click tools. Click **Customize tools** at
its foot to check which tools appear and **drag to reorder** them; the choice is
saved. Collapse the pane with the chevron in its header.

---

## License

Feather PDF is free software under the **GNU General Public License v3**.
