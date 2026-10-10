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
