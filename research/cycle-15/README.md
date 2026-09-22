# Cycle 15 — everyday file access

Implemented 17 September 2026; packaged and documented 18 September after a stalled Finder inspection. Changes remain uncommitted.

## Scope and behavior

- File → New creates an untitled document in the current window, with Save/Discard/Cancel when necessary. Existing Command-N retains New Window; no shortcut reassignment.
- New in Library, New in Library in Window and New Folder expose library creation. Current-window creation waits for the unsaved-change decision before writing an empty file. New-window creation leaves the existing buffer untouched; launch failure preserves the new file and reports the failure.
- File → Open Recent and Go → Recent Files use the same persisted, deduplicated list as the organizer. Paths distinguish duplicate names; Clear Recent Files only removes shortcuts. Missing files report an open failure without replacing the current buffer.
- Go → Locations exposes saved local folders and Add Location.
- Show in Library reveals the current saved file, clears the filter, expands ancestor folders, scrolls to its row and switches to its parent folder if outside the current root. Missing files and scan-limit failures report an error.
- Show in Finder validates a saved local file, then invokes macOS `open -R` with arguments (no shell). Non-Mac fallback reveals the containing folder.
- Removed the duplicate authored Full Screen menu item discovered in native Cycle 14 review; AppKit supplies its native item.
- Failed open/create after Discard no longer deletes recovery for a buffer that was not actually replaced.

## Verification

- Build passed: `logs/build.log`.
- All 31 automated tests passed: `logs/test.log`. New coverage includes nested/outside-root/filtered reveal, missing targets, cancelled creation with no orphan file, Save then create, collision preservation, New resetting path/undo, and dynamic recent-menu opening and missing-file preservation.
- Existing Qt Material dialog-teardown warnings remain; no new QML errors were reported by the final test run.
- Stable Dev bundle refreshed and signature verified: `logs/prepare-dev-app.log`.
- Packaging and deep/strict signature verification: `logs/package.log`, `logs/signature.log`.
- Existing unrelated working-tree changes and private writing were preserved.

## Native results

Verified with computer use in Omawrite Dev:

1. Closed the old clean QA window normally and refreshed the stable bundle. Earlier Cycle 13/14 replacement blocker is resolved.
2. Opened the Cycle 14 View menu and confirmed workspace controls; detected the duplicate Full Screen entry and removed our duplicate in Cycle 15. This is not a full dark/narrow/accessibility audit.
3. Inspected the new File menu and untitled-document disabled reveal actions.
4. Opened `Workspace-tour.md` through File → Open Recent.
5. Selected the examples location through Go → Locations.
6. Created `examples/Cycle15-QA.md` through File → New in Library.
7. Typed disposable sample text, invoked File → New, verified the unsaved prompt, chose Cancel, confirmed text remained, then saved the sample.
8. Filtered the sample out of the library; Show in Library cleared the filter and restored its row.
9. Verified the sample bytes on disk after native Save.

Only sample content was edited. Organizer/library were hidden before attempting a screenshot. The screenshot API returned “Screenshot unavailable”; no screenshot is claimed. Raw accessibility output containing private recents or other application content was not saved in the repository.

Show in Finder was invoked, but the subsequent Finder inspection stalled for roughly 11.5 hours and eventually returned the desktop rather than a confirmed selection. It is **not counted as verified**. The Dev process was left untouched after that delay because the user could have resumed work.

## Remaining native checks

- Confirm Finder selection, new-file-in-new-window launch, actual New Folder creation, and Add Location dialog completion.
- Finish save/discard failure-path checks in native UI; automated tests cover the principal buffer/filesystem guards.
- A native Open dialog attempt on the newly written research fixture showed a disabled Open button; it was cancelled. Recent-file opening worked. Reproduce the picker behavior with the file type filter and a current sample before claiming picker verification.
- Complete dark/narrow/keyboard/restart/fullscreen and local-image reload checks carried forward from Cycle 14.
- Obtain sample-only screenshots when capture is available.

## Artifacts and exercise

`dist/Omawrite Dev.app` and `dist/Omawrite.app` contain Cycle 15. Source fixture: `fixtures/Menu-check.md`; native-created disposable note: `examples/Cycle15-QA.md`.

[Optional exercise](../usability/cycle-15.md): open a recent sample, filter it out and reveal it, then create a library note and try Cancel on an unsaved New action. Rename/move/duplicate remain the next separate stage.
