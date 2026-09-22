# Cycle 16a — Duplicate and Rename

20 September 2026. Implements parity-log F08/F09 as local sibling-file operations; F10 Move To remains next.

Duplicate copies the current UTF-8 editor buffer, including unsaved changes, into an exclusively created file. It keeps the original active and leaves its dirty state/undo intact. Rename changes the current saved file in its folder without saving the buffer, retaining undo and updating its URL, watcher, recovery snapshot, favorites and recents. Names must be supported Markdown/text filenames without separators or surrounding spaces. Existing destinations are never deliberately overwritten. Symlink sources and unavailable files are refused. Rename refuses known external disk changes.

## Verification

- `./bin/build`: passed, logs/build.log.
- `./bin/test`: 32 passed, zero failed; logs/test.log. Normal macOS access used for filesystem watching. Covers Unicode, dirty duplicate, collisions/path escape, dirty rename, favorites/recents, new recovery URL/text, undo/redo, save and watcher notifications at the renamed path. Existing Qt Material teardown warnings remain.
- Clean old Dev app closed normally; `./bin/prepare-dev-app` passed. Stable bundle identity retained. logs/prepare-dev-app.log.
- `./bin/package-mac` and `codesign --verify --deep --strict dist/Omawrite.app`: passed, logs/package-mac.log and logs/package-signature.log.
- Native sample-only checks: duplicate collision refused; Cycle16-QA.md created from Cycle15-QA.md while original stayed active; copy opened through File Recents; unsaved sample insertion retained through Rename collision, Cancel and successful rename; Command-Z/Shift-Command-Z retained undo/redo; Command-S saved the new path. Original sample unchanged. Final disposable fixture: examples/Cycle16-QA-renamed.md.
- [Collision dialog screenshot](screenshots/rename-collision.png) inspected: readable inline error, filename and actions; library hidden to exclude private names. No private document screenshot saved.

## Limits and next step

No injected disk-full/permission failures or recovery relaunch test. Concurrent writers during the read/rename interval, cross-window path coordination, dark/narrow dialog appearance and case-only rename support remain open. Case-only names on case-insensitive volumes currently report collision. Untitled documents and symlinks require saving to a regular file first. Recovery uses the existing snapshot mechanism and inherits its write-failure limitations. Earlier native checks remain in prior cycle records. No full iA parity claim.

Runnable artifacts: dist/Omawrite Dev.app and dist/Omawrite.app. Dev is left on the clean renamed sample. Changes are uncommitted alongside prior cycle changes. Next build: 16b Move To. [Optional exercise](../usability/cycle-16a.md).
