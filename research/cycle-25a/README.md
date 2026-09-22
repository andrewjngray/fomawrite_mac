# Cycle 25a — Coordinated document windows

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

One Backend/QML engine per window in a shared process; same-file opens focus existing windows. Quit closes sequentially through normal save prompts; cancellation/save failure stops the sequence. Recovery slots remain isolated.

## Tests and native verification

Native two dirty windows: New preserves first, Quit/Cancel preserves both, then Discard advances to second prompt. Tests cover new-window request and save-failure cancellation.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

Separate application launches are not consolidated. Saved-window restoration across restart and exhaustive crash/OS-quit/minimized-window matrix remain. Clean windows already closed before a later Cancel are not reopened automatically.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Create two disposable dirty windows, Quit, then Cancel before discarding either.
