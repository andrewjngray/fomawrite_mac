# Cycle 143 — spelling as you type

Two surfaces share one checker (`SpellCheck`, `backend.spellCheck`, see [src/spellcheck.h](../../src/spellcheck.h)). This file has one section per surface; the Live half is written by its own agent.

## Source

Misspelled words get a red underline in the Source editor as the writer types, and a right-click on one offers corrections. Spelling only: grammar stays in the Spelling and Grammar dialog.

| Finding | Behaviour |
| --- | --- |
| Nothing marked misspellings in Source | `MarkdownHighlighter` has a spelling layer ([src/markdownhighlighter.cpp](../../src/markdownhighlighter.cpp)). `Backend::attachDocument` hands it `m_spellCheck`; it follows the service's setting, language and learned/ignored words (`enabledChanged`, `languageChanged`, `wordsChanged` each restyle the whole document). Disabled means no underline formats at all. |
| What counts as prose | Per block, not literal: not fenced code (the highlighter's own block state), not raw code style, not front matter (`---` … `---`/`...` at the top), not an indented code run after a blank line, not inline code spans, link destinations `](…)`, raw URLs, HTML tags, inline `$math$`, and not snake_case / path / e-mail tokens. Non-prose characters are blanked to spaces before checking so UTF-16 offsets stay aligned (the way `Backend::proseForReview` does for the whole text). Existing syntax colours survive: the mark is composed onto `format(i)`. |
| Speed | Results are cached by the block's prose text, so one typed character re-checks one block, undoing it re-checks nothing, and a theme change re-uses every answer. Cold, the real macOS checker restyles 2,000 lines in about 0.4 s. |
| Right-click | A `MouseArea` over the editor takes only the right button. On an underlined word it opens `spellingContextMenu` at the pointer: up to five suggestions (`spellingSuggestion0..4`, or a disabled "No Guesses Found"), a separator, "Learn Spelling" (`spellingLearn`) and "Ignore Spelling" (`spellingIgnore`). A suggestion goes through `backend.correctWriting`, one undoable edit. Anywhere else the press is declined, so default behaviour is untouched. |
| Menu and underline agree | `Backend::misspelledWordAt(position)` asks the highlighter for the same ranges it draws, so a word in code, a URL or front matter is never offered a correction. |

### How the underline is drawn (measured, not assumed)

Frames of the real window were captured and their pixels counted (`sourceSpellingUnderlineIsRedInTheRenderedWindow`, frames in `research/cycle-143/captures/`, local only):

- `QTextCharFormat::SpellCheckUnderline` is **not drawn at all** by Qt Quick's text item (Qt 6.11.2): it tests `fontUnderline`, which is false for that style. The wavy native style is therefore unavailable in Source.
- A plain `SingleUnderline` draws in the **text colour**; the underline colour is ignored.
- The same underline with the run also carrying a **background** does draw in the underline colour. So the mark is a single underline in `#d93025` plus a pale red wash (`#fbd5d1` light, `#5c2b2b` dark). In the captured frame 152 pixels are saturated red under the misspelled word and none under a correct one. It is a straight underline with a wash, not a wave.

### Verification

New tests, [tests/cycle143-source-spelling.inc](../../tests/cycle143-source-spelling.inc): `sourceSpellingUnderlinesProseOnly`, `sourceSpellingFollowsTheSettingAndLearnedWords`, `sourceSpellingChecksOnlyChangedBlocks`, `sourceSpellingUnderlineIsRedInTheRenderedWindow`, `sourceSpellingContextMenuOffersSuggestionsAndCorrects`. Each was run with the feature stubbed (highlighter returning no ranges; then only the formatting skipped and the right-click handler disabled) and failed, then with it restored and passed. Real-checker tests use words no other test touches (`recieve`, `definately`, `occured`) because the system checker's ignore list lasts for the whole test process; they never call Learn Spelling, which would write to the user's dictionary.

### Limits

- Edits to a front-matter delimiter restyle only that block, so words inside the front matter keep their old marks until the next full restyle.
- Indented lines after a blank line are treated as code; a loose-list continuation indented four spaces is therefore not checked.
- The right-click menu does not move the caret or select the word.
- Straight underline with a wash, not a macOS wave (see above). A wave would need the editor to draw it itself.

# Cycle 143 — spelling in both writing surfaces

Both surfaces share one checker, `SpellCheck` ([src/spellcheck.h](../../src/spellcheck.h)): the macOS system spell checker, with a setting, a language, suggestions, and learned/ignored words. Each surface decides which stretches are prose. This record has one section per half; the Source half is written by its own author.

## Live

Misspelled words get a red wavy underline in the Live page as the writer types, and a right-click on one offers suggestions, Learn Spelling and Ignore Spelling.

### What changed

| Piece | Behaviour |
| --- | --- |
| Bridge contract ([src/editor/README.md](../../src/editor/README.md)) | Page to host: `checkSpelling(token, segmentsJson)`, `spellingSuggestions(token, word)`, `learnWord(word)`, `ignoreWord(word)`. Host to page: `spellingReply(token, rangesJson)`, `suggestionsReply(token, wordsJson)`, `setSpellCheck(enabled)`. Tokens are the page's; the page ignores any reply that is not its outstanding request. |
| Host ([src/editorbridge.*](../../src/editorbridge.h), the bridge block of [src/backend.cpp](../../src/backend.cpp)) | Per segment, `SpellCheck::misspellings(text)` with the segment's `from` added; `suggestions(word)`; learn and ignore forwarded. `setSpellCheck` goes out on page ready (replayed like the theme) and whenever `enabledChanged`, `languageChanged` or `wordsChanged` fires, and every one of them makes the page check again. |
| Prose only ([src/editor/src/spelling.ts](../../src/editor/src/spelling.ts)) | The Markdown syntax tree decides: fenced and indented code, inline code, URLs and link destinations, bare and angle-bracket autolinks, raw HTML and comments, front matter, math, reference ids, footnote refs, `[toc]` and the Markdown syntax characters are blanked to spaces, so offsets are unchanged and `_word_` reaches the checker as a word. |
| When | About 300 ms after the last change, 150 ms after leaving the checked window by scrolling; window = viewport plus 2000 characters. An edit that changes a word drops its mark at once; other marks are mapped through the edit, and a late reply is mapped through the edits made since the request. |
| The word being typed | A word the empty caret ends is held back and checked 50 ms after the caret leaves it. Held back whatever follows the caret (a word typed mid-sentence has a space after it too), which is a deliberate widening of "no space after". |
| Right-click | On an underlined word: menu of up to five suggestions, separator, Learn Spelling, Ignore Spelling; Escape, outside click, Tab, scroll or a choice closes it. A suggestion is a user-event change (`input.spelling`): reported to the host as an ordinary change and undoable. Elsewhere the default is untouched. |
| Setting | `setSpellCheck(false)` clears every mark and stops requests; `true` re-checks and replaces all marks on the answer. |

### Verification

- `npm test` 276 / 0 (256 before; 20 new in `test/spelling.test.mjs`: segment extraction skips code, URLs, HTML, front matter, math; marks map through edits; a reply mapped through edits made in flight; stale, unknown, replayed and reset tokens; the caret-word rule; `setSpellCheck(false)` and back; suggestions with stale and timed-out replies; the suggestion transaction is a user event and changes the document; the mock bridge; the underline survives the Live theme overlay in the real cascade, filtered and exact). `npm run typecheck` clean. `dist/editor.js` rebuilt and committed.
- Qt test `liveSpellingUnderlinesMisspelledWordsAndFollowsTheSetting` ([tests/cycle143-live-spelling.inc](../../tests/cycle143-live-spelling.inc)): Live window, "Their going too the shop tomorow. Also zxqvplumb.", an `.fw-misspelled` element holding "zxqvplumb" appears; the first request carries the prose from offset 0; `setEnabled(false)` removes it and the page stops asking; `true` brings it back; `ignoreWord` removes it. The nonsense word is deliberate: the system keeps ignored words per app, and `spellCheckServiceFindsWordsAndHonoursIgnoreAndSetting` ignores "tomorow" for the rest of the process, so in a full run "tomorow" is no longer flagged (the first full-suite run failed exactly there; alone, "tomorow" passes).
- Full Qt suite offscreen (`./bin/test`): 225 passed, 0 failed (this half alone; the Source half adds its own tests).
- Red then green. Extension not mounted (`spellingExtension(bridge)` commented out in `main.ts`, bundle rebuilt): `FAIL! : FomawriteTest::liveSpellingUnderlinesMisspelledWordsAndFollowsTheSetting() 'cycle143Underlined(view).contains("tomorow")' returned FALSE.` Mounted: `PASS : ...liveSpellingUnderlinesMisspelledWordsAndFollowsTheSetting()`. With `src/spelling.ts` absent, `test/spelling.test.mjs` fails to load (`ERR_MODULE_NOT_FOUND`); present, 20 / 20.
- Real browser (dev server, mock bridge, Chromium): right-click on "tomorow" shows the menu, choosing "tomorrow" replaces the word and the mark goes; Escape and an outside click close it; typing "wrold" at the end of the document is not underlined until the caret moves, then it is.

### Limits

- The menu DOM and the plugin timers are exercised by the Qt test and by hand in a browser, not by node tests.
- Only the first five suggestions are shown (the host sends up to eight).
- The page's own `spellcheck="true"` content attribute is left as it was; Qt WebEngine has no dictionaries configured here, so it draws nothing of its own.
- A language change is picked up through `languageChanged`; there is no language menu in the Live page.
- Not hand-tested in the installed app (no install was made from this worktree).
