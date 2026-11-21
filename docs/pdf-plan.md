# PDF in Fomawrite: development programme

Status: **approved and in build. Phase 1 (cycles 160–162) is complete and installed on the Mac; cycles 163 (text markup) and 164 (note pins, navigation, unplaced list, Live chips) are complete; cycle 165 (the page-editing engine, no interface yet) is complete; build 165 is installed on the Mac and in the Windows VM (20 November); cycle 166 is available in the Mac development bundle and Windows test packages passed native CI.** See §9 for the state of the programme and how to resume. Phase 0 (decisions and spikes) is complete: [research/cycle-159/README.md](../research/cycle-159/README.md). This is version 2 of the plan: version 1 was drafted on 18 November 2026 (Claude Opus 5.5); it was reviewed against the code and the Phase 0 results and rewritten as this programme on 19 November 2026 (Claude Fable 5.2). The "Review" section says what changed and why.

**What it is.** The plan for PDF reading, Preview-style markup (highlight, underline, strikethrough, notes on the page) saved as Markdown, and page operations (rotate, reorder, delete, merge) in Fomawrite, on Mac and Windows, with Markdown remaining where Andrew writes. It holds Andrew's requirements (§1), the design (§2–§4), the cycle-by-cycle programme with acceptance tests (§5), how the work is run and verified (§6) and the risks (§7).

**How it is used.** Each cycle in §5 is built from a written brief derived from this document, reviewed, tested red/green, recorded under `research/cycle-NN/` and merged to `windows/port` at each phase end, per `AGENTS.md`. When a cycle changes the design, this document is updated in the same commit.

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

## 2. Review of version 1 (19 November 2026)

Version 1's direction holds: Markdown sidecar as the store of markup, Qt PDF for viewing, qpdf for writing, PDF tools shown only with a PDF, phases that each ship. The review changed these points.

| # | Version 1 | Now | Why |
|---|---|---|---|
| 1 | Page view built from Qt's `PdfPageImage` | Fomawrite's own page item, rendering with `QPdfDocument` | `PdfPageImage` cannot draw annotations already in a PDF and, in a list, retains every page shown: 1.7–2.2 GB after 300 pages on both platforms (cycle 159, spikes 1 and 5). |
| 2 | PDF replaces the editor; a "PDF bar" replaces the Format Bar | The PDF takes the **publishing-preview pane's slot**; its tools are a toolbar on that pane; the editor shows the notes file | Andrew chose the Markdown editor beside the PDF. The editor then still needs the Format Bar, and `WorkspaceLayout` already has the three arrangements needed (editor only, split, preview only), so no new layout machinery. |
| 3 | `Backend` left alone; a separate notes store was an option | `Backend`'s document **is** the notes file in PDF mode; `Backend::open` routes PDFs | Every way of opening a file (explorer, recents, links, Finder, session restore, back/forward) already ends in `Backend::open`; routing there makes PDFs work from all of them, and gives the notes save, undo, recovery and external-change handling unchanged. |
| 4 | Markup anchored by page geometry (`quads=`) plus a PDF hash in YAML front matter | Text markup anchored by **page text index and length, verified by the quoted text**; note pins in page view points; no hash, no front matter | Geometry is derived from the text range at draw time (spike 2), so markers are short, survive re-rendering and export exactly; the quote detects and repairs a changed PDF item by item, which a whole-file hash cannot. A one-line HTML comment replaces front matter, so nothing shows in rendered Markdown. |
| 5 | PDF menu inserted at runtime | Menu declared statically, `visible` bound to PDF mode | A native macOS menu inserted at runtime has no title (spike 3). |
| 6 | Annotated export: qpdf or a second PDFium, to be decided | qpdf | Qt PDF draws a qpdf-written highlight (spike 4); export will also write simple appearance streams for other viewers. |
| 7 | Explorer shows each PDF's page count | Kind label and notes badge only | Page count needs every PDF opened while listing a folder. |
| 8 | Phase 1 ends at a read-only viewer | Phase 1 also shows the notes file beside the PDF as ordinary Markdown, with "Copy as Markdown Quote" | It falls out of points 2 and 3 and gives a usable review workflow before anchored markup exists. |

Unchanged: the mission amendment (approved, in `MISSION.md`), the sidecar name `Report.pdf.md`, the explorer badge, no forms/signatures/text editing/redaction, qpdf for page operations, Mac and Windows parity, MuPDF excluded (AGPL), Qt's LGPL QML viewer files not copied into the source.

