# Cycle 22a — Preview navigation

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

Unique heading slug targets, inserted Markdown TOC, and opt-in proportional bidirectional scrolling with a feedback guard.

## Tests and native verification

Duplicate-heading TOC and actual rendered anchor lookup pass. QML loading/regression tests pass.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

Scroll synchronization is proportional, not semantic paragraph/image alignment. Full long-document native anchor/scroll and cross-file fragment checks remain.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Insert a TOC into a disposable document and follow duplicate-heading links in preview.
