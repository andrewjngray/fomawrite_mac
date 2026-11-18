# PDF in Fomawrite: development plan (for review)

Status: **Phase 0 under way (19 November 2026); Andrew's decisions are in §8.** Nothing user-facing built yet. Drafted 18 November 2026 by Claude Opus 5.5 from Andrew Gray's requests in a working session, for review by a second model (Claude Fable 5.1) before any build cycle starts.

## How to use this document

**What it is.** A proposal for adding PDF reading, lightweight markup (Preview-style highlights, underline, strikethrough, notes) and page operations (rotate, reorder, delete, merge) to Fomawrite, with Markdown remaining the place where Andrew writes. It sets out Andrew's requirements, what the code base already offers, the decisions that shape the work (each with options and a recommendation), a phased delivery plan and the tests each phase needs.

**How it was produced.**
1. Andrew's requests and answers, collected in one conversation on 15–18 November 2026 (summarised in §1, with his words quoted where they decide something), plus four screenshots of macOS Preview: its toolbar, the highlight colour menu (Yellow, Green, Blue, Pink, Purple, Underline, Strikethrough), the Markup toolbar, the Inspector and the **Tools → Annotate** menu.
2. A survey of the repository at `master` `538e16f` (build 158): `src/PublishingPreview.qml`, `src/backend.*`, `src/filelibrary.cpp`, `src/Main.qml`, `src/NativeMenuBar.qml`, `src/FormatBar.qml`, `fomawrite.pro`, `MISSION.md`, `AGENTS.md`, `ARCHITECTURE.md`, `IDEAS.md`, and the Windows branch `windows/port`.
3. Checks against the installed Qt 6.11 headers and QML modules (Qt PDF API, render flags, metadata fields), Homebrew package data (qpdf licence and version) and the licence work of Cycle 158.

Statements are tagged **[verified]** when checked against code, headers or a run in this session, and **[assumption]** when they still need a spike. The reviewer should treat untagged design statements as proposals.

**What the reviewer is asked to do.** See §10. In short: test every [assumption], challenge each decision in §4, look for gaps against Andrew's requirements and the mission, and return a revised plan (or a list of required changes) before any code is written.

---

## 1. Andrew's requirements

From the conversation (15–18 November 2026):

| # | Requirement | Andrew's words / source |
|---|---|---|
| R1 | Open and read PDFs inside Fomawrite so it is the one place he works. "Mostly it's reviewing." | "If I'm just hanging out in this one tool, I prefer to be in this versus other tools. And so I often have to open up PDFs." |
| R2 | Markdown stays the place where he authors; interchange between human and AI tools matters. | "I want to use Markdown as the place where I author and write … the idea of interchangeability between the human interface and AI tools." |
| R3 | Lightweight markup modelled on macOS Preview, not Acrobat or Nitro: highlight in five colours, underline, strikethrough. | Preview screenshots; "the types of things you can do in preview, not Adobe Acrobat or Nitro, are probably where we should go." |
| R4 | Notes **on the page** that stay where they are put, pointing at things. | "I'd like to be able to preserve the ability to put notes on the page, though, and so that they stay … pointing to various things." |
| R5 | Highlights and notes saved as Markdown. | "having that saved down as a markdown with notes would be good." |
| R6 | Page operations: merge, rotate, reorder, delete. | Answer to the job table: "the first two": highlights/notes and merge/rotate/reorder/delete pages. |
| R7 | No form filling; no editing of PDF text for now, but keep the path open. | "I don't really want to fill in forms, and I don't really need to edit text inside the PDF, certainly not for now. I'd like to preserve the ability to move in that direction." |
| R8 | PDF functions stay separate: they appear only while a PDF is being viewed or edited. | "I would like to keep the pdf functions separate and so maybe they only pop up when i am editing or viewing a pdf." The screenshots show Preview's **Tools → Annotate** submenu as the model. |
| R9 | Start with basic, simple PDF opening, then decide what else belongs. | Opening request, 15 November. |
| R10 | Works on Mac and Windows (both versions are maintained). | Standing practice: every Mac build is merged into `windows/port`. |

