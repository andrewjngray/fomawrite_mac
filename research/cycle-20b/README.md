# Cycle 20b — Command palette and sentence focus

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

Search the shared workspace registry; Unicode sentence focus within the active paragraph.

## Tests and native verification

Command-Shift-P, query → Return, and visible sentence dimming passed natively. Boundary tests pass.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

Palette covers the existing workspace registry, not every new file/edit command. Unicode sentence boundaries are not an abbreviation/language grammar model; broader language/performance checks remain.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Open Command-Shift-P, type sentence, press Return, and move through sample sentences.
