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
