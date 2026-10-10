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
