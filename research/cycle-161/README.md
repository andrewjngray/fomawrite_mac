# Cycle 161 — The PDF pane

Build 161 (`0.3.0-dev56`), 19 November 2026. Second build cycle of the PDF programme ([docs/pdf-plan.md](../../docs/pdf-plan.md), §4.3, row "161" in §5). Implemented from a written brief by Claude Opus 5.5; reviewed and verified by Claude Fable 5.2. Not installed: Phase 1 is installed after cycle 162.

## What it does
With a PDF open, the pane in the preview slot is a reader modelled on Preview:
- **Toolbar:** thumbnails · zoom out / percentage (click for actual size) / zoom in · fit width · page field with "of N" · Inspector · find. In a narrow pane, actual size, fit width, thumbnails and Inspector fold, in that order, into a "more" menu; zoom, the page field and find never fold.
- **Reading:** continuous scroll; fit width by default; 25–500% in stops; ⌘+ / ⌘− / ⌘0 with the pane focused; ⌘-wheel and pinch zoom around the pointer; the place in the document is kept across every zoom and resize; sideways scrolling when zoomed in. Page Up/Down, Space, Home/End and arrows; F6 reaches the pages.
- **Page field:** type a page number or a page label (labels win).
- **Selection:** drag selects text on one page, double-click a word, Escape or a click clears; ⌘C and Edit → Copy copy it. Selection across pages is not supported.
- **Links:** internal links jump; a web link asks first, showing the address.
- **Find:** ⌘F with the PDF pane focused (or the editor hidden) opens a find bar on the pane: matches tinted, the current one emphasised and scrolled into view, "3 of 17", Return / Shift+Return and ⌘G / ⇧⌘G, Escape closes. ⌘F with the notes editor focused still opens the editor's find. Searching runs in 12 ms slices so long documents stay responsive.
- **Thumbnails** (140 px column, remembered), **Inspector** (file, kind, version, pages, page size in cm and inches, title, author, subject, keywords, producer, creator, dates, encrypted; "–" when empty).
- **Password** prompt and **error** message in place of the pages.
- **Remembered view:** last page and zoom per PDF (up to 200 PDFs), restored on open; a `#page=N` link wins.
- **Window:** title and header show the PDF's name; the footer's view switch reads Notes / Split / PDF; the publishing-only controls (Web/PDF switch, Output Style, publishing zoom, Export) are hidden while the PDF pane is shown; a **PDF** menu exists only in PDF mode (Go to Page… ⌥⌘G, zoom items, Show/Hide Thumbnails, Show Inspector ⌥⌘I, Find in PDF…). Leaving PDF mode restores the Markdown interface exactly.

## What changed
| File | Change |
|---|---|
| `src/PdfReader.qml` (rewritten), `PdfToolbar.qml`, `PdfFindBar.qml`, `PdfThumbnails.qml`, `PdfInspector.qml` (new) | The pane. |
| `src/pdfsession.h/.cpp` | Incremental find (`startSearch`, `searchResults`, `searching`); one rectangle per text line for selections and matches (`lineRects`: Qt returns a box per glyph for PDFs that place glyphs singly, such as Qt's own `QPdfWriter` output); remembered views in app settings; `largestPageSize`; thumbnails setting; clipboard; a render-request counter for tests. |
| `src/pdfpageitem.h/.cpp` | Requests wait 50 ms and are made only while the page is `wanted` (near the viewport), because Qt's renderer cannot cancel queued pages; renders are capped at 12 million pixels (above that the image is scaled), so 500% on Retina cannot ask for hundreds of megabytes a page. |
| `src/Main.qml`, `WorkspaceHeader.qml`, `DocumentFooter.qml`, `NativeMenuBar.qml`, `LineIcon.qml` | Title, header, footer labels, find routing, F6 region, static PDF menu (`visible` bound to PDF mode, per cycle 159), Edit → Copy for the PDF selection, three icons. |
| `src/documentviewcheck.cpp`, `src/pdfacceptancecheck.inc` (new), `bin/check-document-views` | The new QML files are in the embedded-QML identity lists; the packaged app's self-check gains a `pdfChecks` group (`pdf-reader-pages-find-zoom-and-return`). |

## Verification
- `./bin/test`: **346 passed, 0 failed, 0 skipped** (335 before; 11 new in `tests/cycle161-pdf-pane.inc`), run by the reviewer. `./bin/build` passes.
- New tests: incremental find equals one-call search, returns within 150 ms with partial results on 300 dense pages (a one-call search there takes 430–470 ms), remembered views and their cap; toolbar at 320 / 420 / 600 / 900 px with no overlap and the fold order; zoom modes, stops, limits and the kept place, ⌘-wheel; keyboard navigation and go-to by number and label; selection by drag and double-click, copy by ⌘C and Edit → Copy, internal and web links (no browser is opened in tests); find routing, order, wrap and tints; thumbnails (5 items alive over 300 pages) and Inspector fields; password and error states; remembered page and zoom, `#page=N` winning; **a fast scroll through 100 pages makes 0 render requests while moving and 2 once settled** (100 without the delay and gating); footer, title, header and menu in PDF mode and the Markdown interface restored afterwards.
- Red/green: by the reviewer, reverting `WorkspaceHeader.qml` fails `pdfModeFooterTitleAndMenusRestoreMarkdown`. By the implementer, each undone in turn: render delay and gating (100 requests against 1), footer labels ("Single" ≠ "Notes"), title (`geometry.pdf.md - Fomawrite`), find routing (PDF find bar never shown), F6 region (pages never focused), per-line rectangles (3 ≠ 1).
- Self-check, headless on the built app: the PDF group passes with no QML warnings; all editor, daily-writing, zoom, pane and layout groups pass; the same five publishing groups fail headless as on every earlier build (hover and Web/PDF switching).

## Limits
- Pinch zoom is untested (no native gestures offscreen). The ⌘-wheel test checks the vertical anchor only; a page narrower than the pane stays centred.
- Text is softer above roughly 300% on Retina because of the render cap; tiled rendering would remove that.
- The thumbnails button and the window's sidebar button look alike; to be distinguished in cycle 162.
- No physical use yet: layout and feel are judged from an offscreen capture until Phase 1 is installed.

## For cycle 162
Links to PDFs from Markdown; Copy as Markdown Quote (the reader exposes `selectionPage/Start/Length/Text`); the PDF's table of contents (`backend.pdf.outline()`); session restore and crash recovery of PDF windows; Rename / Move / Trash with the notes file and the remembered view; Open dialog, drop and Open With; Windows menu (route through `win.pdfCommand(...)`, check ⌥ shortcuts against AltGr).
