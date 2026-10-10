# Cycle 147 — Harper in daily use (Andrew's first morning with build 146)

Build 147 (`0.3.0-dev43`), 11 October 2026. Andrew's findings after trying build 146 by hand, and what was done.

| What Andrew saw | Cause | Fix |
| --- | --- | --- |
| Live showed no underlines on his test document (a 5,560-word list of misspellings) while Source showed them all. | The Live page asks the host in windows of up to 4,000 characters; Harper reads a run of lines without sentence punctuation as one sentence and answers with a single readability note and no spelling (60 list lines → nothing; 10 lines → 30 spelling findings). | The host lints the page's segments **line by line** (`Backend::answerSpellingRequest`). Lines are also what the Source highlighter asks for, so the two surfaces share the cache and each line is linted once. Test `liveMarksALongListOfMisspellingsLineByLine` (300 lines; marks in Live within seconds). |
| Clicking a suggestion in the pane was slow to "move on to the next word". | The row only left the list when the document was re-checked and the list rebuilt, about 400 ms after the click (measured: 330–390 ms, no GUI stall), and nothing selected the next row. | The fixed row leaves at once (a pending set the list filters by until the rebuild lands) and the selection moves to the next row (`ReviewPane.fixNow`). Asserted in the same test: the row count drops before the rebuild and `selectedStart` is the next row's. |
| "Publishing Theme Notice: missing optional theme fonts…" modal when switching themes. | An imported Typora theme names a font file it does not ship; the app falls back and said so in a modal every time. | The quiet banner instead, once per message per session, with where to put the font file (the theme's folder, Output Style → Open Themes Folder). Errors still use the dialog. |
| "Gosh, if we could get that to include grammar." | It does: the pane header counts grammar (56 in his document) and the Grammar filter shows only those; the blue marks are Harper's grammar. | Nothing to change; said so. |
| "Where is the Claude theme?" | Output Style → Claude Like, in the same list as the others. | Nothing to change; the serif version is only for Import Theme. |

Measured on the 300-line document (offscreen): Harper ready 2.8–3.0 s after launch; the pane has its first 500 findings 0.4 s later; a fix reaches the pane in 330–390 ms with no event-loop stall; Live marks appear within seconds once the host lints by line (before: none in 30 s).

Open: the pane lists at most 500 findings (his document had more); long lists without sentence punctuation still get one readability note per line in the pane under Style; the 450 MB per window for the Harper page is unchanged.
