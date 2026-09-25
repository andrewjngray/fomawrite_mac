# Cycle 62 — source/visual editing foundation

This cycle adds a deliberately small C++ source-mapping prototype. It does not
make the rendered preview writable and does not change `Backend`, QML, saving,
undo, recovery, or document serialization.

`SourceVisualMapping` retains the canonical `QString` source exactly and
projects supported text into a visual string using UTF-16 offsets, which are the
offsets Qt text controls use. Switching modes must use `roundTripSource()` and
must never serialize `visualText()` back to Markdown. Therefore a mode switch
does not normalize line endings, Markdown spelling, link destinations, or any
unsupported construct.

The initial editable subset is intentionally narrow: ordinary paragraphs,
ATX-heading content, list-item content, strong/emphasis content, and inline-link
labels. A source-to-visual or visual-to-source selection is valid only when all
selected units are mapped contiguously. `sourceEditForVisualReplacement()` adds
a bounded hand-off for later UI work: it returns one source replacement only
when the visual selection is wholly within one mapping and the replacement has
no newline. The caller can apply that replacement through the existing source
editor undo path; this prototype does not apply it itself.

Fenced and indented code, code spans, tables, block quotes, images, comments,
raw HTML, escaped delimiters, reference links, footnotes, and unknown protected
syntax are source-only. They remain visible in the projection with no editable
mapping, so a future visual editor must route a user back to source rather than
attempt a partial rewrite.

The focused test fixture covers headings, strong/emphasis/link labels, list
items, tables, footnotes, images, comments, fenced code, Unicode, CRLF, raw
HTML, code spans, reference links, escaped delimiters, mapping across omitted
markers, and a bounded label replacement. It verifies exact canonical source
round-trip and that the replacement leaves all source outside its returned span
unchanged.

Validation on 25 September 2026: the mapper and focused test compiled with Qt 6.11.2. `./bin/test` ran the focused mapping tests successfully; the complete run reported 103 passed and one unrelated failure in `bundledHelpIsAllowlistedAndDoesNotEditDraft`, whose recovery-file numbering differed from its prior snapshot. No native app workflow was run, because there is no UI integration in this cycle.

Known limits: this is a line-oriented prototype, not a CommonMark parser. It
does not map selections across inline markers or blocks, does not handle IME,
clipboard, drag/drop, or multi-line visual edits, and does not wire selection or
undo to `Backend`. Those limits are deliberate guards for Cycles 64–65.

Final integration on 25 September passed 111/111 tests, including the mapping fixtures. The earlier environment-sensitive failures above were checkpoints, not final failures. The Dev pass for Cycles 64–69 used this mapping on a synthetic sample.
