# Initial macOS port verification

Environment: Apple Silicon, macOS 26.6.2, Qt 6.11.2, Apple Command Line Tools.

## Automated checks

`./bin/build` produces an arm64 application bundle. The Qt test suite reports **13 passed, 0 failed**, including a new regression for UTF-8 Markdown round trips and preservation of unsaved content when opening another document.

The offscreen native-dialog fallback emits warnings inside Qt's Material SplitView during dialog teardown. Tests pass; the real macOS Save As dialog was separately checked.

## Interactive checks

- Launched the app and inspected the welcome document, typography and native menu bar.
- Command-Shift-S opened the macOS Save As panel.
- Saved a disposable Markdown copy; the file matched the original byte-for-byte.
- Command-W on a modified document displayed Save/Discard/Cancel.
- Cancel left the unsaved text intact.
- A Finder-style file-open event displayed the unsaved-change prompt.
- Discard then opened the requested welcome document.

## Limits

Only this Mac has been tested. Linux changes were reviewed but not built here. Printing, live system appearance changes, crash recovery, multiple simultaneous Finder selections and a full accessibility audit still need manual validation. Recovery, printing and external-change handling largely retain upstream behaviour.

The app remains an initial development milestone, with one process per window. Public distribution needs signing/notarisation and broader compatibility testing.
