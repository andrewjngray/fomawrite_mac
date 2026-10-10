# Cycle 144

This record has one section per piece of the cycle; each agent adds its own.

## Review pane

Andrew's report: the modal "Spelling and Grammar" dialog "looks awful, doesn't flow, isn't resizable". It is gone. Edit → Spelling and Grammar (⌘:) now toggles a pane at the right of the document, in Source and in Live, fed by the findings the seam already produces for the underlines (`MarkdownHighlighter::issuesInBlock`, [src/spellcheck.h](../../src/spellcheck.h)).

### What changed

| Piece | Behaviour |
| --- | --- |
| Model ([src/backend.cpp](../../src/backend.cpp)) | `Backend::reviewIssues`: every finding in the document, in order, as `{start, end, word, category, message, suggestions, context, wordOffset, blockNumber}`; offsets are document UTF-16 units. `context` is the sentence around the word (the block text when it has no sentence break), cut to about 120 characters with an ellipsis; `wordOffset` is where the word sits inside it. Capped at 500, `reviewIssuesTruncated` says so. Rebuilt 300 ms after the last text change (typing coalesces into one rebuild) and at once when the checker's setting, grammar switch, language or words change, and on `attachDocument`. A spelling finding whose word the caret ends is left out for 1.5 s after an edit (the underline withholds it too), so half-typed words do not flicker in the list. |
| Lookups | `issueAt(position)`: the first finding containing or touching the position, computed from that block now. `nextIssue(from, backwards)`: start of the next/previous finding, strictly after/before `from`, wrapping, `-1` when none. `misspelledWordAt` (the right-click menu) delegates to the same finding filtered to Spelling. |
| Pane ([src/ReviewPane.qml](../../src/ReviewPane.qml)) | Header "Review" with counts ("3 spelling · 1 grammar", "No issues", "Spelling check is off"), language popup (System language first, then `spellCheck.languages()`, bound to `spellCheck.language`) and a close button; All / Spelling / Grammar filter; a list of rows: category tag, the context with the word in bold and the category colour, the checker's message for grammar, up to five suggestion buttons, Ignore, Learn (Spelling only). Empty states: "No spelling or grammar issues", "No spelling issues", "Spelling check is off" / "Grammar check is off" each with a button that turns the checker on. Palette, dark mode, fonts and `ChromeButton`/`ToolbarButton` as the other panes. The list keeps its scroll position when it is rebuilt. |
| Jump | A row click goes to the finding: Source selects the word (`editor.select`); Live places the page's caret (`placeCursor` + `noteCaretMovedByUser` + `focusLive`, the outline's branch). Return on a keyboard-selected row does the same. |
| Fix | A suggestion runs `backend.correctWriting(start, end, word, replacement)` inside `win.performDocumentEdit`: one undoable edit, refused if the text there changed. Grammar suggestions replace the finding's span. In Live the host edit reaches the page through the existing mirror, and the page's own Undo takes it back (measured in the test). Ignore: `ignoreWord` for spelling; for grammar a new `SpellCheck::ignoreGrammar(text, message)` hides that finding for this run in the pane and the underlines. Learn writes to the system dictionary (never called by tests). |
| Layout ([src/WorkspaceLayout.qml](../../src/WorkspaceLayout.qml)) | `reviewVisible` (intent) and `reviewWidth` (default 320, 240–640) beside organizer/files; `effectiveReviewVisible`/`effectiveReviewWidth` are what the window shows. Both are in `saveState`/`restoreState` (older saved states restore as hidden, 320). The pane is the rightmost `SplitView` item (`reviewSlot`), resizable with the existing handle; a user drag is written back with `updateWidth("review", …)` like the other panes. |
| Commands ([src/WorkspaceCommands.qml](../../src/WorkspaceCommands.qml), [src/NativeMenuBar.qml](../../src/NativeMenuBar.qml)) | `spelling` is a toggle titled "Spelling and Grammar" (no ellipsis), ticked while the pane is on screen, `editSpelling`, ⌘:. New `nextIssue` (⌘;) and `previousIssue` (⌥⌘;), which move to and select the finding in Source, or place the caret in Live. All three are enabled when the checker is available. F6 includes the pane. |
| Removed | `spellingDialog`, `Backend::writingIssues`, `Backend::writingLanguages`, `macWritingIssues` (the language list is `SpellCheck::languages()`). |
| Speed | `SpellCheck::suggestions` caches its answers until the language or the words change: the model asks for every word's guesses on each rebuild, and a 500-finding document would otherwise ask the system again each time. |

### Where the pane sits relative to the footer (decision)