## 3. What exists today (verified)

- Qt PDF is linked and shipped on both platforms (`QT += pdf pdfquick`); `src/PublishingPreview.qml` renders exported PDFs with `PdfPageImage`.
- Qt PDF reads only: render (with an annotations flag), page sizes and labels, text and selections by point or by text index, links, bookmarks, passwords, eight metadata fields. The PDF version comes from the file header.
- `Backend` (`src/backend.cpp`) owns one Markdown document per window: text, undo, save, recovery, external-change detection, recents and back/forward history. `Backend::open(url)` reads any local file as text. `src/main.cpp` owns windows and sessions (`focusExistingDocument`, restore).
- `src/WorkspaceLayout.qml`: `layoutMode` 0 editor only, 1 editor + preview, 2 preview only. The preview pane is `PublishingPreview` in `src/Main.qml`.
- `FileLibrary::isTextFile` admits `md`, `markdown`, `mdown`, `txt`, `text`; `macos/Info.plist` declares Markdown and plain text.
- Menus: `src/NativeMenuBar.qml` (Mac, `Qt.labs.platform`), `src/WindowsMenuBar.qml` (branch `windows/port`, Quick Controls).
- Licensing (cycle 158): MIT app; Qt under LGPL as separate libraries; `bin/check-licenses` guards packages; `bin/tidy-mac-bundle.py` gathers licence files for every bundled Homebrew library.
- Spike results: [research/cycle-159/README.md](../research/cycle-159/README.md).

## 4. Design

### 4.1 Model

- **PDF mode.** A window is in PDF mode when its `Backend` has a PDF open (`backend.pdf.active`). The PDF is shown in the preview pane's slot; the editor (Source or Live) shows the PDF's notes file. Layout modes keep their meaning: 0 notes only, 1 notes + PDF, 2 PDF only. Opening a PDF starts in PDF only when it has no notes file, and in split when it has one.
- **`PdfSession`** (`src/pdfsession.{h,cpp}`, owned by `Backend`, exposed as `backend.pdf`): the one `QPdfDocument` for the window, read into memory so the file on disk is never held open (Windows refuses to rename or replace an open file; cycle 162), its status (ready, password needed, error), metadata, page sizes and labels, text, selection geometry by point and by text index, search, links and outline. Everything Qt PDF returns is normalised here (rectangles on rotated pages arrive un-normalised, spike 2). All geometry is in **page view points**: origin at the top-left of the page as displayed, y downwards, after the page's rotation and relative to its CropBox. No QML `PdfDocument` is used, so the file is loaded once.
- **`PdfPage`** (`src/pdfpageitem.{h,cpp}`, a `QQuickItem`): draws one page at its own size times the screen's pixel ratio, with annotations, rendering off the GUI thread (`QPdfPageRenderer`, multithreaded mode) and showing a white page until the image arrives. Each item owns exactly one image, released with the item; nothing else caches pages.
- **Opening.** `Backend::open(url)`: when `url` is a PDF, load it in `PdfSession`; if that fails, report and change nothing. Then the companion document: open `Report.pdf.md` if it exists, otherwise start an empty untitled document whose save target is `Report.pdf.md` (Save writes there without a dialog; nothing is written until Andrew saves or autosave does). Opening `Report.pdf.md` directly opens its PDF too when the PDF is beside it. Opening any other document closes the PDF. Recents, history, the explorer's current-file reveal and session restore record the **PDF's** URL in PDF mode. The existing unsaved-changes prompt runs before any of this, as for any open.
- **Explorer.** PDFs are listed with a PDF icon and kind label; `X.pdf.md` is not listed when `X.pdf` is in the same folder, and the PDF row shows a notes badge instead.
- **System integration.** macOS: `com.adobe.pdf` declared as Viewer, rank Alternate, so Fomawrite appears in Open With but is never the default; the installer's default-app step stays Markdown and plain text only. Windows: the portable build registers nothing.

### 4.2 The notes file (`Report.pdf.md`)

Plain Markdown that reads well anywhere; Fomawrite's markers are HTML comments, invisible when rendered. Fomawrite changes the file only by inserting or removing the smallest text ranges, through the editor's own edit path, so every edit is one undo step and every other byte is preserved.

```markdown
<!-- fomawrite:pdf-notes 1 -->
# Notes on Report.pdf

## [Page 1](Report.pdf#page=1)

<!-- fomawrite:pdf highlight page=1 colour=yellow text=120+22 -->
> Booking Number: B55577

Check this matches the invoice.

<!-- fomawrite:pdf note page=1 at=420,515 -->
Ask about the return date.

## [Page 3](Report.pdf#page=3)

<!-- fomawrite:pdf underline page=3 colour=red text=48+31 -->
> subject to board approval
```

