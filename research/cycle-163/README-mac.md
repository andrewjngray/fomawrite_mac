# Cycle 163 — Highlights, underline and strikethrough in the notes

Build 163 (`0.3.0-dev58`), 19 November 2026. First build cycle of Phase 2 of the PDF programme ([docs/pdf-plan.md](../../docs/pdf-plan.md), row "163" in §5). Built by Claude Opus 5.5 from a written brief; that agent was stopped just before its final report, so **no developer report exists**. The work was found complete in the working tree, then verified, made portable to Windows and recorded by Claude Sonnet 5.5. The programme is planned and reviewed by Claude Fable 5.2.

## What it does
- **Marking text in the PDF pane.** Select text on a page and mark it as a highlight (five colours: yellow, green, blue, pink, purple), an underline or a strikethrough (both red). The mark is written into the PDF's notes file (`Report.pdf.md`, or the untitled notes) as Markdown, and drawn on the page from the text. Nothing is written to the PDF.
- **Toolbar pen** (after the page field): clicking it marks the current selection with the current tool; with no selection it turns marking mode on and off, and every selection then made by dragging or double-click is marked at once. Its arrow menu chooses the colour, Underline or Strikethrough (remembered in the app settings). Esc leaves marking mode.
- **PDF menu** (Mac): Highlight Text (⌃⌘H), Underline Text (⌃⌘U), Strike Through Text (⌃⌘S); enabled with a selection. The test checks no other command in the window uses those shortcuts.
- **Right-click on the page.** With a selection: Highlight, Underline, Strikethrough. Over a mark: for a highlight the five colours (current one ticked) and Remove Highlight; for an underline or strikethrough, Remove Underline / Remove Strikethrough. Marking exactly the same text with the same kind again removes the mark, as in Preview.
- **One undo step each.** Marking, removing and recolouring go through the editor's own edit path in Source and in Live, each as exactly one step of undo; undo takes the mark off the page. Overlays are drawn from the editor's current text, saved or not. A PDF with untitled notes gets its notes file only when saved. Marking in PDF-only layout keeps the layout and the focus on the page.
- **Markers follow the notes.** If the notes are edited by hand (text moved, a marker deleted, headings changed, pages reordered) the page follows; a mark whose quote is no longer where it was is searched for on the page, then on the three pages either side; otherwise it is left unplaced (not drawn, untouched in the file).

