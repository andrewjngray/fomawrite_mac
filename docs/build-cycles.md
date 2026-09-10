# Build cycles and usability log

This is the standing record for Andrew and Codex. Each cycle leaves a working build, records verification and known gaps, and gives Andrew a small optional usability exercise. Feedback steers the next cycle; routine implementation does not require a check-in.

## Cycle history

| Cycle | Scope | Result | Evidence |
|---|---|---|---|
| 0 | macOS foundation | Complete | Native menus/dialogs, UTF-8 saving; 13 tests |
| 1 | Explorer/editor/live preview | Complete, 1d13d12 | 15 tests; native three-pane check |
| 2 | Typography, source markup, paragraph/typewriter focus | Complete, 8ab9b69 | 17 tests; native focus/table/task checks |
| 3 | Outline and statistics | Complete, 21b865e | 18 tests; packaged app/signature; native heading jump |
| 4 | iA menu audit; organizer, favorites, recents, sorting | Completed with Cycle 5 | Persistence/sorting tests and native organizer inspection |
| 5 | UI alignment and Markdown preview consistency | Complete | 20 tests; comparison screenshots |

Earlier execution detail, including the overnight computer-use stall, is in [overnight-progress.md](overnight-progress.md). Detailed reference capabilities are in [ia-writer-inventory.md](ia-writer-inventory.md).

## Cycle 4 — organizer and file navigation

**Intent:** match the useful structure of iA's organizer + file list + editor + preview.

**Implemented:** persisted local locations; file/folder favorites; 20 deduplicated recent files; removable shortcuts; name/modified/created/extension sorting; ascending/descending; folder pinning; independent organizer visibility. Organizer collapses below 1,000 px. Document switching still uses the unsaved-change prompt. Removing a shortcut does not delete files.

**Verification:** 19-test cycle-4 suite passed; organizer and recent entries inspected in the running Cycle 5 app. Final visual polish and packaging included in Cycle 5.

**Known gaps:** filter searches expanded folders only; no smart folders, hashtags, excerpts, list navigation or drag/drop organization. Locations are plain local paths, not special iCloud APIs. Settings are stored per app/user; multiple already-open processes do not live-sync their organizer state.

**Andrew's optional five-minute test:**

1. Open the packaged app and choose a small writing folder with a subfolder.
2. Add a second location. Switch between them and try sorting by name/date.
3. Favorite a folder and a saved document; open them from Favorites.
4. Edit a disposable note, then select a recent file. Check Save/Discard/Cancel feels clear.
5. Hide Organizer, switch Editor/Split/Preview, and resize the window.

**Feedback:** What felt awkward? Which navigation action did you expect but could not find? Were the pane widths, labels and click targets comfortable? Record examples below or tell Codex in this task.

- Keep:
- Change:
- Missing:
- Bug/reproduction:

## Upcoming cycles

| Cycle | Planned features | Acceptance focus |
|---|---|---|
| Next navigation cycle | Quick open, back/forward, tree/list, date/excerpt options; bounded recursive search | Deferred in favor of Andrew’s requested UI tuning |
| 6 | Hashtags and smart folders | Accurate indexing; ordinary Markdown stays portable |
| 7 | Preview sync, anchors, output templates, HTML/PDF export | Source/output consistency; local images; pagination |
| 8 | Expanded Format menu and editing toolbar | One-step undo; selection/cursor stability; Markdown round trip |
| 9 | Sentence focus, spelling and writing aids | Useful behavior without false linguistic/provenance claims |
| 10 | Native multi-document lifecycle, autosave/version strategy, accessibility | Recovery and data safety; keyboard-only use |
| Later | Authorship, publishing, custom commands/integrations | Define actual personal workflow before implementation |

## Template for each next cycle

- Intent and planned features:
- Implemented / deferred:
- Build and automated checks:
- Native comparison and usability checks:
- Known gaps and risks:
- Commit and runnable artifact:
- Andrew's short test:
- Feedback and next-cycle changes:

## Cycle 5 — UI alignment and preview consistency

**Andrew's direction:** prioritize the UI before more features; verify the preview around `#title`; keep screenshots and usability records in the project.

**Implemented:** compact shared chrome controls, segmented Editor/Split/Preview choices, narrower organizer/file panes, subtle separators, bottom file filter, smaller file rows, quiet section labels, bottom editor actions and visible status feedback. macOS dialogs inherit the system-style interface face. Preview typography uses a smaller body size and controlled heading scale. Source highlighting now keeps fenced code literal and recognizes up to three leading spaces before headings.

**Preview result:** iA confirms `# title` is a heading and `#title` is a hashtag rather than a heading. Omawrite follows the same heading rule; hashtag linking remains unimplemented. No automatic source rewriting was introduced.

**Checks:** 20 tests pass, including real QML preview document heading levels, source preservation and literal fenced code. Visual review caught and corrected clipped control labels and a preview font-base mismatch. Known Qt Material teardown warnings remain in the native-dialog test; no new runtime failure was observed.

**Usability exercise and screenshots:** [research/cycle-05](../research/cycle-05/README.md), [Andrew's checklist](../research/usability/cycle-05.md).

**Deferred:** exact iA template rendering, synced scroll, excerpt rows, full icon/accessibility polish, sentence focus and broader Markdown extensions. This is closer visual alignment, not a parity claim. Preview/Editor still scroll independently.

**Cycle 5 final result:** build/package and deep/strict signature verification passed. Native checks confirmed organizer visibility, heading rendering, live heading updates, and Preview/Split switching. The sample was restored and saved; existing unsaved user windows were not touched. Four final/reference screenshots plus an intermediate screenshot and test/package logs are retained under `research/cycle-05/`. Runnable artifact: `dist/Omawrite.app`; review fixture: `examples/Preview-check.md`. For a fresh process when an older Omawrite is already open, run `./dist/Omawrite.app/Contents/MacOS/Omawrite examples/Preview-check.md` from this project folder.