- First line: `<!-- fomawrite:pdf-notes 1 -->` (format version). The title and page headings are ordinary Markdown that Andrew may edit or delete; Fomawrite adds a page heading only when it inserts the first item for a page, keeping pages in order.
- Marker grammar: `<!-- fomawrite:pdf KIND key=value … -->` on its own line. `KIND`: `highlight`, `underline`, `strike`, `note`; later `rect`, `oval`, `line`, `arrow`, `text`. Keys: `page` (1-based, as shown in the page link); `colour` (`yellow`, `green`, `blue`, `pink`, `purple` for highlights, any of those or `red` for underline and strike); `text=START+LENGTH` (character range in Qt PDF's text for that page); `at=X,Y` and `rect=X0,Y0,X1,Y1` in page view points (§4.1). Qt PDF does not expose a page's rotation or CropBox, so view points are what Fomawrite can measure exactly; when Fomawrite rotates a page (Phase 3) it rewrites these values with the page numbers, and the Phase 4 export converts them to PDF user space with qpdf, which can read both, using the spike 2 mapping. Unknown kinds and keys are kept and ignored.
- An item is its marker plus the block that follows, up to the next marker, heading or end of file. For text markup the first block is a blockquote of the marked text; anything after it is Andrew's comment. For a note, the following text is the note.
- **As built in cycle 163** (`src/pdfnotes.cpp`; the example above is produced byte for byte): a page heading names the page label when it differs from the number (`[Page iii](Report.pdf#page=3)`) and the file name is percent-encoded; the `# Notes on <file>` title is added only to empty notes; the header line goes at the very top when the first mark is inserted into notes that have none, and is not put back if deleted; pages are kept in ascending order and items in text order within a page, separated by blank lines; an item ends at the next marker or any heading; markers inside fenced or indented code are text; Windows line breaks are kept; `colour=` is written for every text mark (red for underline and strike). Removal deletes the marker and its quote while the quote still equals the page's text (otherwise only the marker; the writer's comment always stays), takes a generated page heading with the page's last item and the header line (and an untouched title) with the notes' last mark. Re-anchoring searches the same page for the quote nearest the stored start, then three pages either side. Notes holding only the header line and no marks lose that line on an insert followed by a remove (the one non-restoring case).
- **As built in cycle 164** (`src/pdfnotes.cpp`, `src/pdfmarks.cpp`): a note is `<!-- fomawrite:pdf note page=N at=X,Y -->` with X,Y integers in page view points, followed by one empty text line where the note's text goes, placed at the end of its page section; removing a note keeps its text (marker and one following blank line go); an empty note is removed whole. A note whose `at=` is outside its page, or whose page does not exist, is unplaced. Marker lines are drawn as chips in Live only; the document is not changed.
- **Drawing.** Overlays come from parsing the editor's current text, saved or not, so undo in the editor removes the highlight from the page. Text markup geometry comes from the text range (`PdfSession`); if the page's text at that range no longer equals the quote, the quote is searched for on the page and then on neighbouring pages; an item that cannot be placed is listed as "unplaced" in the PDF pane and left untouched in the file.
- **Removing** a mark from the PDF pane deletes its marker and its unedited quote; Andrew's own comment text stays in the file as plain Markdown.
- **Links.** `Report.pdf#page=3` in any Markdown document opens the PDF at that page (Source, Live and Preview); in the notes file it scrolls the PDF beside it.

### 4.3 The PDF pane (R3, R4, R8)

All PDF tools live on the pane or in a PDF menu that exists only in PDF mode; the Markdown interface is otherwise unchanged, and the Format Bar keeps working on the notes editor (hidden in PDF-only layout, as it is today whenever the editor is hidden).