## The notes text produced
A first highlight on a PDF with no notes (from `pdfNotesInsertsTheCanonicalExampleExactly`), for "Booking Number:  B55577" on page 1 at offset 120 (the quote is tidied; the length is that of the page's text, 24 here because the PDF text had two spaces and a line break):

```markdown
<!-- fomawrite:pdf-notes 1 -->
# Notes on Report.pdf

## [Page 1](Report.pdf#page=1)

<!-- fomawrite:pdf highlight page=1 colour=yellow text=120+24 -->
> Booking Number: B55577
```

A later underline on page 3 adds `## [Page 3](Report.pdf#page=3)` after it with `<!-- fomawrite:pdf underline page=3 colour=red text=48+25 -->` and its `> ` quote; the writer's comments under a quote are never touched.

## Deviations from the §4.2 format
The code writes exactly the §4.2 example. Where the plan was silent, §4.2 now records what the code does (edited in this commit): page headings use the page label when it differs from the number (`[Page iii](…#page=3)`) and a percent-encoded file name; a "# Notes on <file>" title is added only to empty notes; the header line is added at the very top the first time a mark goes into notes that have none, and is not put back if the writer deleted it; items and headings are placed in ascending page order and text order; a mark's block ends at the next marker or heading; markers inside fenced or indented code are text; removal deletes the marker and the quote only while the quote is unedited (an edited quote stays as plain Markdown), takes a generated page heading with the page's last item, and takes the header line (and an untouched title) with the notes' last mark; re-anchoring looks three pages either side. Unknown kinds and keys are kept. Notes that hold only the header line and no marks lose that line on the next insert and remove (documented, tested).

## What changed
New: `src/pdfnotes.h/.cpp` (the notes model: parse, insert, remove, recolour, place; pure functions on strings, minimal edits), `src/pdfmarks.h/.cpp` (`backend.pdfMarks`: parses the current text, places and draws marks, performs marking), `src/PdfToolMenuItem.qml`, `tests/cycle163-pdf-markup.inc`.
Changed: `src/PdfReader.qml` (overlays, marking mode, context menu), `src/PdfToolbar.qml` (pen and its menu; "of N" folds last), `src/NativeMenuBar.qml` (three PDF menu items), `src/Main.qml`, `src/backend.h/.cpp` (`applyNotesEdit`, `liveMirrorsDocument`), `src/pdfsession.h/.cpp`, `src/editorbridge.h/.cpp` (`pushIsolatedChanges`), `src/editor/src/modes.ts` and `src/editor/test/changes.test.mjs` (a host edit may be flagged as isolated in the history, so it is exactly one undo step), `src/editor/dist/editor.js` (rebuilt), `src/pdfacceptancecheck.inc` and `src/documentviewcheck.cpp` / `bin/check-document-views` (self-check), `fomawrite.pro`, `tests/tests.pro`, `src/resources.qrc`, `tests/tst_fomawrite.cpp`, `ARCHITECTURE.md`; `tests/cycle161-pdf-pane.inc` (the `Cycle161Menus` helper) and `tests/cycle162-pdf-integration.inc`, so the menu tests run on Windows too.

## Verification
- `./bin/test`: **372 passed, 0 failed, 0 skipped** (357 before; run by the planner before and by Claude Sonnet 5.5 after the portability patch and version bump; 15 new tests in `tests/cycle163-pdf-markup.inc`: 7 on the notes model on strings — parse features, hostile input, the canonical example, page and text order, remove/recolour, insert-then-remove restoring the bytes across 16 texts × 5 marks, re-anchoring — and 8 in the app: one undo step in Source and in Live, untitled notes written only by Save, overlays on every fixture rotation, marks following edits to the notes, the pen/menu/marking mode, the PDF menu items and the page's context menu, and PDF-only marking).
- Editor: **328 passed, 0 failed**; `npm run typecheck` clean; `npm run build` reproduces the committed `dist/editor.js`.
- Red/green (Claude Sonnet 5.5), each undone in turn then restored byte for byte:
  1. In `PdfNotes::removeMark` the "quote still unedited" test replaced by `true` (so an edited quote is deleted too): `pdfNotesRemoveAndRecolour` fails at `cycle163-pdf-markup.inc(295)`, "Compared values are not the same" (the edited-quote case).
  2. In `PdfNotes::defaultColour` the default for underline and strikethrough changed from red to yellow: `pdfNotesInsertsTheCanonicalExampleExactly` fails at `cycle163-pdf-markup.inc(207)`, the inserted text no longer equals the plan's example.
  The implementer's own red/green results were not reported.
- Self-check, headless (`--check-document-views`): `pdfChecks` both groups pass (`pdf-reader-pages-find-zoom-and-return`, and the new `pdf-highlight-overlay-notes-and-undo`), `qmlWarnings` empty, every editor, daily-writing, zoom, pane and layout group passes; the same five `publishingThemeChecks` groups fail headless as on every earlier build.
- The three menu tests of cycle 161 and the cycle 163 menu test now find the PDF menu through `Cycle161Menus`, in the native macOS bar or the Windows bar.

## Limits and what could not be confirmed
- **No developer report**, so anything the implementer knew and did not write down is lost. I read the tests and the diff and ran the suite; I did not read every line of `pdfnotes.cpp`, `pdfmarks.cpp` or the QML.
- **Not seen on screen.** The suite checks overlay geometry against the text range on every rotation and the pen's states through the UI items, but nobody has looked at a real pen drag, the marks' drawn thickness and offsets (underline and strike positions are heuristics from the glyph box), their colours or the marking-mode feel. Colours are fixed (the page is always white) and were not looked at in a dark window.
- Undo in Live is asserted by the tests for one step per mark and for undo taking the mark off the page; behaviour with typing interleaved by hand, and with the Live page not yet loaded (PDF-only layout: undo goes to the document; the page gets the text when shown), is as the tests assert and no more.
- Underline and strikethrough have no colour choice (red); a strikethrough over a highlight is allowed.
- Marks across a page boundary do not exist (no selection across pages, as in cycle 161).
- Only the three text-markup kinds are implemented. Notes pinned on the page (`note` markers), clicking a mark to reveal its item and the reverse, marker chips in Live, and the unplaced list in the pane are cycle 164. Unplaced marks are skipped silently for now.
- No Windows menu entries yet on this branch; the Windows merge adds them without shortcuts (Ctrl+Alt is AltGr on many layouts).
- The pen's behaviour with the Inspector, find and the toolbar's folding at very narrow widths is covered only as far as the toolbar test goes.
- The same limits as cycle 161: pinch zoom untested, text soft above about 300%.

## Not installed
`/Applications/Fomawrite.app` and the Windows VM remain build 162; build 163 exists in source and in `build/` only.
