# Cycle 144 — grammar as you type

Grammar findings from the shared checker (`SpellCheck::issues`, see [src/spellcheck.h](../../src/spellcheck.h)) are underlined as the writer types. This file has one section per piece; other agents add theirs.

## Source grammar

Sentence-level grammar findings (the macOS grammar pass: doubled words and the like) get a blue underline in the Source editor, like the red spelling mark, and a right-click on one shows the checker's own message and corrections.

| Piece | Behaviour |
| --- | --- |
| Mark | `MarkdownHighlighter::highlightSpelling` draws each Grammar finding as a single underline in `#1a73e8` plus a pale wash (`#d9e7fb` light, `#243a5c` dark; `m_grammarBackground`, set in `rebuildFormats` beside `m_spellBackground`), composed onto the syntax format. Qt Quick draws no wave and takes the underline colour only when the run has a background (Cycle 143 finding), so grammar uses the same straight underline plus wash as spelling. Marked runs carry `SpellMarkProperty`, `SpellWordEndProperty` (the range's block-relative end) and the new `SpellCategoryProperty` (1 spelling, 2 grammar), so the caret rule restores the right colours. |
| Where it draws | Prose only, by the same gates spelling uses (fences, code style, front matter, indented code after a blank line, blanked inline code, URLs, tags, math). `grammarIssuesInBlock(block)` is the one list; `issuesInBlock` (spelling then grammar, in document order) is built from it. In focus mode a block outside the focus range, and characters outside a partial focus range, get no wash. |
| Spelling wins | Where a spelling range and a grammar range overlap, the characters belong to spelling: red, no blue. The grammar range is drawn first minus those characters. |
| Caret rule | A grammar range that ends exactly at the caret is withheld (the sentence being typed), same as a spelling word. `setCaret` / `applyCaretRule` edit the block's layout formats directly; nothing calls `rehighlightBlock`, so `QTextDocument::revision()` does not move (record: Cycle 143, F1). |
| Setting | Follows `SpellCheck::grammarEnabled` (and the master `enabled`): off clears blue, keeps red. |
| Right-click | `Backend::grammarIssueAt(position)` returns `{start, end, word, message, suggestions}` for a Grammar finding at the position (from `grammarIssuesInBlock`), and nothing on a character spelling owns. The Source `MouseArea` tries `misspelledWordAt` first, then `grammarIssueAt`. A grammar hit opens `grammarContextMenu` (a sibling of `spellingContextMenu`): the message as a disabled first line (`grammarMessage`), up to five corrections (`grammarSuggestion0..4`, applied through `backend.correctWriting` inside `win.performDocumentEdit`, one undoable edit), a separator, and Ignore Grammar Issue (`grammarIgnore`). No Learn: a sentence is not a word. |
| Ignore | The macOS checker has no per-issue ignore, so `MarkdownHighlighter::ignoreIssue(block, start, end)` (reached through `Backend::ignoreGrammarIssue(start, end)`) records the finding in a per-highlighter set keyed by block number, range and the text of the range; `grammarIssuesInBlock` skips it, so the underline, the right-click and `issuesInBlock` all drop it. The set lives on the highlighter, which `Backend::attachDocument` recreates, so it is cleared on document attach (`clearIgnoredIssues()` also exists). **Restyle path chosen: the direct layout-format path, not `rehighlightBlock`.** The ignored finding's marks are stripped from the block's formats in place (the same commit-and-`updateBlock` tail as `applyCaretRule`). `rehighlightBlock` would have been acceptable for a user action, but it bumps the document revision; the direct path is a dozen lines, shares its tail with the caret rule, and leaves the revision alone (asserted by the test). |
| Test seam | `setGrammarChecker(fn)` replaces the service's grammar pass with a bare function (like `setSpellChecker`), because the macOS pass only flags doubled words and cannot produce a sentence range over a misspelling on demand. Used by the overlap test only. |

### Contrast

The editor's text on the two washes (WCAG relative luminance, computed in the test and logged):

| Text on wash | Ratio |
| --- | --- |
| light `#34363A` on `#d9e7fb` | 9.67 |
| light `#222324` on `#d9e7fb` | 12.57 |
| light custom theme `#342f27` on `#d9e7fb` | 10.6 |
| dark `#ECEEF2` on `#243a5c` | 9.85 |
| dark `#eeeeee` on `#243a5c` | 9.86 |

All above 4.5:1. The highlighter's secondary colours are lower on the wash but unchanged in kind from how they read on white: marker grey `#68707B` on `#d9e7fb` is 4.0 (5.1 on white); the non-macOS link colour `#2077b2` is 3.86 (4.4 on white). The macOS link colours pass (`#244f88` 6.58 light, `#8eb5f0` 5.46 dark). The underline `#1a73e8` is 4.51 on white and 4.22 on `#101010`; it is a mark, not text.

### Verification

Tests in [tests/cycle144-source-grammar.inc](../../tests/cycle144-source-grammar.inc), all with the real macOS checker (the doubled word "the the" and the misspelling "recieve"), except the overlap, which stubs the findings:

- `sourceGrammarUnderlinesDoubledWordsAndSpellingStaysRed`: "the the" is blue (range [11, 18) in the test sentence, message "The word 'the' may be inadvertently doubled."), "recieve" in the same block stays red, nothing inside a fence, composes onto a heading's bold, the dark wash is swapped in, spelling wins on an overlap (stub), contrast above 4.5:1.
- `sourceGrammarFollowsItsSettingAndKeepsSpelling`: grammar off clears blue and keeps red; on restores; the master switch off clears both.
- `sourceGrammarWithholdsTheRangeAtTheCaretWithoutARevision`: withheld at the range end, drawn one short and one past, restored on leaving the block with the right colours, `document.revision()` unchanged, focus mode.
- `sourceGrammarUnderlineIsBlueInTheRenderedWindow`: frame of the real window, 1546 bluish pixels under "the the", 0 under a clean word, 0 red there; 0 with the caret at the range end, back after, 0 with grammar off; the revision unchanged. Frames go to `research/cycle-144/captures/` (git-ignored).
- `sourceGrammarContextMenuShowsTheMessageCorrectsAndIgnores`: a spelling hit still opens the spelling menu, a clean word opens neither, the grammar range opens the menu with the message (disabled) and the correction "the" (the checker's only one), no Learn; choosing it fixes the text, Undo restores it; Ignore clears the mark and `grammarIssueAt` with no revision bump and leaves the spelling mark alone; with grammar off nothing opens.
- `sourceGrammarIgnoreIsPerDocumentAndRestylesInPlace`: ignore hides one finding, not the other block's; survives a restyle; a new highlighter starts empty; `clearIgnoredIssues` restores.

Red then green: with the grammar drawing loop stubbed (the loop iterating an empty list) and the QML grammar branch of the right-click disabled, 2 passed, 6 failed (the six tests above, e.g. `'cycle144IsBlue(document, position)' returned FALSE. (blue at 0)`, `no blue under the doubled word (0)`, `menu->property("visible").toBool()' returned FALSE`); restored, 8 passed, 0 failed (the six plus init and cleanup). Full `./bin/test` offscreen: 238 passed, 0 failed.

### Limits

- The macOS grammar pass, as run here, flags doubled words; it did not flag agreement or capitalisation in the sentences tried. The sentence-level plumbing (a range over a misspelled word) is tested with a stub, not the real checker.
- Ignore is per document attach, keyed by block number and text, so editing text above the finding shifts block numbers and the ignore no longer matches (the finding reappears; ignore again).
- The message is one disabled menu line, elided at the menu's width (320 px).
- Straight underline with a wash, not a wave (as spelling).
- Not hand-tested in an installed app.

# Cycle 144

Other sections of this record are written by the agents that built the other pieces.

## Live grammar

Grammar findings get a blue wavy underline in the Live page as the writer types, next to the red spelling ones. A right-click on a grammar range shows the checker's message, its corrections and "Ignore Grammar Issue". Spelling behaviour is unchanged.

# Cycle 144

This record has one section per piece of the cycle; each agent adds its own.

## Review pane

Andrew's report: the modal "Spelling and Grammar" dialog "looks awful, doesn't flow, isn't resizable". It is gone. Edit → Spelling and Grammar (⌘:) now toggles a pane at the right of the document, in Source and in Live, fed by the findings the seam already produces for the underlines (`MarkdownHighlighter::issuesInBlock`, [src/spellcheck.h](../../src/spellcheck.h)).

### What changed

| Piece | Behaviour |
| --- | --- |
| Contract ([src/editor/README.md](../../src/editor/README.md)) | `spellingReply(token, rangesJson)` entries are now `{from, to, category, message?, suggestions?}`. `category` is `"Spelling"` (entry is only `{from, to, category}`) or `"Grammar"` (with the checker's `message` and `suggestions`). An entry with no category is spelling, so the old shape `{from, to}` still works; any other category is dropped. No new slot: `checkSpelling`, `suggestionsReply`, `learnWord`, `ignoreWord`, `setSpellCheck` are as they were. |
| Host ([src/backend.cpp](../../src/backend.cpp), bridge-connection block only) | The `spellingRequested` handler calls `m_spellCheck.issues(segment.text)` instead of `misspellings`, adds the segment's `from`, and emits the richer JSON (message and suggestions for Grammar only). `grammarEnabledChanged` now pushes the setting (`setSpellCheck`) like the other settings, which the page treats as "check again"; with grammar off `issues()` returns spelling only, so the blue marks disappear at that re-check. |
| Marks ([src/editor/src/spelling.ts](../../src/editor/src/spelling.ts)) | `fw-misspelled` unchanged; `fw-grammar` is a second mark whose decoration spec carries `{message, suggestions}`, so the menu needs no host round trip and the data maps through edits with the mark. Edits drop a grammar mark by the same rule as a spelling one. A reply replaces both kinds in its window. |
| Caret rule | A grammar range the empty caret ends is withheld, and checked 50 ms after the caret leaves, exactly like a word (`withholdCaretWord` is generic over the finding). |
| Right-click | On a grammar range (and not on a red word): the message as a disabled first line, up to five corrections (user event `input.grammar`: undoable, reported through `documentChanged`, caret behind the correction, refused if the text moved), a separator, "Ignore Grammar Issue". Where both kinds apply, the spelling menu opens. |
| Ignore Grammar Issue | Page-local: the range's text goes into `SpellEngine.ignoredGrammar` (per document; cleared when `setDocument` builds a new view; kept across `setSpellCheck`), every grammar mark with that text goes at once, and findings with that text are filtered from later replies. The host is not told. |
| CSS ([src/editor/editor.css](../../src/editor/editor.css)) | `.fw-grammar { text-decoration: underline wavy #1a73e8; text-decoration-skip-ink: none; text-underline-offset: 0.2em; }`. Overlap (CodeMirror nests one mark span in the other): both classes on one element = red; grammar nested in spelling = `text-decoration: none` (the red line already runs under it); spelling nested in a longer grammar range = red keeps its line and the blue one sits lower, a second line beneath. `:has()` is not used. |
| Mock bridge ([src/editor/src/bridge.ts](../../src/editor/src/bridge.ts)) | Answers with categories; a doubled word ("is is") is a Grammar finding with message "Repeated word". |

### Verification

- `npm test` 290 / 0 (276 before; 14 new in `test/grammar.test.mjs`; one existing assertion in `test/spelling.test.mjs` now expects `category: "Spelling"` from the mock). `npm run typecheck` clean. `dist/editor.js` rebuilt and committed.
- New node tests: both categories in one reply; richer-reply parsing (old shape, unknown category dropped, malformed fields, five-correction cap); grammar marks keep message and corrections through edits and a mid-range edit drops them; both kinds replaced per window; a late reply mapped through edits made since the request; the ignore set (marks dropped, later replies filtered, by text across the document, survives `setSpellCheck(false)`/`true`, cleared for a new document); Learn/Ignore Spelling leave grammar marks alone; the caret rule on grammar; overlap lookups; `input.grammar`; the menu models; the mock bridge; the real cascade (filtered and exact Live): blue alone, red on one element carrying both classes in either order, `none` for grammar inside spelling, offset for grammar around spelling.
- Qt test `liveGrammarUnderlinesFollowTheGrammarSetting` ([tests/cycle144-live-grammar.inc](../../tests/cycle144-live-grammar.inc)): Live window with "This is is a test. We recieve the mail."; an `.fw-grammar` element holding "is is" and an `.fw-misspelled` one holding "recieve" appear; the grammar mark's computed `text-decoration-color` is `rgb(26, 115, 232)`; `setGrammarEnabled(false)` removes the blue mark, the red one stays, and a 800 ms wait does not bring grammar back; `true` returns it.
- Red then green. Host stubbed to drop Grammar entries (`continue` in the handler): `FAIL! : FomawriteTest::liveGrammarUnderlinesFollowTheGrammarSetting() '!cycle144Texts(view, ".fw-grammar").isEmpty()' returned FALSE. Loc: tests/cycle144-live-grammar.inc(40)`. Restored: `PASS : ...liveGrammarUnderlinesFollowTheGrammarSetting()`. Node: `test/grammar.test.mjs` against the Cycle 143 `spelling.ts` fails to load (`The requested module '../src/spelling.ts' does not provide an export named 'grammarAt'`); against the new one 14 / 14.
- Full Qt suite offscreen (`./bin/test`): 233 passed, 0 failed, 258 s (232 before this cycle's one new test).

### Limits

- The menu DOM and the plugin timers are covered by the Qt test and by hand in a browser, not by node tests; the right-click on a grammar range was not exercised in the real page by a test.
- The overlap look (two lines when a short misspelling sits inside a longer grammar range) is reasoned from the CSS cascade and not measured in rendered pixels.
- "Ignore Grammar Issue" is by text and per document: the same wording elsewhere in the document is ignored too, and it comes back after the document is reopened. It does not feed `SpellCheck`.
- `liveSpellingUnderlinesMisspelledWordsAndFollowsTheSetting` fails when run on its own (an existing order dependency: it needs `spellCheckServiceFindsWordsAndHonoursIgnoreAndSetting` to have ignored "tomorow" first; the Cycle 143 record describes it). In the full run and when run after that test it passes. Not changed here.
- Not hand-tested in the installed app (no install was made from this worktree).

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
