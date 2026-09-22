# Cycle 22b — Markdown extension subset

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

Preview-only ==highlight==, explicit relative [[target|label]] links, single-line footnotes and bounded local text/Markdown content blocks. Literal code preserved; include depth 5, 256 KiB per include, 1 MiB total, cycles/escaping outside the current include directory refused.

## Tests and native verification

Automated literal-code/include-cycle/traversal tests pass. Native sample renders highlights, wikilink, footnote appendix, table and literal fenced code. Screenshot reviewed.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

Wikilinks do not search nearest matches across the library. Multiline/backlinked footnotes, CSV/image/code content blocks, full title syntax and rebasing ordinary relative links inside nested includes remain. Missing includes show a message.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Open examples/Cycle28-QA.md and compare source to preview.
