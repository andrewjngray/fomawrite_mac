# Cycle 24 — Mac presentation subset

Date: 21 September 2026. Status: implemented subset; acceptance gaps remain below.

## Planned scope and changes

Native Markdown share picker plus Minimize, Zoom and Bring All to Front; platform-specific Objective-C++ remains separate.

## Tests and native verification

Native share picker opened on disposable prose and Escape dismissed it. No transmission performed. Build/QML tests pass.

The final combined suite passed all 48 tests. See [final logs](../cycle-28/logs/). Stage-specific logs, where present, are in `logs/`; shared screenshot evidence is in [Cycle 28 screenshots](../cycle-28/screenshots/). Native checks ran on shared combined builds. No claim that every menu label has full iA semantics.

## Known gaps

Share-sheet open/cancel passed; remaining window-presentation checks remain. Title/toolbar fade/statistics-only modes and external-display behavior are not implemented in this pass.

## Runnable artifacts and optional exercise

Use `dist/Omawrite Dev.app` for QA or `dist/Omawrite.app` for the packaged build. Both come from the final source. No commit was made because the working tree includes substantial earlier/user changes.

Open Share Markdown on disposable text and cancel without choosing a destination.
