# Cycle 29 — Authorship file-operation closeout

Implemented: saved metadata follows Rename/Move; Duplicate includes current unsaved annotations. Metadata destinations are never overwritten by these operations. Failure before the Markdown operation cleans up staged metadata; partial Move retains the metadata alongside the retained copy. Failure to remove old metadata reports cleanup needed.

## Verification

- Build passed: `logs/build-final.log`.
- Tests: 51 passed, 0 failed: `logs/test-final.log`. The first new dialog test checked visibility during its exit animation; corrected to wait for completion, then the whole suite passed.
- Native Duplicate generated `sample/Safety copy.md` and matching metadata.
- Native Rename changed `Safety-renamed.md` to `Safety-final.md`, preserving the active document and text.
- Native Move selected `sample/moved` in the folder picker and reported “Moved to moved”; the document remained active.
- Resulting bytes, SHA-256 and Human / QA author range 0–5 verified for both outputs: `logs/native-files.txt`.
- The old apparent empty-window switch did not represent a lost document: opening its renamed path focused the original still-running window, including its original “Renamed to” status. Repeat Rename passed. The initiating cause of the earlier focus change is unknown, so no focus fix is claimed.
- Screenshot capture returned “Screenshot unavailable”; no private writing or screenshot was saved. Native observations above are a written verification record.
- Dev quit normally after verification; process inspection confirmed no Omawrite processes before refreshing bundles. No dirty user document was discarded.

## Limits

Not crash-atomic across the Markdown/metadata pair. No cross-volume, disk-full, disconnection or concurrent external-writer stress verification. Recovery snapshot annotations are tested; exhaustive crash/relaunch recovery is still open. Full feature parity and release approval remain open.

Runnable artifacts: `dist/Omawrite Dev.app` and `dist/Omawrite.app` (local ad-hoc signing). Packaging logs are in `logs/`.
