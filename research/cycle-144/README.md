# Cycle 144

Other sections of this record are written by the agents that built the other pieces.

## Live grammar

Grammar findings get a blue wavy underline in the Live page as the writer types, next to the red spelling ones. A right-click on a grammar range shows the checker's message, its corrections and "Ignore Grammar Issue". Spelling behaviour is unchanged.

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