Preview reference (from the screenshots), for scope: zoom out / actual size / zoom in; highlight tool with colour menu; Markup toolbar (text selection, rectangular selection, sketch, draw, shapes, text box, signature, page/text style, note); rotate; form/markup buttons; Inspector (file name, document type, PDF version, page count, page size, title, author, subject, producer, creator, creation and modification dates); search; **Tools → Annotate**: Highlight Text ⌃⌘H, Underline Text ⌃⌘U, Strike Through Text ⌃⌘S, Rectangle ⌃⌘R, Oval ⌃⌘O, Line ⌃⌘I, Arrow ⌃⌘A, Polygon, Star, Text ⌃⌘T, Speech Bubble, Mask, Loupe ⌃⌘L, Note ⌃⌘N, Signature; **Tools**: Add Bookmark ⌘D, Rotate Left ⌘L, Rotate Right ⌘R, Crop ⌘K, Redact, Show Inspector ⌘I.

## 2. Mission fit

`MISSION.md` currently says: "A personal Markdown writing app for macOS in which plain `.md` files are the only truth", and that advancing the project means "not more features" until Live is good enough to be the daily editor. PDF work conflicts with both as written.

Proposal (needs Andrew's yes before Phase 1; do not edit MISSION.md without it):

> Fomawrite writes Markdown. It also opens PDFs so that writing about them happens in one place: reading, highlighting and noting a PDF produces Markdown beside it, never hidden state. Fomawrite changes a PDF's own bytes only when Andrew explicitly asks for a page operation or an annotated export, and always through an atomic save with the previous version kept for recovery.

This keeps "the file is the truth": the notes are a Markdown file, and the PDF is touched only by explicit, reversible operations. `IDEAS.md` gets a pointer to this plan as a **Maybe** until Andrew decides (per `AGENTS.md`, a Maybe is not built).

## 3. What exists today

- **Qt PDF is already linked and shipped on both platforms** [verified]: `fomawrite.pro` has `QT += pdf pdfquick`; the Mac bundle contains `QtPdf`/`QtPdfQuick`; the Windows package contains `Qt6Pdf.dll`/`Qt6PdfQuick.dll`. `src/PublishingPreview.qml` already renders exported PDFs with `PdfDocument`, `PdfPageImage` and `PdfLinkModel`; `src/backend.cpp` uses `QPdfDocument` and `QPdfSelection` for publishing checks.
- **Qt PDF capabilities** [verified in Qt 6.11 headers and QML module]: page rendering (`QPdfDocument::render` with `QPdfDocumentRenderOptions`, including a `RenderFlag::Annotations` flag), page sizes and labels, text selection (`getSelection`, `getAllText`), search (`PdfSearchModel`), links (`PdfLinkModel`), bookmarks/outline (`QPdfBookmarkModel`), passwords, and eight metadata fields (Title, Subject, Author, Keywords, Producer, Creator, CreationDate, ModificationDate). Ready-made QML views: `PdfMultiPageView`, `PdfScrollablePageView`, `PdfPageView`.
- **Qt PDF cannot write** [verified]: no API creates or changes annotations, pages or files. Any change to a PDF needs another library (§4.3).
- **PDF version** is not exposed by Qt PDF [verified]; it can be read from the file header (`%PDF-1.x`).
- **Licensing** [verified, Cycle 158]: Fomawrite is MIT (public repository). Qt is used under LGPL v3 as separate libraries; `bin/check-licenses` fails a package with a GPL-only Qt module or a missing notice. Qt's QML viewer files (`PdfMultiPageView.qml` etc.) are LGPL-3.0/GPL source: they may be *used* as Qt ships them, but **copying and modifying them into Fomawrite's MIT source would bring LGPL code into the repository**; a custom view should be written from the `PdfPageImage`/`PdfSelection`/`PdfSearchModel` building blocks, as `PublishingPreview.qml` already does.
- **The app is built around one Markdown document per window** [verified]: `Backend` (`src/backend.cpp`, ~4,600 lines) owns one text document, its save/recovery/undo and history; `FileLibrary::isTextFile` admits only `md`, `markdown`, `mdown`, `txt`, `text`; open dialogs filter Markdown; `macos/Info.plist` declares Markdown and plain text only.
- **Contextual UI precedent** [verified]: `src/FormatBar.qml` is a second toolbar row shown under the header; `src/NativeMenuBar.qml` (Mac) and `WindowsMenuBar.qml` (Windows branch) build the menus.

## 4. Decisions

Each decision lists options, a recommendation and why. The reviewer should challenge these first.

### 4.1 Where highlights and notes live

| Option | For | Against |
|---|---|---|
| **A. Markdown sidecar beside the PDF** (recommended) | Meets R2, R5 directly; readable and editable by Andrew and AI tools; PDF untouched (mission); trivially diffable and syncable; no PDF-writing library needed for Phase 2 | Highlights are invisible to other PDF apps unless exported; anchors can break if the PDF itself changes |
| B. Real PDF annotations inside the PDF (Preview's model) | Visible in Preview/Acrobat; shareable as one file | Rewrites the PDF on every markup (mission conflict); needs a writing library from day one; notes not in Markdown unless mirrored, which creates two truths |
| C. Both, kept in sync | Best of both on paper | Two sources of truth; conflict resolution; highest complexity |

**Recommendation: A, plus an explicit "Export annotated PDF…" in Phase 4** that writes the sidecar's markup into a *copy* of the PDF as real annotations for sharing. Existing annotations already in a PDF (for example from Preview) are displayed as part of the page (Phase 1 spike: [assumption] that `PdfPageImage` renders them, or that a custom image provider with `RenderFlag::Annotations` does) and can be imported into the sidecar later (Phase 4).

### 4.2 Sidecar format

Name: `Report.pdf` → `Report.pdf.md` in the same folder [proposal; alternative `Report.notes.md`]. Created only when the first highlight or note is made; never by merely opening a PDF.

Requirements: valid, pleasant Markdown when read anywhere; each item carries enough anchor data to redraw it on the page; Andrew (or an AI) can edit the prose freely without breaking anchors; Fomawrite edits the file with minimal insertions/removals so every other byte is preserved (the existing "file is the truth" rule).

Draft:

```markdown
---
fomawrite-pdf-notes: 1
pdf: Report.pdf
pdf-sha256: 3e23ca8e…        # bytes the anchors were made against
pages: 13
---

# Notes on Report.pdf

## Page 1

<!-- fomawrite:highlight id=h7f3 page=1 colour=yellow quads=312.4,700.1,402.9,700.1,312.4,688.0,402.9,688.0 -->
> Booking Number: B55577

[Page 1](Report.pdf#page=1). Check this matches the invoice.

<!-- fomawrite:note id=n2a1 page=1 at=420.0,515.5 -->
**Note** ([Page 1](Report.pdf#page=1)): Ask about the return date.
```

- An item is the block from its `<!-- fomawrite:… -->` marker to the next marker or heading. The quote is the selected text at creation; the prose after it is Andrew's comment. HTML comments are invisible when the Markdown is rendered (Live, Preview, GitHub) and survive editing.
- Kinds: `highlight` (colours yellow, green, blue, pink, purple, matching Preview), `underline`, `strike`, `note` (a point on the page), later `rect`, `oval`, `line`, `arrow`, `text`, `bubble`.
- Coordinates: PDF user space in points, unrotated page, origin bottom-left, `quads` in the PDF `/QuadPoints` order, so export to real annotations (§4.1) is a direct mapping. [assumption: Qt PDF selection geometry is top-left origin in points; conversion needs a test per page rotation.]
- Re-anchoring: if `pdf-sha256` no longer matches, Fomawrite tries to re-find each quote's text on its page (then nearby pages); items it cannot place are listed as "unplaced" in the notes panel, never deleted.
- Page operations in Fomawrite (§4.4) rewrite `page=` values and `pdf-sha256` in the same save.
- Deep links: `[x](Report.pdf#page=3)` in *any* Markdown document opens the PDF at that page (Source, Live and Preview).

### 4.3 PDF engine

Viewing stays on **Qt PDF** (already shipped, LGPL, both platforms). For writing (page operations in Phase 3, annotated export in Phase 4):

| Option | Licence | For | Against |
|---|---|---|---|
| **qpdf** (recommended for Phase 3) | Apache-2.0 [verified: Homebrew qpdf 12.4.2] | Mature, structure-preserving page operations (rotate via `/Rotate`, delete, reorder, merge, split, extract); C++ API; keeps everything else in the file intact; MIT-compatible | Writing markup annotations means building annotation dictionaries (and possibly appearance streams) by hand; Windows ARM64 build must be produced and pinned [assumption: available via vcpkg or a CI build] |
| PDFium as a separate library (e.g. prebuilt `pdfium-binaries`) | BSD-3 / Apache-2.0 | Full annotation API (`fpdf_annot.h`) with appearance generation, plus page editing and saving; same engine Qt PDF uses | Second copy of PDFium beside Qt PDF's private one [assumption: no symbol clash on macOS two-level namespace or Windows DLLs]; large binary; pinning and updates |
| PDFKit (macOS only) | Apple system framework | Exactly Preview's behaviour | No Windows (R10) |
| MuPDF | AGPL-3.0 or commercial | Capable | Would put the whole app under AGPL; excluded |

Recommendation: **qpdf for page operations; decide the annotated-export engine (qpdf dictionaries vs PDFium) in the Phase 4 spike.** Every addition goes through `licenses/` and `bin/check-licenses`.

### 4.4 How page operations save

Preview changes pages in place on save. Proposal:
- Rotate, delete, reorder and insert/merge are **pending edits** shown immediately in the viewer (rotation and order are view state until saved), with undo/redo.
- **Save** writes a new PDF with qpdf to a temporary file and atomically replaces the original; the previous bytes go into Fomawrite's recovery store so the change can be undone after saving. The unsaved-changes prompt covers pending page edits. **Save As…** and **Export Selected Pages…** write new files.
- The sidecar's `page=` anchors and `pdf-sha256` are rewritten in the same save (one map from old to new page indices; rotation does not move user-space coordinates).
- Merge: **Insert Pages from File…**, dragging a PDF from the explorer onto the thumbnails, and (later) **Combine** on several PDFs selected in the explorer.

### 4.5 PDF mode in the interface (R8)

When the active document is a PDF, the window enters **PDF mode**; leaving it restores the Markdown interface exactly.

- **Editor area:** a continuous, scrollable page view built from `PdfPageImage` (not a modified copy of Qt's LGPL `PdfMultiPageView`, §3) with an overlay layer for highlights, underlines, strikes and note pins.
- **PDF bar** in place of the Format Bar (same slot): zoom out · actual size · zoom in · fit width; highlighter with the colour menu (Yellow, Green, Blue, Pink, Purple, Underline, Strikethrough) as in Preview; Note; rotate left/right; thumbnails toggle; Inspector (i); search. Markdown formatting controls are hidden, not disabled.
- **Menus:** a **PDF** menu (or Preview's **Tools → Annotate** arrangement) exists only in PDF mode, holding Highlight Text, Underline Text, Strike Through Text, Note, Rotate Left/Right, Delete Page, Insert Pages from File…, Export Selected Pages…, Show Inspector, Show Thumbnails. Markdown-only menus (Format, Insert…) are hidden or disabled in PDF mode. [assumption: `Qt.labs.platform` native menus can be inserted/removed at runtime on macOS; the Windows Quick menu bar can toggle visibility.]
- **Shortcuts:** use Preview's. [verified against the QML key sequences on `master`, where Qt's "Ctrl" is ⌘ and "Meta" is ⌃ on macOS] ⌃⌘H/U/S/N (markup and note), ⌘L/⌘R (rotate) and ⌘D are unused; ⌘I is Italic in Markdown, so it can mean Inspector only in PDF mode; ⌃⌘F is already full screen. Windows uses Ctrl for ⌘ and needs its own check on `windows/port` (Ctrl+H, Ctrl+R and others are common there).
- **Thumbnails sidebar** (Preview's page sidebar): page images with labels; select, multi-select, drag to reorder, context menu (rotate, delete, insert, export).
- **Inspector:** file name, type, PDF version (from the header), page count, page size (cm/in), the eight metadata fields, encrypted yes/no.
- **Notes panel:** the sidecar, listed by page; clicking an item scrolls the PDF to it; clicking a highlight on the page selects its item.
- **Find:** ⌘F uses `PdfSearchModel` in PDF mode, with the existing find bar's look.
- **Copy as quote:** with text selected, "Copy as Markdown Quote" puts `> text` plus `[Page n](Report.pdf#page=n)` on the clipboard (for pasting into any document or an AI tool).

### 4.6 Document model in the app

`Backend` is one Markdown document per window and should not be bent into a PDF model.

Recommendation: a new C++ `PdfSession` (one per window, separate from `Backend`) that owns the `QPdfDocument`, pending page edits, the save path and the sidecar anchor model; `Main.qml` gains `documentKind: "markdown" | "pdf"`. Opening a PDF goes through the existing pending-action flow (unsaved prompt for the current Markdown), joins recent files and back/forward history, and appears in the window title.

Open question for review (**the most important architectural choice**): should the notes panel *be the existing Markdown editor* bound to the sidecar? That is, in PDF mode `Backend`'s document is `Report.pdf.md` (created on the first annotation), shown in Source/Live in the side pane, so save, undo, recovery, external-change detection and AI-friendly editing all come for free, and creating a highlight inserts a block into that document as one undoable edit. The alternative is a separate, simpler notes store with its own save path. This plan leans to reusing the editor; the reviewer should test it against `Backend`'s assumptions (one document, window title, recovery keys, Live/Source switching).

### 4.7 Explorer

- PDFs are listed beside Markdown (page count where Markdown shows its first line; a PDF icon). `FileLibrary::isTextFile` stays for text, with a separate `isPdfFile` [proposal].
- `Report.pdf.md` sidecars are shown under or beside their PDF (a "notes" badge on the PDF row) rather than as unrelated documents [proposal; reviewer to consider simply listing them].
- Fomawrite is offered in "Open With" for PDFs (`macos/Info.plist` role Viewer, rank Alternate) but never made the default PDF app (the installer's default-app step for Markdown stays as is).

### 4.8 Windows

Every phase lands on Mac `master` and is merged into `windows/port`, built and tested by Windows CI (x64 and ARM64) before the phase is complete. qpdf must be pinned with checksums like the Qt archives (`windows/qt-6.11.3.json`) and built or fetched for both architectures [assumption].

## 5. Phases

Each phase is one or more cycles under `AGENTS.md` (code, red/green tests, record, CHANGELOG line, STATUS). Estimates are cycle counts, not dates.

### Phase 0: decisions and spikes (1 cycle, no user-facing change)
- Andrew decides §2 (mission text), §4.1, §4.2 file name, §4.6 notes model.
- Spikes, each with a short written result in `research/`:
  1. `PdfPageImage` / `QPdfDocument::render` shows existing annotations (Preview-made highlight and note) on Mac and Windows.
  2. Selection geometry and coordinate conversion to PDF user space for pages rotated 0/90/180/270 and with a non-zero CropBox.
  3. Runtime insertion/removal of a native **PDF** menu on macOS; visibility on Windows.
  4. qpdf: build and link on Mac (Homebrew) and Windows x64/ARM64 (vcpkg or CI), licence notice, `bin/check-licenses` update; rotate/delete/reorder/merge round trip on fixtures, verified by page text via Qt PDF.
  5. Memory and speed on a 300-page scanned PDF and a 50 MB PDF (lazy page images, cache limits).

### Phase 1: open and read (1–2 cycles)
- Open PDFs from the explorer, File → Open, drag and drop, Finder/Explorer Open With; recent files and history.
- PDF mode shell: page view, PDF bar (zoom, fit, page indicator, go to page), thumbnails (read-only), Inspector, ⌘F search, text selection and copy, links (external links confirm first), outline in the existing Outline pane, last page remembered in app settings, reload on external change, password prompt, clear error for damaged files.
- Markdown deep links `#page=n`.
- Tests: fixture PDFs (generated in-test with `QPdfWriter` for exact text, plus small committed fixtures: rotated pages, password, existing Preview annotations, damaged file); open/close/switch with an unsaved Markdown document; mode switch restores the Format Bar and menus; search hits; copy text; link activation; deep link to page; native self-check (`src/documentviewcheck.cpp`) gains a PDF-open group.

### Phase 2: highlights, underline, strikethrough and notes in Markdown (2–3 cycles)
- Highlighter tool with Preview's colour menu, underline, strikethrough, note pins; overlays drawn from the sidecar; notes panel; Copy as Markdown Quote; sidecar created on first markup; re-anchoring and "unplaced" list; delete and recolour.
- Tests: sidecar round trip is byte-exact for untouched content; creating, recolouring and deleting items are minimal edits with one undo each; anchors redraw at the same page geometry after reopen; re-anchoring after a PDF change; Live/Source rendering of the sidecar hides markers; deep links from the sidecar open the right page.

### Phase 3: page operations (2 cycles)
- Rotate left/right (selected pages), delete, reorder by dragging thumbnails, insert pages from another PDF, export selected pages; pending edits with undo; atomic save with recovery copy; sidecar anchor remapping.
- Tests: each operation verified by page count, page order (page text) and `/Rotate` via Qt PDF after save; sidecar `page=` remap; undo after save restores the exact previous bytes; failure mid-save leaves the original untouched.

### Phase 4: more markup and sharing (2+ cycles, each item decided separately)
- Shapes (rectangle, oval, line, arrow), text box, speech bubble, all stored in the sidecar.
- **Export annotated PDF…** writes the sidecar's markup as real annotations into a copy (engine decided by spike).
- Import existing PDF annotations into the sidecar.
- Extract page text to a new Markdown document.

### Not planned (R7)
Form filling, signatures, editing PDF text, redaction (a real redaction must remove content, not cover it, and is a security feature in its own right), crop, OCR. The sidecar format, the qpdf writer and `PdfSession` keep the door open to text editing later without committing to it.

## 6. Testing and verification approach

- Red/green per `AGENTS.md`: each behavioural test is shown failing on the previous code.
- Model tests (C++, offscreen): sidecar parser/writer, anchor maths, page-operation results checked through Qt PDF.
- QML tests (offscreen): PDF mode switching, bar and menu visibility, tool actions.
- Native checks: the packaged app's `--check-document-views` gains PDF groups; headless runs need Qt's offscreen platform plug-in added to a scratch copy (the packaged app ships only `cocoa`, see `research/cycle-158/README.md`).
- Windows CI runs the same Qt tests and the deployed self-check on x64 and ARM64.
- Fixture PDFs are small, generated or committed sample documents; no private PDFs (the Preview screenshots in this conversation show a private itinerary and are not committed).

## 7. Risks

1. **Scope creep against the mission** (§2): mitigated by phases with explicit go/no-go and the "Maybe" gate.
2. **`Backend` coupling** (§4.6): PDF mode must not regress Markdown save, recovery or Live; every phase runs the full suite.
3. **Anchors drifting** when PDFs are replaced by newer versions: re-anchoring by quote text, "unplaced" list, never silent loss.
4. **Large PDFs and memory**: page images must be lazy and bounded (the open "450 MB memory" question from earlier work applies).
5. **Writing PDFs safely**: qpdf preserves structure, but encrypted, signed or malformed PDFs need refusal paths (a signed PDF's signature is invalidated by any change; warn first).
6. **Licensing**: qpdf (Apache-2.0) is fine; any PDFium copy must be added to notices; MuPDF stays excluded; Qt's LGPL QML views are used unmodified or not at all.
7. **Windows ARM64 dependencies** for qpdf.
8. **Native menus changing at runtime** on macOS (spike 3).

## 8. Andrew's decisions (19 November 2026)

1. Mission amendment in §2: **approved as written** (now in `MISSION.md`).
2. Sidecar name: **`Report.pdf.md`**.
3. Notes beside the PDF: **the existing Markdown editor (Source/Live) on the sidecar**, the §4.6 recommendation, with pins on the page as well.
4. Explorer: **one row per PDF with a notes badge**; the sidecar opens with its PDF.
5. Export annotated PDF: not yet decided; stays in Phase 4.

## 9. Out of scope for this plan

Cloud sync, collaboration, OCR, PDF creation other than the existing Markdown export, e-signatures, forms.

## 10. Instructions for the reviewer

You are reviewing a plan, not code. Do not start building.

1. Read, in order: `MISSION.md`, `AGENTS.md`, `ARCHITECTURE.md`, `STATUS.md`, then this file. Skim `src/PublishingPreview.qml`, `src/backend.h`, `src/Main.qml` (document and pending-action flow), `src/FormatBar.qml`, `src/NativeMenuBar.qml`, `src/filelibrary.cpp`, `fomawrite.pro`, and on branch `windows/port` `src/WindowsMenuBar.qml` and `bin/windows/`.
2. Check every **[verified]** statement you can and every **[assumption]**; say which you confirmed, refuted or could not test.
3. Challenge §4 decision by decision. The highest-value questions: Is a Markdown sidecar the right primary store (4.1)? Is the sidecar format robust to Andrew and AI tools editing it (4.2)? Is reusing the Markdown editor for the notes pane sound given `Backend` (4.6)? Is qpdf the right writer, and what about annotated export (4.3)? Does PDF mode (4.5) meet R8 without destabilising the Markdown interface?
4. Check the plan against R1–R10: anything missing, mis-scoped or over-built?
5. Check the phases: is Phase 1 small enough to ship and use? Are the tests sufficient and red/green-able? Is anything ordered wrongly?
6. Return: (a) a list of required changes with reasons, (b) optional improvements, (c) answers or recommendations for §8, and (d) either a revised version of this document or precise edits to it. Keep Fomawrite's rules: small verifiable cycles, the file is the truth, Mac and Windows parity, MIT-compatible licences.
