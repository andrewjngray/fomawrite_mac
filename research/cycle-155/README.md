# Cycle 155 — Formatting that works, a Word-like Format Bar, formatting marks, and "+ ▾" in the explorer

Build 155 (`0.3.0-dev51`), 4 November 2026. Andrew's requests, with screenshots from Fomawrite, iA Writer and Word:

1. "The editing buttons at the top of the screen are just not working" (B, I, link, ¶ in Live).
2. "A plus button with drop down in the top of the explorer pane for creating new documents and also new files" (iA Writer's "+ ▾").
3. "Expand the layout and formatting buttons to have things like Word has … in particular when pilcrow (¶) is pushed to show the hidden formatting on the live view."

## What was wrong with the buttons
Clicking a toolbar button moved keyboard focus to the button before its action ran, so the Live page no longer counted as focused and every formatting command did nothing. Source already kept "formatting ownership" through a click on formatting controls; Live did not. The header's link button also opened Source's link dialog, and the ¶ menu's items were enabled only in Source. A real-click test failed on build 154 (document unchanged) and passes now.

## What was built
- **Toolbar fix.** Live keeps formatting ownership while focus is on formatting controls; focus returns to the text after each command; the header link opens Live's link panel; the format menus work in Live.
- **Explorer "+ ▾".** "+" makes a new document; "▾" opens New File…, New Folder…, Sort By, View Options, Hide/Show Sort Bar, Hide/Show Filter Bar, Choose Folder…, Refresh. Narrow columns give way in order: folder arrows first (still in the Go menu), then the ▾, then the "+".
- **Format Bar** (View → Show Format Bar, on by default), a second row under the top bar, Word's Home tab in Markdown terms: Style ▾ (Body, Heading 1–6, Quote, Code Block; the label shows the current line's style), Bold, Italic, Strikethrough, Inline code, Highlight, Link, Bulleted / Numbered / Task list, Decrease / Increase indent, Clear formatting, ¶. Narrow panes fold the right-hand groups into a » menu. The bar fades with the header while writing, like the rest of the chrome, and works even when focus is elsewhere (it acts on the visible editor). F6 from Source lands on it. When the bar is hidden, the header's B / I / link / ¶ return.
- **Show formatting marks** (¶, View → Show Formatting Marks, ⌘8 as in Word for Mac): spaces as dots, tabs as arrows, ¶ at every line end including empty lines, in Live and in Source. The marks are drawn over the text: never part of the file, not selectable, not copied, and toggling them changes nothing in the document or its undo history.
- **Word controls left out on purpose:** font name and size, text colour, highlight colour choice, underline, alignment, line spacing, shading and borders. Markdown cannot store them, and the file is the truth (MISSION.md).

## Review
Three builders in parallel (explorer; toolbar and Format Bar; Live marks), then a reviewer with screenshots, then two fixers. The reviewer's findings, all fixed before install:
1. Source drew no marks (Qt Quick's TextEdit ignores Qt's show-whitespace text option); the test passed only because the bar's own ¶ highlight changed the image. Source marks are now an overlay over the visible text; the test compares the text area only.
2. Style → Code Block with a caret split the word at the caret, and nothing removed a fence. It now fences the whole paragraph; Code Block or Body inside a fence removes it; one undo.
3. Two buttons in a row on the same words garbled the Markdown (B then I gave `**beta****` in Live; Source dropped the bold). Markers now toggle and compose (B + I = `***word***`, the same button twice removes it) and Live keeps the words selected between clicks.
4. The whole bar was greyed out after opening a document from the library until the text was clicked.
5. The Style button clipped its label ("He…").
6. The bar stayed visible while the header faded during writing.
7. The narrow header's Formatting-menu button used the same ¶ glyph as the marks toggle.
8. F6 skipped the bar.
9. (Existing) dark Live showed a white band below short documents.

Tests: `tests/cycle155-new-menu.inc`, `cycle155-format-bar.inc`, `cycle155-ui-fixes.inc`, `cycle155-editing-fixes.inc`, page `test/marks.test.mjs`; each new test failed on the code before its fix.

Open: strike, code and highlight do not merge with a surrounding pair other than bold (B inside `~~word~~` nests); the Style menu shows the current style on the button, not as a tick; Cmd+8 was tested through the command path, not a physical key press.
