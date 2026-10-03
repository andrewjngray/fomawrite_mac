# Cycle 104 — clearer folder and writing controls

Date: 4 October 2026. Review target: `0.3.0-dev5`, macOS `0.3.0 (104)`.

Andrew’s toolbar review requested a recognizable folder heading, stronger formatting symbols and direct text-size controls beside Aa. The approved workspace composition from Cycle 103 remains in place; this cycle does not reset saved layout or appearance choices.

## Changes

- The central column’s heading is a tonal button with a blue folder glyph, the current folder name and an explicit chooser tooltip. Clicking retains the existing native folder picker. There is no misleading dropdown arrow for a system-panel action.
- The formatting capsule uses a visibly bold **B**, italic serif **I**, the existing link glyph and typographic **¶**. Its existing formatting actions remain unchanged.
- A compact **− / +** capsule sits between formatting and **Aa**. It uses the existing persisted text-size commands, changes Source, Preview and Visual Edit, and disables each end at the supported size limits. It does not change Markdown, Undo history or export point size.
- Preview and Visual Edit previously applied minimum font sizes after the zoom offset, making some initial minus clicks ineffective. Applying the zoom after their normal baseline keeps the initial appearance and lets each step visibly change the text.
- In narrow document headers, formatting condenses and the history capsule hides when necessary, retaining zoom, Aa and pane restoration without overlap. Document history remains available from the native Go menu and its existing shortcuts.

## Native visual evidence

These are captures of the running native Qt workspace using synthetic text:

- [Wide controls](screenshots/cycle104-controls-wide.png)
- [Narrow controls](screenshots/cycle104-controls-narrow.png)
- [Collapsed navigation](screenshots/cycle104-controls-collapsed.png)

## Verification

- `./bin/build`, invoked by the final package command, completed. [Build/package log](package.log).
- Full `./bin/test`: **144 passed, 0 failed, 0 skipped**. [Log](tests.log).
- Three focused control tests passed both offscreen and with native Cocoa: **5/0/0** including setup/cleanup. [Offscreen log](targeted-controls.log), [native log](native-controls.log).
- Actual pointer clicks change rendered body-line height in all three modes, including the initial minus click; preserve saved and unsaved Unicode Markdown, canonical Undo/Redo, output style and point size; and respect both size bounds.
- Resize checks cover 1440, 1100, 980, 900 and 720px widths in Source, Split and Preview, plus collapsed navigation. Controls are checked after layout settles and clicked in each configuration. The logs can show transient overlap diagnostics while Qt is still settling; final geometry and clicks pass.
- The real folder button emits the existing `chooseFolder` action with the correct anchor. The fixture intentionally does not open a blocking native picker. An initial test comparison of `/var` and `/private/var` was corrected to compare canonical paths; the production folder path behavior was unchanged.
- Full-suite logs retain existing delayed Preview/Material SplitView teardown warnings. No assertions failed. Physical VoiceOver/display checks and Andrew’s visual review remain open; broader native window routing was verified in Cycle 103 and was not rerun for this toolbar change.

## Review builds

All three copies have been refreshed and pass strict/deep local signature verification:

- `dist/Fomawrite Dev.app` — stable Dev identity, local Qt linkage.
- `dist/Fomawrite.app` — ordinary/demo package with bundled Qt.
- `/Applications/Fomawrite.app` — installed ordinary package; executable matches the demo.

See [artifacts.json](artifacts.json) for exact hashes and source revision, [Dev preparation](dev-install.log) and [Applications installation](install.log). The previous Applications copy is retained at the backup path in the install log. Local signing is not public notarization; the public RC1 release is unchanged.

[Optional review exercise](../usability/cycle-104.md). Next: Andrew’s actual writing review, followed by the highest-value remaining image/table/list interaction with exact-source safeguards. Optional distribution Cycle 101 remains separate.