The footer spans from the editor pane's left edge to the window's right edge, and the view controls are pinned to `width - 12` (tests `cycle103SceneBounds(view).right() == width - 12` in the footer suites). The pane therefore sits **above** the footer, not beside it: the footer is untouched, no control moves when the pane opens or closes, and the pane stops at the footer's top (`anchors.bottomMargin: documentFooter.height`, the way the preview pane's `bottomInset` does). The alternative, ending the footer at the pane's left edge, would have moved the view controls every time the pane toggled. Nothing in the pane binds a width to a state derived from its own position.

Split plus the pane needs 480 + 320 + 240 = 1040 px; narrower than that the layout contracts to Single (the review pane is the last to give way, so the preview goes first). At the narrowest width with no room (below 320 + 240) the pane is dropped.

### Verification

New tests, [tests/cycle144-review-pane.inc](../../tests/cycle144-review-pane.inc): `reviewIssuesListSpellingAndGrammarInOrderWithDocumentOffsets`, `reviewIssuesAreCappedAndTheCapIsReported`, `reviewIssuesAreRebuiltAfterAPauseAndTypingCoalesces`, `reviewPaneTogglesWithTheCommandAndTheMenuTickFollows`, `reviewRowClickMovesTheSourceCaret`, `reviewSuggestionAppliesAsOneUndoableEdit`, `nextAndPreviousIssueWrapRoundTheDocument`, `reviewPaneResizesWithTheSplitViewAndItsWidthPersists`, `reviewPaneLeavesTheFooterControlsWhereTheyAre`, `reviewPaneFixesAndJumpsInTheLivePage`, `reviewPaneRendersInLightAndDarkThemes` (frames in `captures/`, local only). The old `writingReviewExcludesCodeAndCorrectsSafely` now tests the issue model (`issueAt`) instead of `writingIssues`. The real macOS checker is used; no test calls Learn (it writes the user's dictionary); the ignore list is process-wide, so each test that ignores a word has its own (`colage`, `wrimblesnort`), and `zxqvplumb` (ignored by a Cycle 143 test) is not reused.

Red, then green:

- Pane not wired (`reviewSlot` forced `visible: false`): 5 of the 10 behavioural tests fail, e.g. `FAIL! : reviewPaneTogglesWithTheCommandAndTheMenuTickFollows() 'slot->isVisible()' returned FALSE`, `reviewRowClickMovesTheSourceCaret() 'cycle144Row(f.window.data(), 2)' returned FALSE`, `reviewSuggestionAppliesAsOneUndoableEdit()`, `reviewPaneResizesWithTheSplitViewAndItsWidthPersists()` and `reviewPaneFixesAndJumpsInTheLivePage()` (`Totals: 7 passed, 5 failed`; the model and next/previous tests rightly pass without the pane). The footer test only became sensitive to the slot afterwards (it now asserts the slot is visible).
- Model not built (`rebuildReviewIssues` returning at once): `FAIL! : reviewIssuesListSpellingAndGrammarInOrderWithDocumentOffsets() '!f.backend.reviewIssues().isEmpty()' returned FALSE`, the cap test, the debounce test, row click, suggestion, next/previous (`Compared values are not the same`) and the Live test fail (`Totals: 5 passed, 7 failed`).
- Debounce (interval 300 ms → 10 ms): `reviewIssuesAreRebuiltAfterAPauseAndTypingCoalesces` fails with `Actual (changed.count()): 6, Expected (0)`.
- Green: all eleven pass; full `./bin/test` offscreen on this branch: **243 passed, 0 failed, 0 skipped** (274 s). The footer suites (`cycle107-footer-transitions`, `cycle134-footer-live`) are in that run, unchanged and green with the pane closed; the new footer test covers it open.

### Limits

- The shortcuts are untested by hand. On a US keyboard ⌘: *is* ⇧⌘;, so the spec's ⇧⌘; for Previous Issue would have collided with Show Spelling and Grammar; Previous Issue is ⌥⌘; instead. Whether Qt's native menu accepts `Ctrl+:` as ⌘: needs a try on a real Mac.
- Ignore on a grammar finding is per run and per window (`SpellCheck::ignoreGrammar`); the spelling ignore list is the system's, per process.
- Findings cover what the checker and the prose rules cover: the Live page still draws only spelling underlines, but the pane lists grammar for both surfaces.
- The word the caret ends is held out of the list for 1.5 s after an edit; in Live the host does not see the page caret per keystroke, so the rule uses the Source caret only.
- Not installed or hand-tested in the app (no install was made from this worktree).
