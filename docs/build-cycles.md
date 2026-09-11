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
| Later navigation cycle | Hashtags and smart folders | Accurate indexing; ordinary Markdown stays portable |
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


## Cycle 6 — navy palette and line icons

**Planned scope / feedback:** Andrew found teal unpleasant and preferred iA Writer's icon treatment. Prioritize color and control clarity before expanding features.

**Implemented:** navy active controls, pale blue selected rows, consistent blue focus/selection accents; original line icons replace symbol-font glyphs in the toolbar and library. Editor/Split/Preview retain labels and accessible names. macOS document accent defaults changed separately from Linux.

**Checks:** build succeeds; all 20 existing regression tests pass. No document-format or I/O behavior was changed. Native verification and screenshots are recorded in [cycle 6 research](../research/cycle-06/README.md).

**Known gaps:** full dark-appearance and keyboard accessibility audit remains outstanding; synced preview and advanced navigation remain deferred. This pass does not claim iA feature parity.

**Runnable artifact:** `dist/Omawrite.app`. [Optional usability exercise](../research/usability/cycle-06.md): try layout icons, select files/text, and judge blue contrast and icon clarity.

**Native result:** sample opened, navy/icon rendering reviewed, Preview/Split switching verified, screenshots saved. Package and deep/strict signature check passed. Runtime log empty; sample unchanged. Dark appearance not manually verified.

## Cycle 7 — UI matching

Andrew said the open apps still looked very different and requested direct comparison and iteration. Planned scope: pane proportions, smaller type, shared title/toolbar space, quieter navigation rows and footer controls. Two layout revisions are in the working tree; the first passed 20 tests after corrections, but native review exposed title/content overlap. A second correction builds and has an empty runtime log; visual and regression checks remain pending. Cycle 6 remains the packaged/committed checkpoint. See [resume notes and current evidence](../research/cycle-07/README.md). Paused at Andrew's request before he sleeps the computer; no background work scheduled. Optional usability exercise after verification: compare both apps on Preview-check.md, assess text scale, pane balance and toolbar placement.

**Cycle 7 final result (11 September):** resumed and completed with corrected macOS title/toolbar placement, smaller typography, balanced panes, file dates/snippets, footer controls and persistent status. Build, 22 tests, package and deep/strict signature check passed. Native review verified key layout controls and final screenshot; no sample changes. Runnable artifact: `dist/Omawrite.app`. [Evidence and remaining gaps](../research/cycle-07/README.md), [optional comparison exercise](../research/usability/cycle-07.md). Preview template details and full native-window/dark/narrow-width audits remain outstanding.

## Workflow follow-up — approval visibility and reusable QA app

Andrew identified repeated app-access approvals as the cause of long attachment waits. Changed macOS ChatGPT notification style to Persistent (notifications and sounds were already enabled). Added `bin/prepare-dev-app` with one stable QA path/ID and a running-process guard; no global security policies changed. Preparation, shell syntax and strict signature verification passed. Closed two inspected clean QA windows; preserved recovered unsaved content. In-app permission/question alerts and the one-app Always allow selection require Andrew's own interaction; Computer Use cannot operate Codex itself. [Details and procedure](development-app-approvals.md).

## Cycle 8 — rounded Sort by menu

**Planned scope:** Andrew asked for rounded controls and a compact dropdown matching the sorting menu in his iA screenshot.

**Implemented:** rounded Sort by pill, separate compact menu, four sort fields, exclusive A to Z/Z to A choices, folder pinning, persistent date/excerpt visibility toggles, checkmarks and standard interface typography. The folder-actions menu shares the style. Navigation and date-format submenus remain deferred.

**Verification:** build and 23 tests passed, including menu action coverage. Native menu inspected and captured in the stable development app; saved sample closed normally before refreshing it. Dark appearance/full keyboard audit untested. [Records](../research/cycle-08/README.md), [optional usability exercise](../research/usability/cycle-08.md). Runnable artifacts: `dist/Omawrite Dev.app` and packaged `dist/Omawrite.app`. Final packaging and strict signature verification passed.

## Cycle 9 — quieter file browsing

**Planned scope / feedback:** Andrew requested delayed filename tooltips and compact file rows, with a top-level excerpt toggle.

**Implemented:** 700ms hover delay for file rows and shared chrome controls (including organizer paths); a rounded Previews toggle beside Sort by, synchronized with Show Text Excerpts. Compact filename-and-icon rows retain extensions. Dates and excerpts start hidden through a one-time preference migration; subsequent choices persist. Hidden excerpts are not fetched. Document preview is unaffected.

**Checks:** build succeeded; all 23 tests passed with normal macOS access, including button/menu synchronization. The initial restricted run failed the existing file-watcher check; the native rerun passed. Native QA verified the migrated compact default and toggling excerpts on/off with sample files. Screenshots saved in [cycle 9 records](../research/cycle-09/README.md). Exact hover timing and dark appearance remain manual checks.

**Runnable artifacts:** `dist/Omawrite Dev.app` (stable QA identity) and `dist/Omawrite.app`. See [optional usability exercise](../research/usability/cycle-09.md). No source documents changed.

Final packaging and deep/strict signature verification passed.
