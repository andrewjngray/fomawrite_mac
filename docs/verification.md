# macOS development verification

Environment: Apple Silicon, macOS 26.6.2, Qt 6.11.2, Apple Command Line Tools.

## Automated checks

`./bin/build` produces an arm64 application bundle. The Qt test suite reports **20 passed, 0 failed**, including a new regression for UTF-8 Markdown round trips and preservation of unsaved content when opening another document.

The offscreen native-dialog fallback emits warnings inside Qt's Material SplitView during dialog teardown. Tests pass; the real macOS Save As dialog was separately checked.

## Interactive checks

- Launched the app and inspected the welcome document, typography and native menu bar.
- Command-Shift-S opened the macOS Save As panel.
- Saved a disposable Markdown copy; the file matched the original byte-for-byte.
- Command-W on a modified document displayed Save/Discard/Cancel.
- Cancel left the unsaved text intact.
- A Finder-style file-open event displayed the unsaved-change prompt.
- Discard then opened the requested welcome document.

## Standalone bundle

`./bin/package-mac` completed successfully. `codesign --verify --deep --strict` verified the local signature. An audit of 194 Mach-O files found no broken bundle symlinks or external non-system library dependencies (excluding each library's own install identifier). The packaged app launched successfully and displayed the welcome document with native menus. Homebrew QML symlinks are dereferenced during packaging.

## Limits

Only this Mac has been tested. Linux changes were reviewed but not built here. Printing, live system appearance changes, crash recovery, multiple simultaneous Finder selections and a full accessibility audit still need manual validation. Recovery, printing and external-change handling largely retain upstream behaviour.

The app remains an initial development milestone, with one process per window. Public distribution needs signing/notarisation and broader compatibility testing.

## Writing workspace — 9 September 2026

The current suite adds folder expansion and safe file creation, source preservation during preview updates, presentation changes without undo mutations, local image resources, relative file-link resolution, typewriter centering, and outline parsing around fences/front matter. Native screenshots checked the three-pane layout, focused paragraphs, source syntax, preview paragraph spacing, task checkboxes and tables. The standalone bundle is rebuilt after these milestones.

Still to check manually: clicking relative links and image display in a real document, large-library performance, narrow window layouts and long-document interactions. Outline is a practical ATX/setext parser, not a complete Markdown parser. Focus follows physical text paragraphs; wrapped lines within one paragraph stay active together.

Cycle 5: compact UI and fenced-code/heading consistency verified with 20 passing tests, packaged signature verification, native live-heading updates and layout switching. Screenshots and logs: research/cycle-05/.

## Cycle 6

Build and package passed; 20 regression tests passed. Deep/strict package signature verification passed. Native sample checks verified navy/icon rendering and Split/Preview switching; records and screenshots live in `research/cycle-06/`. No sample edits or runtime warnings. Dark appearance and full keyboard-only accessibility remain untested manually.

## Cycle 7

Build, 22 tests, package and strict/deep signature verification passed. Final runtime log empty. Native checks covered title/content alignment, library toggling, Split/Full and Find opening. Two new regressions cover excerpts and saved-document typography changes. See `research/cycle-07/README.md` for screenshots, intermediate failures and known untested workflows.