- **Toolbar** (pane header, Preview's order): thumbnails toggle · zoom out · actual size · zoom in · fit width · page indicator / go to page · highlighter with menu (Yellow, Green, Blue, Pink, Purple, Underline, Strikethrough) · note · rotate left / right (Phase 3) · Inspector · search. The highlighter works both ways, as in Preview: select text then click it, or toggle it on and every selection is marked.
- **Reading:** continuous vertical scroll; pinch and ⌘+/⌘−/⌘0 zoom; Page Up/Down, Space, Home/End, arrow keys; text selection by drag, word by double-click; ⌘C copies; links (internal jump; external ask first); the PDF's table of contents as a second tab of the pane's sidebar, beside the thumbnails, as in Preview; last page and zoom per PDF kept in app settings; reload when the file changes on disk; password prompt; clear message for a damaged file.
- **Thumbnails** (left edge of the pane): page images with labels; click to go; Phase 3 adds multi-select, drag to reorder and a context menu.
- **Inspector** (popover): file name, kind, PDF version, page count, page size (cm and inches), title, author, subject, keywords, producer, creator, created, modified, encrypted.
- **Find:** ⌘F with the PDF pane focused searches the PDF (matches highlighted, next/previous, count), with the existing find bar's look.
- **Copy as Markdown Quote:** `> selected text` and `[Page n](Report.pdf#page=n)` on the clipboard.
- **Menu "PDF"** (declared statically, visible only in PDF mode, on both platforms): Highlight Text ⌃⌘H, Underline Text ⌃⌘U, Strike Through Text ⌃⌘S, Note ⌃⌘N, Copy as Markdown Quote, Go to Page…, Zoom items, Show Thumbnails, Show Inspector ⌘I (Italic's ⌘I applies only when the editor has focus), and from Phase 3 Rotate Left ⌘L, Rotate Right ⌘R, Delete Page, Insert Pages from File…, Export Selected Pages…. Windows uses Ctrl for ⌘ after a collision check on `windows/port`.
- **View switch:** the footer's Single / Split / Preview read Notes / Split / PDF in PDF mode. The window title shows the PDF's name.

### 4.4 Page operations (Phase 3, R6)

- qpdf 12 (Apache-2.0) is the only library that writes PDFs, used from one file (`src/pdfeditor.{h,cpp}`). Mac: Homebrew `qpdf`, bundled by packaging with its licence files gathered by `bin/tidy-mac-bundle.py`. Windows: vcpkg with a pinned baseline, the DLLs deployed and noticed by `bin/windows/package-manifest.py`. `bin/check-licenses` requires the qpdf notice.
- Rotate, delete, reorder and insert are **pending edits**: the page list and per-page rotation in `PdfSession` change at once (the view follows), with undo and redo, and the window shows unsaved changes. **Save** writes the new PDF to a temporary file with qpdf, verifies it opens with the expected page count, atomically replaces the original, keeps the previous bytes in Fomawrite's recovery store, reloads, and rewrites `page=` values and page headings in the notes in the same step. **Save As…** and **Export Selected Pages…** write new files.
- **Save and Undo:** Pending edits keep the saved PDF's page numbers until Save. The pane translates marks as pages move. A successful save starts a new PDF page-edit history. If it rewrites page references or pin coordinates, Source and Live start new notes histories too. A failed PDF write preserves both histories. The previous PDF bytes are kept in `<AppData>/pdf-previous/<hash of path>/` (latest three per file).
- Refused with a clear message: encrypted PDFs that qpdf cannot rewrite without the owner password; digitally signed PDFs unless Andrew confirms that the signature will be invalidated.
- Merge: Insert Pages from File…, dropping a PDF from the explorer onto the thumbnails, and later Combine for several PDFs selected in the explorer.

### 4.5 Phase 4 (each item needs Andrew's go)

Shapes, text box and speech bubble stored in the notes; **Export Annotated PDF…** (the notes' marks written as real annotations with appearance streams into a copy, via qpdf); import of annotations already in a PDF into the notes; extract page text to Markdown. Not planned: forms, signatures, PDF text editing, redaction, crop, OCR.

## 5. Programme

Cycle numbers continue the Mac series. Each cycle follows `AGENTS.md`: code, red/green tests, `research/cycle-NN/README.md`, a CHANGELOG line, STATUS. Each phase ends with a merge to `windows/port`, Windows CI on x64 and ARM64, and installs on Mac and in the Windows VM for Andrew to try.

| Cycle | Phase | Scope | Done when |
|---|---|---|---|
| 159 | 0 | Decisions, spikes, review (complete) | Record written |
| 160 | 1 | **PDF core.** `PdfSession`; `PdfPage` item; `Backend::open` routing and the companion document; explorer lists PDFs and folds notes files into a badge; a test fixture builder (C++ port of the spike's generator: text at known positions, rotations, CropBox, annotations, password, damaged). No pane yet beyond a bare page list in the preview slot. | Tests: load/refuse/password; page count, sizes, labels, text; selection geometry on all rotations and CropBox equals the spike mapping; pixels show existing annotations; a 300-page list scroll keeps resident memory bounded (the spike's measure, with a generous ceiling); opening a PDF from an unsaved Markdown document prompts; companion rules (existing notes, untitled with save target, direct notes-file open, leaving PDF mode); recents/history hold the PDF; explorer rows and badge. Full suite green. |
| 161 | 1 | **PDF pane.** `PdfReader.qml` in the preview slot: toolbar, zoom and fit, keyboard and trackpad navigation, selection and copy, links, find, thumbnails, Inspector, go to page, last page/zoom memory, reload on change, password and error states; Notes / Split / PDF switch; window title; static PDF menu (Mac). | QML tests for each control and state; mode switch leaves the Markdown interface exactly as before (Format Bar, menus, layout restored); search results and selection text match fixtures; native self-check gains a PDF group. |
| 162 | 1 | **Links, integration, Windows.** `#page=n` links from Source, Live and Preview; Copy as Markdown Quote and a context menu on the page; table-of-contents tab; session restore and crash recovery of PDF windows; Rename / Move / Trash with the notes file; Open dialog, drop and Open With on macOS; Windows menu bar entries and shortcut check; merge, Windows CI, installs. | Link and restore tests; Windows CI green on both architectures; Phase 1 installed on both machines. |
| 163 | 2 | **Notes model and text markup.** `src/pdfnotes.{h,cpp}`: parse, insert, remove, recolour as minimal text edits; overlays for highlight (five colours), underline, strikethrough; highlighter tool, colour menu and shortcuts; marks removable and recolourable from the page; edits go through the editor's programmatic edit path in Source and Live as one undo step each. | Parser/writer round trips are byte-exact for untouched text; each operation is the minimal edit and one undo; overlays match the text range's geometry on every fixture rotation; undo in the editor removes the overlay; file created only on save. |
| 164 | 2 | **Notes on the page and navigation.** Note pins (place, move, edit in the editor, popover on the page); click a mark to reveal its item in the editor and the reverse; marker chips in Live; re-anchoring by quote and the unplaced list; merge, Windows CI, installs. | Pin geometry on all rotations; move is one edit; re-anchoring fixtures (text shifted on the page, moved to another page, removed); Live shows chips, Source shows raw markers; Windows CI green. |
| 165 | 3 | **qpdf and saving.** `pdfeditor` over qpdf; build, packaging and licence notices on Mac and Windows; pending-edit model with undo; atomic save with recovery copy; refusal paths; notes remapping. | Each operation verified through Qt PDF after save (count, order by text, orientation); undo after save restores the previous bytes exactly; a failed write leaves the original untouched; notes `page=` and headings remapped; `bin/check-licenses` passes with qpdf and fails without its notice; Windows packages contain the qpdf DLLs. |
| 166 | 3 | **Page operations in the pane.** Thumbnail multi-select, drag reorder, context menu; rotate, delete, insert from file, drop a PDF from the explorer, export selected pages; menu items and shortcuts; merge, Windows CI, installs. | QML tests per operation and for unsaved-changes prompts; Windows CI green; Phase 3 installed. |
| 167+ | 4 | Per §4.5, after Andrew's decision on each item. | |

Follow-up outside the phases: move `PublishingPreview.qml` off `PdfPageImage` to the `PdfPage` item (the retention found in spike 5 probably explains the earlier 450 MB question).

## 6. How the work is run and verified

- **Who does what.** Each cycle is implemented from a written brief by a development model (Claude Opus 5.5 subagents); design, briefs, review of every diff, the red/green check, records and release steps are done by the reviewing model (Claude Fable 5.2). Andrew decides scope and tries each phase.
- **Tests.** C++ model tests (offscreen) for `PdfSession`, the notes model and `pdfeditor`; QML tests (offscreen) for the pane and mode switching; the packaged app's `--check-document-views` gains PDF groups, run headless from a scratch copy with the offscreen plug-in (cycle 158) and on Windows CI's desktop. Every behavioural test is shown failing on the previous code. Fixtures are generated in tests or are small committed samples; no private PDFs.
- **Evidence.** The full Mac suite locally (GitHub's macOS job has not been getting a runner; see STATUS), the editor tests on GitHub, and Windows CI on x64 and ARM64 for every merge.
- **Markdown must not regress.** The full suite runs on every cycle; PDF code paths are inert unless a PDF is open.

## 7. Risks

1. **`Backend` coupling.** Routing PDFs through `Backend::open` and giving an untitled document a save target touch persistence code. Mitigation: cycle 160 is that change alone with its own tests, before any interface work.
2. **Programmatic edits in Live.** Marks must be inserted through the same host-to-editor path the Review pane and formatting use, or Live and the host diverge. Cycle 163 starts by identifying and testing that path.
3. **Text-index stability.** Qt PDF's page text could differ across Qt versions; the quote check and re-anchoring cover it, and a test pins the indices for the fixtures.
4. **Large or slow PDFs.** Rendering is off the GUI thread with one image per visible page; the memory test stays in the suite.
5. **Writing PDFs.** Atomic replace, verification before replace, recovery copy, refusal for encrypted and signed files.
6. **Windows dependencies.** qpdf through vcpkg is proven on both architectures (4 minutes a build); pin the vcpkg baseline and cache it in CI.
7. **Scope.** Phase 4 items need separate decisions; nothing outside §4 is built.

## 8. Andrew's decisions (19 November 2026)

1. Mission amendment: **approved as written** (in `MISSION.md`).
2. Notes file name: **`Report.pdf.md`**.
3. Notes beside the PDF: **the existing Markdown editor (Source/Live) on the notes file**, with pins on the page as well.
4. Explorer: **one row per PDF with a notes badge**.
5. Export annotated PDF: not yet decided; stays in Phase 4.
6. Review the plan, make it a complete programme, then build it, using development models for implementation (19 November).

---

## 9. State of the programme (19 November 2026)

| Cycle | What | Status |
|---|---|---|
| 159 | Phase 0: decisions, spikes, review | Done; [record](../research/cycle-159/README.md) |
| 160 | PDF core | Done, pushed; installed on the Mac and in the Windows VM as build 163 (19 November 2026) |
| 161 | PDF pane | Done, pushed; installed on the Mac and in the Windows VM as build 163 (19 November 2026) |
| 162 | Links and integration (Phase 1 complete) | Done, pushed; installed on the Mac and in the Windows VM as build 163 (19 November 2026) |
| 163 | Notes model and text markup | Done in source on both branches: Mac `master` `279d72e` (plus test fix `8283065`); merged into `windows/port` as `1d21369`, which adds Highlight / Underline / Strike Through Text to the Windows PDF menu (no shortcuts); Windows CI run 44440728575 green on x64 and ARM64 with no re-run needed. Installed on the Mac and in the Windows VM as build 163 (19 November 2026). |
| 164 | Notes on the page and navigation: note pins, mark and notes navigation, marker chips in Live, re-anchoring and the unplaced list | Done in source on Mac `master` (build 164, `0.3.0-dev59`); installed on the Mac and in the Windows VM (19 November 2026). Windows merge: see the Windows branch `CHANGELOG.md` |
| 165 | qpdf, saving, recovery copy, notes remapping | Done in source on both branches (Mac `master` build 165, `0.3.0-dev60`; Windows merged and CI-green); no interface yet (cycle 166 adds it); installed on the Mac and in the Windows VM. [Record](../research/cycle-165/README.md) |
| 166 | Page operations in the pane | Mac development build available for hands-on review. Windows test packages passed native CI. |
| 167 onward | Phase 4 (§4.5) | Waits for Andrew's decisions |

### What remains
1. Builds 160-164 are installed on the Mac and in the Windows VM (build 164 installed 19 November 2026).
2. Cycle 165 is installed on the Mac and in the Windows VM; cycle 166 is in Mac review and Windows test packages passed native CI.
3. Cycle 166.
4. Phase 4 decisions (Andrew).
5. Known open items: Live has no click-to-follow for links; no selection across pages; pinch zoom untested; text soft above about 300% zoom; the intermittent Windows theme-file test (`publishingFolderThemesRefreshAfterExternalChanges`, "Cannot atomically edit a folder theme"), which passes on a re-run; the marks of cycle 163 and the pins, popover, flash, chips and unplaced list of cycle 164 have not been looked at on screen (including dark mode); no Qt test of a chip click in a running Live page; marks re-anchor only within 3 pages.

### How to resume
Each remaining cycle is built from its row in §5 and the design in §4 by a sub-agent on a lower-cost model (Opus or Sonnet), one cycle at a time; Fable is used only for planning. Start by reading `research/cycle-16N/README.md` of the previous cycle (for cycle 165, `research/cycle-164/README.md`), then `AGENTS.md` ("What a cycle is"). The Windows branch gets the Mac cycle by a merge (`windows/port`), as recorded in its `CHANGELOG.md`.
