# Cycle 25b — Native tabs

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

AppKit merge, next/previous, detach, overview and tab-bar controls. Reserve native tab-bar height below custom toolbar.

## Tests and native verification

Live merge produced two native tabs; switching retained text; detach preserved unsaved text. Screenshot found a covered first line; corrected inset and reviewed final screenshot.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

Restart restoration/tab-group persistence and exhaustive dirty-close/overview/fullscreen/keyboard states remain. Some tab commands remain enabled when not applicable.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Merge two sample windows, switch tabs, detach one and verify its draft.
