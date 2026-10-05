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

**Superseded on 17 September 2026:** use the [staged development plan](development-plan.md) for proposed Cycles 14–28 and the [screenshot parity log](ia-menu-parity-2026-09-17.md) for acceptance scope. The table below is historical planning, not the current schedule.

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

## Cycle 10 — library options menu

**Scope:** add the actions visible in Andrew's reference screenshot to the library toolbar dropdown.

**Implemented:** New File, New Folder, Sort By and View Options submenus, Hide/Show Sort Bar and Hide/Show Filter Bar. Original line icons, submenu chevrons and compact rounded styling. Bar visibility persists. Sort By shares its implementation with the existing sort dropdown; View Options controls dates and text excerpts. Choose Folder and Refresh remain below a separator. Hiding the filter bar preserves its active filter text.

**Verification:** build and 24 tests passed, including bar-action labels and submenu sorting. Native QA verified opening both submenus and hiding/restoring both bars. Sample document unchanged. Screenshots and logs in research/cycle-10/. New File/New Folder use the existing dialogs and I/O; creating new items via these new menu entries and full keyboard/dark-mode behavior remain untested in native QA. No claim of full iA View Options parity.

**Artifacts:** dist/Omawrite Dev.app and dist/Omawrite.app. Optional exercise: open the dropdown beside +, try the two submenus, hide and restore each bar, and confirm your preferred settings survive restart.

Cycle 10 final check: New File and New Folder dialogs opened and cancelled successfully in native QA. Final packaging and deep/strict signature verification passed.

## Cycle 11 — blue icons and larger Mac interface text

**Feedback / planned scope:** Andrew requested iA-like filled blue folders, blue outline location markers, white document icons, larger header labels, system-style interface typography, and rounder window corners.

**Implemented:** original vector drawings for shaded blue folders and folded white pages in the file list; blue outline folders for organizer locations/favorites; filled folder in the header. Native macOS general system font replaces Helvetica Neue in interface chrome and menus; source writing font remains unchanged. Header folder/document labels grow from 12 to 15px with stronger weight; file labels and standard buttons use 13px. No new settings.

**Verification:** build and 24 regression tests passed after correcting an icon-color fallback found in native QA. Native inspection confirmed location outline colors, filled folders, document icons, restored toolbar icons and larger header labels. Saved screenshots/icons-and-header.png uses the project library and sample document; private recents hidden. Sample contents unchanged.

**Limits:** outer window corners and traffic-light controls remain native and unchanged; no custom frame or claimed exact iA corner matching. Icons are original approximations, not extracted iA assets. Full dark-mode, narrow-window and display-scaling checks remain outstanding.

**Artifacts:** dist/Omawrite Dev.app and dist/Omawrite.app. Optional usability exercise: compare folder/document legibility, header text size and location blue, and try a narrower window. Report whether the white document icons need stronger contrast.

Cycle 11 final packaging and deep/strict signature verification passed.

## Cycle 12 — collapsible organizer sections and native title bar

**Planned scope:** Favorites/Recents disclosure chevrons, aligned favorite actions, and investigation of the larger/lower native traffic lights and rounder frame in Andrew's iA reference.

**Implemented:** clickable section headers with down/right chevrons and persisted expanded states. Favorites collapses its entries and add actions; Recents collapses its entries without clearing them. Favorite folder/document use identical left padding and plus-icon columns. macOS now requests a native unified NSToolbar/title-bar style rather than a plain title bar; native controls retain their system behavior and dimensions.

**Verification:** build and 25 tests passed, including disclosure toggles and shortcut preservation. Native QA verified both collapses and Favorites restoration, aligned actions, and continued toolbar interaction. Screenshots under research/cycle-12/screenshots show the sample document and hide private recents. The screen-sharing badge overlays the traffic lights, so exact dot size/placement and corner-radius parity with iA remain unverified. No custom corner mask or enlarged imitation controls added. Full dark-mode, restart persistence and fullscreen transition checks remain outstanding.

**Artifacts:** dist/Omawrite.app and dist/Omawrite Dev.app. The running Dev bundle has the same functional implementation as the final source (subsequent changes only reindent QML/add tests). The sample acquired unsaved state during QA; no save/discard action was taken and that window was preserved.

**Optional exercise:** toggle both section headers, check aligned Favorite actions, restart after saving your work to check persistence, and compare the traffic lights and corners once app inspection stops.

Native API reference: https://developer.apple.com/documentation/appkit/nswindow/toolbarstyle-swift.enum/unified

Cycle 12 final packaging and deep/strict signature verification passed.

## Cycle 13 — Codex becomes the interface reference

**Direction:** Andrew replaces iA as the visual reference with his Codex screenshot. Focus on monochrome folder icons, calmer selection, typography and positioning.

**Implemented:** original gray outline folder drawing with a detached top edge and tapered body, used throughout library/header/organizer; document rows use outline page icons. Neutral rounded selection replaces blue navigation highlights; file dividers/selection stripe removed. Navigation text 14px, regular-weight folder/document headers; document title aligned left. Sidebar surface and spacing softened. Focus rings appear for keyboard navigation rather than every mouse click. Existing native unified window controls retained; their exact dimensions/corners were not customized.

**Checks:** build and 25 tests passed. Native inspection of the updated build is pending because the existing Dev window has unsaved changes. User asked whether those may be saved before refreshing. No sample or private writing has been overwritten. No new screenshot is claimed until that check completes. The user reference screenshot contains private material and is not copied into the repository.

**Artifacts:** packaged dist/Omawrite.app; running dist/Omawrite Dev.app still contains the previous cycle pending safe refresh. Full dark/narrow-window audit remains outstanding.

**Usability:** compare the outline folders, neutral selections and header placement with Codex; check sidebar labels at normal window size.

Cycle 13 packaging and deep/strict signature verification passed. Native visual verification and Dev refresh remain pending the unsaved-document decision.

## Menu audit planning pass — 12 September

Andrew requested screenshots and separate menu inventories for iA Writer, Typora and Omawrite Dev, then an incremental comparison/selection table. Created docs/menu-comparison.md and research/menu-audit-2026-09-12/ with per-app inventory records and capture checklist. Verified iA File today and Omawrite menus against source; remaining iA details are from the previous native audit. Typora commands and all fresh screenshots remain pending app-access approval: automatic review blocked potential incidental exposure of private document/sidebar content. No product code changes or build/test run needed. Suggested first implementation pass: View, using existing functionality. Raw capture folder is ignored by Git. This audit is not complete.

## Screenshot parity planning pass — 17 September 2026

**Requested scope:** inspect Andrew’s new iA menu screenshots, log existing/partial/missing behavior and stage development toward functional parity.

**Completed:** viewed all seven images in screenshots/ (File, Edit, Format, View, Focus, Go and Window); compared current QML/C++ menus and supporting behavior; created [command-level log](ia-menu-parity-2026-09-17.md) and [staged plan](development-plan.md). Updated inventory for confirmed Go commands and already-implemented bar visibility. Prior working-tree changes preserved.

**Verification:** documentation/source review only; no editor changes, build/test run, app attachment or new runnable artifact. Last recorded Cycle 13 build/25 tests/package success remains historical. Native Cycle 13 closeout is still pending.

**Known gaps:** nested reference menus, export choices, and app/Authors/Help screenshots absent from this batch. Supplied images include private surrounding content and have not been staged or copied into research. Full parity remains a multi-stage target, including advanced language/provenance and native lifecycle behavior.

**Next runnable increment:** Cycle 13 QA closeout, then Cycle 14 shared native View/Focus/formatting actions. Existing artifacts remain dist/Omawrite.app and dist/Omawrite Dev.app; no refresh claimed.

**Optional exercise:** read the File and Go tables, then identify the three missing commands most useful in daily work; use that feedback to reorder later increments. No feedback is required to understand or use the proposed sequence.

## Cycle 14 — shared native workspace menus

**Planned scope:** expose existing View/Focus/inline-format functionality through native menus with shared state; preserve document behavior.

**Implemented:** shared command registry and native menu adapter; pane/bar visibility, sorting, dates/excerpts, text size, source markup, preview/layout/typeface/reload, outline/statistics/fullscreen; Focus paragraph/typewriter; Format strike/inline code. No new shortcuts or nonfunctional reference-menu placeholders. Organizer is unavailable when hidden by library/narrow-window constraints.

**Correction found by tests:** selected-text wrapping previously required multiple undo steps. C++ QTextCursor edit blocks now make bold/italic/strike/code wrapping a single undo operation, retaining the inner selection.

**Checks:** build passed; 28 automated tests passed with normal macOS access. The first restricted run failed the pre-existing file-watcher test; the normal-access run passed. Tests cover shared menu states, size bounds, source/undo preservation, preview parsing and formatting undo/redo. Existing Qt Material teardown warnings remain.

**Native verification:** pending. App attachment returned no-window/timeout errors; process check found the old Dev app running. Preparation guard refused replacement, preserving possible unsaved work. Andrew has been asked to save/quit normally. No new screenshot or Cycle 13 visual closeout is claimed. Dark-mode, keyboard, restart-persistence, local-image reload and fullscreen native checks remain.

**Evidence/artifacts:** [cycle records](../research/cycle-14/README.md), [usability checklist](../research/usability/cycle-14.md); packaged dist/Omawrite.app is the new build, stable dist/Omawrite Dev.app remains old pending safe refresh. Changes are uncommitted.

**Optional exercise:** configure the workspace through View, toggle Focus, format a sample word and undo once. Native QA closeout comes before Cycle 15.

## Cycle 15 — everyday file access (17–18 September 2026)

**Planned scope:** File New variants, New Folder, recents, Finder/library reveal and Go locations; preserve UTF-8 storage, unsaved prompts and recovery.

**Implemented:** New in current window; existing New Window retained; new library file in current/new window; New Folder; shared File/Go recents; dynamic locations and Add Location; Finder and explicit library reveal. File creation waits for the unsaved decision. Failed open/create preserves the current buffer/recovery. Removed duplicate Full Screen item after native inspection identified AppKit’s automatic entry.

**Checks:** build and 31 automated tests passed; stable Dev bundle prepared/signature verified. Final package/signature logs are in [Cycle 15 records](../research/cycle-15/README.md). No shortcuts reassigned.

**Native results:** old clean QA window closed safely; Cycle 14 View menu inspected. Cycle 15 recents opening, location switching, library file creation, unsaved New/Cancel, sample Save and filtered-file reveal verified. Native sample is examples/Cycle15-QA.md. Screenshot capture unavailable. Finder inspection stalled overnight and returned Desktop; Finder selection remains unverified. Native picker attempt was inconclusive (Open disabled), cancelled; recents opening worked.

**Gaps:** native new-window creation, folder creation, Add Location, picker reproduction, Finder selection, dark/narrow/keyboard and remaining Cycle 14 checks. Source and tests do not establish full iA parity. No private screenshots saved.

**Runnable artifacts:** dist/Omawrite Dev.app refreshed to Cycle 15; dist/Omawrite.app packaged separately. Changes uncommitted. [Optional exercise/checklist](../research/usability/cycle-15.md): create/reopen/reveal a sample and cancel unsaved New. Next feature stage: Cycle 16 duplicate/rename/move, with the remaining native checks tracked explicitly.

## Cycle 15b — blue folder accents (18 September 2026)

**Planned scope / feedback:** Andrew likes the current outline shapes and asks for blue folder icons, retaining gray document icons. Apply the distinction to Locations, folder favorites, library rows and the folder header. Keep neutral selection backgrounds and existing text colors. This is a small polish pass before resuming Cycle 16’s duplicate/rename/move work.

**Verification plan:** build and existing regression suite; refresh the stable Dev app only after normal close; visually inspect sample folders/documents, record screenshots if capture is available. No new behavior tests needed for this color-only change.

**Cycle 15b result:** blue outlines applied to folder buttons and library folders; document icons stay gray. Build and 31 tests passed. Closed the clean Dev sample normally, refreshed the stable bundle, reopened Workspace-tour.md and verified light-mode folder/document distinction, header and selected-location appearance. [Screenshot and records](../research/cycle-15b/README.md), [optional review](../research/usability/cycle-15b.md). Private recents collapsed; no source document edited. Dark-mode and folder-favorite appearance remain unverified natively. Packaged artifact/signature logs saved under research/cycle-15b/logs/. Both app bundles contain this polish pass. Next scope remains Cycle 16a duplicate/rename, followed by 16b move.

## Cycle 16a — Duplicate and Rename (20 September 2026)

**Planned scope:** add sibling-file Duplicate and Rename to File. Duplicate copies the current editor text (including unsaved edits) without switching or saving the original. Rename changes the saved file’s name in its current folder, retaining the open buffer and undo; update recents/favorites, watcher and recovery. Reject collisions, path separators, unavailable files and external disk changes. Preserve Save As and existing shortcuts. Move To remains Cycle 16b.

**Acceptance:** filesystem/content/dirty-state/undo tests; native sample-only dialog, collision, cancel and successful operations; stable QA bundle refresh and package/signature checks.


**Implemented:** File Duplicate… and Rename… with a shared name dialog and inline errors. Duplicate writes the current UTF-8 buffer to a new sibling, leaving the original active and dirty state unchanged. Rename preserves text/undo and updates the active path, watcher, recovery snapshot, favorites and recents. Existing names, invalid names, symlinks and unavailable sources are refused; Rename additionally refuses detected disk changes. Save As remains unchanged.

**Checks:** build passed; 32 automated tests passed with normal macOS access, including Unicode content/names, no-overwrite behavior, invalid paths, dirty buffer preservation, undo/redo, organizer paths, recovery snapshot contents/new URL, save to the renamed path and external-change detection after rename. Existing Qt Material teardown warnings remain. Stable Dev preparation and packaged app signature verification passed.

**Native verification:** sample Duplicate collision and success (original stays active), opening copy through Recents, Rename collision and Cancel, successful dirty rename, undo/redo and Save all passed. Sample-only [collision screenshot](../research/cycle-16a/screenshots/rename-collision.png) visually inspected. Left Dev open on the clean examples/Cycle16-QA-renamed.md sample; prior user document was closed normally without edits.

**Known gaps:** case-only renames on case-insensitive volumes are refused; source symlinks and unsaved untitled documents require saving to a regular file first. Disk-full/permission fault injection, recovery relaunch, concurrent writers in the rename interval, dark/narrow dialog checks and cross-window path coordination are not verified. Move To remains 16b; earlier native QA gaps remain tracked. This is implemented local Duplicate/Rename behavior, not a claim of complete iA parity.

**Artifacts / exercise:** dist/Omawrite Dev.app and dist/Omawrite.app refreshed. [Evidence](../research/cycle-16a/README.md), [optional exercise](../research/usability/cycle-16a.md). Changes remain uncommitted alongside earlier cycles. Next: 16b Move To, then 17 search/edit menus. Current plan has 22 listed increments remaining across the unfinished Cycle 16 and Cycles 17–28, plus outstanding QA closeout.


## Cycle 16b — Move To (20 September 2026)

**Planned scope:** File → Move To… chooses an existing local destination folder, keeps the filename and unsaved buffer/undo, and updates path, recents/favorites, watcher and recovery. Refuse collisions, unavailable/symlink sources and disk changes. Copy saved bytes exclusively, verify destination and recheck source before removing original; failure keeps the original active and reports any retained copy. Relative document links resolve from the new folder. Native cancel/success and automated safety checks, build/test, stable QA refresh and packaging required.


**Implemented:** native File → Move To… with a Qt folder picker on macOS. Keeps the filename and unsaved buffer/undo; moves saved disk contents using an exclusive copy, reads back the copy and rechecks the source before deleting the original. Collision, symlink source and unavailable/external-change errors preserve the active document. Failed source removal leaves both files and reports this explicitly. Success updates active URL/base URL, watcher, recovery and recents/favorites. Relative links resolve from the new folder; linked assets are not moved.

**Checks:** build passed; final 33-test suite passed, including QML picker open/accept, Unicode, same-folder no-op, missing/nonlocal destination, existing file and dangling-link collisions, read-only source-directory removal failure, dirty buffer/undo/redo/save, recovery URL/text, favorites/recents and external-change watching at the new path. An initial synthetic picker test attempted selection before opening it; corrected to open the dialog before choosing a destination. Existing Qt Material teardown warnings remain. Stable Dev preparation and package/signature checks passed.

**Live app verification:** user granted inspection after an automatic-review block; clean prior sample closed normally and stable Dev refreshed. Native file/folder pickers kept Open disabled, reproducing the earlier picker issue. Move To now uses Qt's built-in picker on macOS; destination navigation/selection and collision error passed. Cancel preserved the source. [Sample-only screenshot](../research/cycle-16b/screenshots/move-collision.png) visually inspected. QA stopped when the user switched Dev to their own document; no private screenshot saved and no further app actions taken.

**Remaining:** successful live-app move/recents reopen, separate-volume device test, disk-full/disconnection, crash/recovery relaunch, dark/narrow/keyboard audit, concurrent-writer race between verification and deletion, and cross-window coordination. Extended file metadata/ACL preservation is not promised beyond QFile copy behavior. The native Open picker issue remains outside this increment; Move To's Qt fallback is usable. Core move semantics and picker integration pass automated tests, but live success is still pending.

**Artifacts / next:** both dist/Omawrite Dev.app and dist/Omawrite.app refreshed; [records](../research/cycle-16b/README.md), [optional exercise](../research/usability/cycle-16b.md). Changes remain uncommitted with previous cycles. Next feature stage: Cycle 17 search/edit menus, with Move To live QA closeout first. The plan has 21 later feature increments across Cycles 17–28 plus outstanding verification.


## Cycle 16c — delayed path tooltips (20 September 2026)

**Feedback / scope:** Andrew reports the full-path tooltip appears immediately and wants approximately two seconds of hover.

**Changed:** Locations, Favorites and Recents now use ChromeButton's shared hint/visibility/delay instead of defining attached tooltips in each delegate. Added configurable tooltipDelay; path rows explicitly use 2000ms. Library path tooltips also use 2000ms. Ordinary toolbar hints retain 700ms, and accessible sidebar names remain the short file/folder labels.

**Checks:** build and all 33 existing tests passed; diff whitespace check passed. No new tests for this small timing change. Native timing verification and Dev refresh pending access: automatic review rejected app inspection due to possible private content; renewed approval requested despite earlier approval. Existing Dev bundle is preserved until normal close. Ordinary dist/Omawrite.app packaged and signature verified; Dev remains Cycle 16b pending access/normal close. Details: research/cycle-16c/README.md.

**Optional exercise:** hover a Recent item for less than two seconds, then hold still for two seconds; full path should appear only after the delay. Repeat for Locations/Favorites/library and move away to dismiss. This polish pass does not advance parity scope; Cycle 17 remains the next feature build after pending QA.


## Cycle 17 — search/edit menus (21 September 2026)

**Planned scope:** native Find submenu, Find/Replace/Next/Previous/selection search, Delete and editing enabled states. Shared search entry points reveal the editor from preview-only mode. C++ case-insensitive literal matching preserves original UTF-16 offsets; replacement uses a single undo block. Verify wrapping, empty/no-match queries, Unicode and replacement undo. Carry forward Cycle 16 native checks without claiming closure.


**Implemented:** native Edit → Find submenu with Find, Find and Replace, Next, Previous and Use Selection for Find; shared toolbar/shortcut entry points; Shift-Command-G for previous. Delete and editing enabled states respect the focused text field and read-only content. C++ literal case-insensitive matching uses original string offsets; single and all replacement run in one QTextCursor undo block. Empty/no-match replacement is disabled.

**Verification:** build and 34 tests passed, including Unicode offsets after İ/emoji, search wrapping, no-match states, Replace All one-step undo, single deletion replacement, selection search and Delete targeting the query field. Stable Dev refreshed after authorized inspection and normal close of the clean user document. Both app bundles packaged/refreshed; ordinary package signature verified. Native sample verified Find/Replace menu, enabled states, three replacements, one-step document undo, and previous/next wrapping (3/3 then 1/3). Sample-only screenshot visually inspected. Agent-created text discarded through the unsaved prompt; Dev left clean/untitled.

**Limits:** literal case-insensitive search only; no regex, whole-word or normalization equivalence. Full keyboard/accessibility/dark/narrow checks, hidden iA submenu options, clipboard variants and earlier Move To/separate-volume checks remain pending. Cycle 16c tooltip fix is now installed in Dev; exact live hover timing remains unverified. Changes remain uncommitted alongside earlier cycles.

**Artifacts / exercise:** dist/Omawrite Dev.app and dist/Omawrite.app; research/cycle-17/README.md and research/usability/cycle-17.md. Next: Cycle 18a block formatting. 20 listed feature increments remain across Cycles 18–28, plus outstanding QA.


**Final visual correction:** screenshot review showed the search panel covering the first source line. The editor now reserves vertical space beneath Find/Replace. Rebuilt, all 34 tests passed again, and the refreshed native sample screenshot confirms source matches remain visible. Final screenshot uses `cat CAT cat`; only agent-created test text was discarded afterward.


## Five-increment run — 18a through 20a (21 September 2026)

Andrew authorized the next five listed builds: 18a block formatting; 18b inline/structural insertion; 19a date/table/case/clear styles; 19b rich clipboard; 20a document/library history, enclosing folder, source links and recursive filename quick-open. Each stage will build/test independently; final stable bundles include all five. Unknown reference submenu details remain unverified rather than invented parity claims. Native QA will use disposable text and preserve the user's current writing.


## Cycle 18a — block formatting (21 September 2026)

**Planned scope / changes:** Heading/body, bullet/number/task lists, task toggling, blockquote, indent/outdent and line moves. C++ transformations preserve multiline selection and use one undo block.

**Checks:** ./bin/build and ./bin/test passed at this increment (35 tests). Final combined build revalidated all 39 tests; existing Qt Material teardown warnings remain. Native checks below used the final combined build, not five separately installed bundles.

**Native verification:** Native Format → Headings converted the current sample line to heading 2.

**Known gaps:** Nested Markdown containers and quoted/indented fence variants are not a complete Markdown parser; exhaustive native list/line-move checks remain.

**Runnable artifacts:** dist/Omawrite Dev.app (stable io.github.andrewjngray.omawrite.dev) and dist/Omawrite.app refreshed after normal QA-app close; ordinary package signature verification passed. Logs: research/cycle-18a/logs/. Changes remain uncommitted alongside earlier work.

**Optional exercise:** Select two sample lines, apply Numbered List, then undo once.


## Cycle 18b — inline and structural insertion (21 September 2026)

**Planned scope / changes:** Toggle bold/italic/strike/inline code; backtick-safe delimiters, atomic link insertion, fenced code block and horizontal rule insertion.

**Checks:** ./bin/build and ./bin/test passed at this increment (36 tests). Final combined build revalidated all 39 tests; existing Qt Material teardown warnings remain. Native checks below used the final combined build, not five separately installed bundles.

**Native verification:** Command-B applied and removed bold on the selected heading text, retaining the inner selection.

**Known gaps:** Exact iA delimiter semantics and exhaustive escaped/nested markup combinations remain unverified.

**Runnable artifacts:** dist/Omawrite Dev.app (stable io.github.andrewjngray.omawrite.dev) and dist/Omawrite.app refreshed after normal QA-app close; ordinary package signature verification passed. Logs: research/cycle-18b/logs/. Changes remain uncommitted alongside earlier work.

**Optional exercise:** Select a word, press Command-B twice, then try Code Block and undo.


## Cycle 19a — small editing tools (21 September 2026)

**Planned scope / changes:** ISO date, basic two-column table, Unicode case conversion and Clear Surrounding Inline Styles. Protected code/link-looking selections are refused for case/clear operations.

**Checks:** ./bin/build and ./bin/test passed at this increment (37 tests). Final combined build revalidated all 39 tests; existing Qt Material teardown warnings remain. Native checks below used the final combined build, not five separately installed bundles.

**Native verification:** Native Change Case → UPPERCASE converted the selected sample heading text.

**Known gaps:** Clear Styles is limited to surrounding inline markers; case conversion is not language-specific title casing. Native date/table coverage remains pending.

**Runnable artifacts:** dist/Omawrite Dev.app (stable io.github.andrewjngray.omawrite.dev) and dist/Omawrite.app refreshed after normal QA-app close; ordinary package signature verification passed. Logs: research/cycle-19a/logs/. Changes remain uncommitted alongside earlier work.

**Optional exercise:** Select a word and change its case; insert a table on a blank line and undo.


## Cycle 19b — rich clipboard (21 September 2026)

**Planned scope / changes:** Copy As Markdown, HTML source or Formatted Text; Paste As Plain Text or Markdown from HTML. Clipboard conversions use Qt Markdown/HTML and atomic replacement.

**Checks:** ./bin/build and ./bin/test passed at this increment (38 tests). Final combined build revalidated all 39 tests; existing Qt Material teardown warnings remain. Native checks below used the final combined build, not five separately installed bundles.

**Native verification:** Formatted Text copy → Markdown from HTML paste preserved sample heading structure; one undo restored exact source. Sample screenshot visually inspected.

**Known gaps:** Cross-application paste interoperability remains untested; Qt conversion normalizes Markdown and is not lossless for every extension. Authorship paste is future work.

**Runnable artifacts:** dist/Omawrite Dev.app (stable io.github.andrewjngray.omawrite.dev) and dist/Omawrite.app refreshed after normal QA-app close; ordinary package signature verification passed. Logs: research/cycle-19b/logs/. Changes remain uncommitted alongside earlier work.

**Optional exercise:** Copy a sample heading as formatted text, paste as Markdown, then undo once.


## Cycle 20a — navigation and quick-open (21 September 2026)

**Planned scope / changes:** Independent per-window document and library histories, cursor restoration, enclosing folder, basic source-link opening and cancellable recursive filename Quick Open. Search excludes hidden/symlink/build/dependency folders and caps at 20,000 entries or 100 results.

**Checks:** ./bin/build and ./bin/test passed at this increment (39 tests). Final combined build revalidated all 39 tests; existing Qt Material teardown warnings remain. Native checks below used the final combined build, not five separately installed bundles.

**Native verification:** Quick Open found two disposable fixtures and Return opened the first. Open Link at Cursor opened the second; Back restored the first cursor. Forward with unsaved sample changes prompted; Cancel preserved them. Agent edits were undone and sample saved clean. Sample-only screenshots visually inspected.

**Known gaps:** History is session-local and New resets it; not coordinated across windows. Source links support basic inline/autolinks, not reference links or nested parentheses. Local nontext targets are unsupported. Native large-library stress, full keyboard/dark/narrow checks and earlier Move To/separate-volume/tooltip timing QA remain pending.

**Runnable artifacts:** dist/Omawrite Dev.app (stable io.github.andrewjngray.omawrite.dev) and dist/Omawrite.app refreshed after normal QA-app close; ordinary package signature verification passed. Logs: research/cycle-20a/logs/. Changes remain uncommitted alongside earlier work.

**Optional exercise:** In the examples location, Quick Open “Cycle20”; follow Second sample, go Back, edit, then try Forward and Cancel.

## Cycle 20b — command palette and sentence focus (in progress)

Planned scope: searchable existing workspace commands with keyboard navigation and live enabled/check states; sentence focus using Unicode sentence boundaries within the current paragraph, with fenced source focused by line. Paragraph and sentence modes are mutually exclusive. This is a defined local subset, not an English grammar/abbreviation model or full-menu palette. Build/test and native sample checks required before closeout. Remaining cycles 21–28 retain their existing scope and are not marked complete.


## Cycle 20b — Command palette and sentence focus (21 September 2026 closeout pass)

**Planned scope / changes:** Search the shared workspace registry; Unicode sentence focus within the active paragraph.

**Verification:** Command-Shift-P, query → Return, and visible sentence dimming passed natively. Boundary tests pass. Final combined 48-test suite passes.

**Known gaps:** Palette covers the existing workspace registry, not every new file/edit command. Unicode sentence boundaries are not an abbreviation/language grammar model; broader language/performance checks remain.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-20b/README.md). Open Command-Shift-P, type sentence, press Return, and move through sample sentences.


## Cycle 21a — Bounded saved-content search (21 September 2026 closeout pass)

**Planned scope / changes:** Recursive filename/content search off the UI thread; a per-query in-memory content index is rebuilt from saved files. Limits: 100 results, 20,000 entries, 256 KiB per file, 32 MiB total; unreadable/oversized files reported. Hidden/symlink/build/dependency folders excluded.

**Verification:** Automated saved edit/rename/delete and stale-query checks pass. Live exact-tag content search found the sample. Final combined 48-test suite passes.

**Known gaps:** Not a persistent index: rereads bounded files. Unsaved buffers are excluded. Tree/list/date options not confirmed beyond existing controls; native large-library stress remains.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-21a/README.md). Quick Open → Search saved file contents too; search a phrase absent from filenames.


## Cycle 21b — Saved queries and tags (21 September 2026 closeout pass)

**Planned scope / changes:** Persist up to 30 root-scoped queries; remove/reopen them in Quick Open. #tag queries match case-folded tags outside basic fences/inline/indented code. Open content searches refresh every five seconds. Hashtag insertion added.

**Verification:** Saved-query persistence and code exclusion tests pass. Native #sample search and Save query passed; persisted query survived restart. Final combined 48-test suite passes.

**Known gaps:** Saved queries are in Quick Open, not organizer smart-folder rows. Nested Markdown code cases and a browsable tag index remain; these are partial smart-folder semantics.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-21b/README.md). Search #sample with content enabled, save the query, and reopen it.


## Cycle 22a — Preview navigation (21 September 2026 closeout pass)

**Planned scope / changes:** Unique heading slug targets, inserted Markdown TOC, and opt-in proportional bidirectional scrolling with a feedback guard.

**Verification:** Duplicate-heading TOC and actual rendered anchor lookup pass. QML loading/regression tests pass. Final combined 48-test suite passes.

**Known gaps:** Scroll synchronization is proportional, not semantic paragraph/image alignment. Full long-document native anchor/scroll and cross-file fragment checks remain.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-22a/README.md). Insert a TOC into a disposable document and follow duplicate-heading links in preview.


## Cycle 22b — Markdown extension subset (21 September 2026 closeout pass)

**Planned scope / changes:** Preview-only ==highlight==, explicit relative [[target|label]] links, single-line footnotes and bounded local text/Markdown content blocks. Literal code preserved; include depth 5, 256 KiB per include, 1 MiB total, cycles/escaping outside the current include directory refused.

**Verification:** Automated literal-code/include-cycle/traversal tests pass. Native sample renders highlights, wikilink, footnote appendix, table and literal fenced code. Screenshot reviewed. Final combined 48-test suite passes.

**Known gaps:** Wikilinks do not search nearest matches across the library. Multiline/backlinked footnotes, CSV/image/code content blocks, full title syntax and rebasing ordinary relative links inside nested includes remain. Missing includes show a message.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-22b/README.md). Open examples/Cycle28-QA.md and compare source to preview.


## Cycle 23a — Export and print (21 September 2026 closeout pass)

**Planned scope / changes:** Atomic HTML/PDF export, rendered/source print and native page setup. Exports preserve editor source/dirty state and refuse overwriting the active source. HTML carries its source base URL.

**Verification:** 48-test final suite includes real HTML/PDF output checks. Basic Unicode heading/bold PDF rendered with Poppler and visually inspected. Final combined 48-test suite passes.

**Known gaps:** Native export picker cancellation, multi-page/image/table layout, page-break insertion and portable asset bundling remain unverified/unimplemented. Exported HTML links local assets; moving the HTML alone is not a self-contained export.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-23a/README.md). Export a disposable sample as PDF and inspect every page before sharing.


## Cycle 23b — Output style foundation (21 September 2026 closeout pass)

**Planned scope / changes:** Three independently named output font presets plus local JSON custom fontFamily/pointSize (8–32) validation.

**Verification:** Compiled and covered by QML/export regression suite; native custom-style workflow not yet verified. Final combined 48-test suite passes.

**Known gaps:** This is a font-style foundation, not complete preview/output templates. No persisted template selection, template asset system, headers/footers/title pages, paginated preview or fit-page/fit-width modes.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-23b/README.md). Try Reading Serif for an export; compare it with Clean Sans.


## Cycle 24 — Mac presentation subset (21 September 2026 closeout pass)

**Planned scope / changes:** Native Markdown share picker plus Minimize, Zoom and Bring All to Front; platform-specific Objective-C++ remains separate.

**Verification:** Native share picker opened on disposable prose and Escape dismissed it. No transmission performed. Build/QML tests pass. Final combined 48-test suite passes.

**Known gaps:** Share-sheet open/cancel passed; remaining window-presentation checks remain. Title/toolbar fade/statistics-only modes and external-display behavior are not implemented in this pass.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-24/README.md). Open Share Markdown on disposable text and cancel without choosing a destination.


## Cycle 25a — Coordinated document windows (21 September 2026 closeout pass)

**Planned scope / changes:** One Backend/QML engine per window in a shared process; same-file opens focus existing windows. Quit closes sequentially through normal save prompts; cancellation/save failure stops the sequence. Recovery slots remain isolated.

**Verification:** Native two dirty windows: New preserves first, Quit/Cancel preserves both, then Discard advances to second prompt. Tests cover new-window request and save-failure cancellation. Final combined 48-test suite passes.

**Known gaps:** Separate application launches are not consolidated. Saved-window restoration across restart and exhaustive crash/OS-quit/minimized-window matrix remain. Clean windows already closed before a later Cancel are not reopened automatically.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-25a/README.md). Create two disposable dirty windows, Quit, then Cancel before discarding either.


## Cycle 25b — Native tabs (21 September 2026 closeout pass)

**Planned scope / changes:** AppKit merge, next/previous, detach, overview and tab-bar controls. Reserve native tab-bar height below custom toolbar.

**Verification:** Live merge produced two native tabs; switching retained text; detach preserved unsaved text. Screenshot found a covered first line; corrected inset and reviewed final screenshot. Final combined 48-test suite passes.

**Known gaps:** Restart restoration/tab-group persistence and exhaustive dirty-close/overview/fullscreen/keyboard states remain. Some tab commands remain enabled when not applicable.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-25b/README.md). Merge two sample windows, switch tabs, detach one and verify its draft.


## Cycle 25c — Opt-in autosave and saved versions (21 September 2026 closeout pass)

**Planned scope / changes:** Optional one-minute autosave for saved documents with disk-content comparison; pause on external changes. Explicit native NSFileVersion checkpoint of saved bytes under file coordination. Restore into editor as one undo step; disk changes only on Save.

**Verification:** Autosave external-change protection tested. Native checkpoint creation, listing, restore and undo back to prior dirty sample passed. Final combined 48-test suite passes.

**Known gaps:** No automatic version capture per save or native Versions-browser UI. Versions cover Markdown, not authorship sidecars. Autosave has a remaining external-writer race between comparison and write; disk-full/disconnection/crash stress remains. Default is off.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-25c/README.md). Create a saved-file version; edit a sample; restore; undo before deciding whether to Save.


## Cycle 26a — Native spelling subset (21 September 2026 closeout pass)

**Planned scope / changes:** Selection spelling review through NSSpellChecker and Emoji command. Native platform Edit menu also exposes system Dictation/AutoFill where available.

**Verification:** Native selection spelling correctly reported the sample typo “quikc”. Build/QML tests pass. Emoji/dictation remain unverified. Final combined 48-test suite passes.

**Known gaps:** Inline spelling/grammar, automatic substitutions/corrections, speech and completion integration remain. Selection spelling reports words only; no suggestions or replacement workflow.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-26a/README.md). Select disposable prose and run Check Selection Spelling.


## Cycle 26b — Writing analysis foundation (21 September 2026 closeout pass)

**Planned scope / changes:** Selection analysis through system lexical classes and a small review-word list; comma-separated custom review words. Limit first 50,000 characters and 1,000 results.

**Verification:** Native analysis reported sample Determiner/Adverb/Adjective/Noun classes and flagged “really” as a review word. Screenshot inspected. Broader language corpus remains unverified. Final combined 48-test suite passes.

**Known gaps:** Analysis is an explicit panel, not live parts-of-speech highlighting/style checking. Select prose manually; code not excluded automatically. No exact iA linguistic-output claim.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-26b/README.md). Select prose, Analyze Selection, and judge whether review words are useful.


## Cycle 27 — Manual authorship annotations (21 September 2026 closeout pass)

**Planned scope / changes:** Explicit Human/AI/Reference labels with optional author; Unknown clears labels. QTextDocument formatting ranges participate in undo and edits. Hidden .<filename>.omawrite-authors.json sidecar stores ranges with SHA-256 source match; external text changes invalidate them. Recovery snapshots include annotations.

**Verification:** Native selected Reference label and Save passed. Tests cover mark undo/redo, sidecar save/reopen and external-edit invalidation. Final combined 48-test suite passes.

**Known gaps:** Annotations are assertions, not verified provenance. Inserted text can inherit nearby labels. Clipboard/export, Duplicate/Move/Rename sidecar migration, paste-edits/author-aware merging and full attribution visualization remain. Native versions do not version sidecars.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-27/README.md). Select sample words, mark Reference, Save; keep the hidden sidecar with the Markdown file.


## Cycle 28 — Release/parity audit — still open (21 September 2026 closeout pass)

**Planned scope / changes:** Integrate new slices, review samples, run regressions, package/sign locally and record remaining acceptance gaps honestly.

**Verification:** Final build and all 48 tests passed. Ordinary and stable Dev bundles refreshed; package deep/strict signature verification passed. Sample-only screenshots reviewed and basic exported PDF visually inspected. Final combined 48-test suite passes.

**Known gaps:** Not full parity or release sign-off. Complete the acceptance gaps in this matrix and earlier Move To success/separate-volume, exact tooltip timing, clipboard cross-app, accessibility/dark/narrow/picker checks. No notarization/public release.

**Artifacts / exercise:** Both stable bundles refreshed. [Evidence](../research/cycle-28/README.md). Use the new sample and report one concrete surprise at a time.


## Cycle 29 — Authorship through file operations (closeout increment)

**Planned scope / changes:** Preserve authorship sidecars through Duplicate, Rename and Move. Duplicate records current buffer annotations, including unsaved changes. Rename/Move carry saved metadata with saved Markdown while retaining dirty annotations and undo in the editor. Destination metadata collisions and source metadata symlinks block migration. Metadata is staged before changing the source path; failed duplication rolls back its newly created metadata. If old metadata cannot be safely removed, retain it and report cleanup needed.

**Verification:** `./bin/build` passed; `./bin/test`: **51 passed, 0 failed**. Regression coverage includes dirty annotations, save/reopen, collision protection, rollback, rename-dialog window retention, undo and recovery snapshot metadata. Native sample Duplicate, Rename and same-volume Move passed; resulting sidecar hashes and annotation ranges verified on disk. A previous apparent empty-window switch was investigated: the original renamed document remained open in the same process and reopening its path focused it. Repeat Rename kept the correct window active. No data-loss reproduction and no speculative focus fix; the original focus switch remains unexplained.

**Known gaps:** Two-file operations are not crash-atomic transactions. Cross-volume/device-removal and concurrent external-writer stress remain unverified. Clipboard/export provenance, native version sidecars and full recovery-relaunch matrix remain future work. Session/tab restore and cross-launch ownership are separate closeout items. Cycle 28 release/parity audit remains open.

**Artifact / exercise:** `dist/Omawrite Dev.app` and `dist/Omawrite.app`; [evidence](../research/cycle-29/README.md). On a disposable annotated document, Duplicate, Rename and Move, then reopen and check authorship. Report any repeat of an unexpected focus change.


## Cycle 30 — Open a file or folder by path

**Planned scope / changes:** User-requested File → Open by Path command (Shift–Command–O on macOS), also available in the editor file menu. Accepts absolute local paths, `~/`, paired outer quotes and local `file:///` URLs. C++ validates and canonicalizes existing readable folders or Markdown/text files; QML routes files through the existing unsaved-change guard and folders to the library without replacing the current document. Inline errors keep the dialog open.

**Verification:** Build passed; **53 tests passed**. New tests cover spaces/Unicode/hash characters, encoded URLs, quotes, home expansion, symlinks, missing/unsupported/nonlocal paths, folder opening with a dirty document, and file-open Cancel/Discard. Native Shift–Command–O, home-relative file path, folder path, invalid-path message and dirty-open Cancel passed. Sample-only error screenshot reviewed.

**Known gaps:** No relative paths, shell expansion, environment variables, path completion or remote URLs. File types match the library's Markdown/text extensions. Dark/narrow and permission-revocation races not independently exercised.

**Artifacts / exercise:** Stable `dist/Omawrite Dev.app` and packaged `dist/Omawrite.app`; [evidence](../research/cycle-30/README.md). Press Shift–Command–O and paste `~/Documents`, then try a Markdown file path containing spaces.


## Cycle 31 — Filter field label and rounded edges

**Scope / changes:** Andrew reported the filter placeholder floating above its outline on focus, and the right curve disappearing when narrowing. Use a Basic TextField with explicit colors/padding/insets, a fixed placeholder, a content-independent implicit width, zero minimum layout width and a maximum constrained to the library's inner width. Rounded background follows control height.

**Verification:** Build and 54 tests pass. QML geometry checks cover 1100/900/720-pixel windows and long input while focused. Native focus/typing checked on a sample-only library; fixed label and both rounded ends visually inspected. Screenshot in research/cycle-31. Native resize gestures did not resize the window, so minimum-width verification is automated rather than a claimed native drag pass.

**Known gaps:** Independent native minimum-width drag and dark-mode visual checks remain. Existing broader closeout gaps unchanged.

**Artifact / exercise:** Stable Dev and packaged app updated. Focus Filter files, type a query, and narrow the library; both ends should remain inside its bounds. See [evidence](../research/cycle-31/README.md).

## Cycle 32 — Workspace restart and launch coordination

**Scope / changes:** One owner per installed executable, with local-socket forwarding for repeated launches and canonical-path duplicate focus. Saved-file windows retain bounds, library root and cursor; macOS tab grouping/order and last active document survive Quit/relaunch. Workspace metadata uses atomic owner-readable files. Existing orphan recovery snapshots are opened before saved-session files so dirty recovered buffers are not overwritten by restoration. Save As during a Quit prompt updates the saved session URL. OS Quit events use the guarded close sequence.

**Verification:** Build and **56 tests passed**. Added workspace round-trip/corrupt-input/rejected-write checks and concurrent secondary-launch forwarding, including Unicode/spaces and activation-only requests. Native sample checks: second executable opened another file and exited; repeated launch selected the existing tab; Merge All Windows, Quit/relaunch restored two tabs in order; cursor position 2 survived; dirty Quit → Cancel retained text, Undo restored the sample, and Save completed Quit. Final active-document state stays recorded while another app has focus. Reviewed sample-only screenshot retained.

**Known gaps:** Dev and ordinary app installations are deliberately separate owners. Missing saved files and clean untitled windows are skipped. Normal bounds are restored; minimized/fullscreen state and external-display placement are not reproduced. Existing sequential quit can close earlier windows before a later Cancel; it does not reopen those windows. Crash/recovery/disk-full and multiple dirty-window/OS-shutdown stress remain in Cycle 34. Session metadata has a one-second checkpoint interval; existing dirty-text recovery remains separate. Native Save As during Quit not independently exercised.

**Artifacts / exercise:** Stable `dist/Omawrite Dev.app` and `dist/Omawrite.app`; [evidence](../research/cycle-32/README.md). Open two disposable documents, merge them as tabs, place the cursor partway through one, Quit and reopen. Verify order and cursor; explicitly closing a document removes it from the next restored session.

**Next:** Cycle 33 themes. [Ten cycles remain in the updated plan](remaining-cycles.md), including themes; full iA parity remains unclaimed.

## Cycle 33 — Workspace themes

**Scope / changes:** Aa → Theme offers Follow system (reset), Light, Dark and Warm paper. Persisted selection updates existing windows. Shared palette covers writing surfaces, preview, library/organizer, chrome, filter, menus and in-app dialogs; blue folder/gray file icons retained. macOS native window appearance follows the selected light/dark mode. Export/print styling is independent.

**Verification:** Build and 57 tests passed. Tests invoke theme menu actions, check persistence/manual overrides/reset, text/modified preservation and 4.5:1 text/muted contrast against panels. Native Warm paper and Dark visually inspected on synthetic sample tabs; restart retained Dark and switching to the second tab retained the theme. Native tab bar now changes appearance. Sample-only screenshots reviewed.

**Limits:** Bundled presets only; no Typora CSS imports or theme editor. macOS native tab labels can be faint while the app is inactive; broader accessibility/system-picker/high-contrast audits remain in Cycle 41. Linux custom Omarchy colors retain their existing behavior in Follow system and are not native-tested here.

**Artifact / exercise:** Stable Dev build; ordinary package refreshed with the following safety increment. Aa → Theme → Warm paper, then Dark; compare the library and preview and report a preferred default. Reset with Follow system. [Evidence](../research/cycle-33/README.md).

## Cycle 34a — Recovery baseline and cancellable multi-document Quit

**Scope / changes:** Recovery snapshots now retain the last known disk baseline with owner-only permissions. Autosave after recovery refuses externally changed files; older snapshots without a baseline recover text but require manual Save. Autosave checks read errors and rechecks the target after staging a write; short writes report failure. Quit gathers Save/Discard decisions for every document before closing any windows. Document revisions invalidate earlier approval if edited before the final close. A later Cancel retains earlier drafts/windows, including recovery data.

**Verification:** Build and **59 tests passed**. An isolated test subprocess created two annotated drafts, waited for snapshots, and exited without cleanup; the parent recovered both text and annotations. One file was externally changed between exit/recovery and autosave preserved that external content; the unchanged file autosaved normally. QML test confirms discard approval does not close or clear a draft and later edits change its revision. Native two-dirty-tab Quit: Discard First → Cancel Second retained both drafts/tabs; repeat Discard both exited normally. Reviewed synthetic screenshot retained.

**Known gaps / remaining Cycle 34:** This closes two concrete safety issues, not the entire safety cycle. A non-cooperating writer can still change a file in the short interval between final comparison and atomic rename; portable filesystem APIs do not provide compare-and-swap. Physical disk-full/removal, fullscreen/minimized/external-display restoration and full OS-shutdown stress remain. Version snapshots still cover Markdown rather than sidecars; no automatic per-save version history or native Versions browser. Save As during multi-document Quit still needs dedicated native verification. No fabricated completion count: nine planned cycles remain, including the unfinished portion of 34.

**Artifact / exercise:** Stable Dev and ordinary app refreshed; [evidence](../research/cycle-34a/README.md). On two disposable drafts choose Quit, Discard on the first and Cancel on the second. Both should remain open with their text. Then save or discard normally.

## Cycle 34b — Safe version restoration

**Scope / changes:** Native versions remain Markdown-only snapshots. Restoring a version now inserts unlabelled historical text instead of inheriting current authorship; one Undo restores the preceding text and annotations together. Restoring explicitly pauses autosave until a successful manual Save. The pause is written to recovery metadata and restored with the draft, and the version dialog explains this policy. New/open documents reset the pause.

**Verification:** Build and **61 tests passed**. A native NSFileVersion test creates a saved version, changes and annotates current text, restores, verifies autosave leaves disk untouched, verifies one-step Undo/Redo of text and labels, then saves/reopens unlabelled text. It checks the recovery snapshot records the autosave pause. Failed Save As to a missing destination preserves the current URL, draft, original file and Undo. Native Dev sample Create Version → edit → Restore → Undo passed; status and dialog explain the pause. Disposable edit discarded on normal Quit. Sample-only screenshot reviewed.

**Limits / remaining Cycle 34:** Native snapshots do not include sidecars; this cycle defines safe unlabelled restoration rather than claiming historical provenance. No automatic version capture on each save or native Versions-browser UI. Disk-full/device-removal, fullscreen/minimized/display restoration and OS shutdown matrix remain. The pause remains after Undo until manual Save, conservatively requiring an explicit decision. Full crash/relaunch of a version-restored draft was not separately exercised; recovery flag serialization is tested. Nine planned cycles remain, including unfinished Cycle 34.

**Artifact / exercise:** `dist/Omawrite Dev.app` and `dist/Omawrite.app`. [Evidence](../research/cycle-34b/README.md). On a sample file create a saved version, edit, restore it, then Undo. Choose Save explicitly to accept restored text.

## Cycle 34c — Automatic previous-version history

**Scope / changes:** Opt-in File → Keep Previous Version on Save preserves changed existing saved bytes before replacement. Checkpoint failure blocks Save; unchanged saves avoid duplicate versions. Markdown-only version policy retained.

**Tests:** Build and 62 tests pass, including native NSFileVersion previous-byte capture and deduplication. Native UI verification is part of the combined Cycles 34c–38 run.

**Limits:** Not native Versions-browser UI or sidecar history. External non-cooperating-writer race, OS-shutdown, physical disk/device and display/window-state matrix remain open.

**Artifact / exercise:** Stable Dev/ordinary bundles updated at the combined run. Enable Keep Previous Version on Save on a sample; change and Save, then restore its earlier version and Undo.

## Cycle 35 — Authorship clipboard and inspection

**Scope / changes:** Editor Markdown Copy/Cut carries custom hash-bound annotation metadata; Paste validates ranges and prevents external plain text inheriting labels. One Undo reverses pasted text and annotations. Annotation rows select their spans; explicit metadata JSON export includes provenance limitations.

**Tests:** Build and 63 tests pass; range clipping, transfer, stale hash rejection, Undo and metadata export covered. Native routes checked in combined run.

**Limits:** Assertions are not verified authorship. Other apps may strip custom MIME data. Exported HTML/PDF does not embed assertions; matching Markdown and metadata are separate artifacts. Collaborative merging/full live attribution visualization remain future work.

**Artifact / exercise:** Combined stable bundles. Mark a sample Reference, copy/paste it within Omawrite, inspect its range, then Undo. Paste plain text from another app and confirm it is unlabelled.

## Cycle 36 — Footnotes and richer content blocks

**Changes:** Multiline footnotes with repeated-reference backlinks; named-anchor lookup in preview; rebased links/images in nested Markdown includes; wiki fragments; local image, escaped CSV and literal code content blocks. Existing bounds retained.

**Tests:** Build and 64 tests pass. New cases cover multiline notes, repeat backlinks, nested relative assets, CSV quoting/escaping, inline code and wiki fragments. Native combined preview check follows.

**Limits:** Not a complete Markdown parser. Nearest-match wiki lookup, complex nested inline links, semantic scroll synchronization and the full long-document/fragment matrix remain open.

**Artifact / exercise:** Combined bundles. Preview an included chapter with a local image, a quoted CSV and a multiline footnote; follow its return link. Source files remain unchanged.

## Cycle 37 — Persisted output templates and portable raster images

**Changes:** Persisted output styles/custom font, size, header/footer and optional title page. PDF/print share paginated output with page counters. Page-break insertion uses a Markdown comment. HTML embeds bounded local raster images; unavailable assets reject export and retain prior output.

**Tests:** Build and 65 tests pass. Persisted style, embedded image, page break and export rollback covered. Rendered two-page PDF visually reviewed; corrected a boundary defect that moved the preceding image to page 2. Corrected images/text/headers/footers inspected in output-final evidence. Native picker checks follow in combined QA.

**Limits:** No in-app paginated/fit-page preview; document hyperlinks may remain external. Full title-page/custom template and physical printer matrix remains open.

**Artifact / exercise:** Combined stable bundles. Select Reading Serif, insert a page break, export a two-page PDF and HTML with a local PNG. Move the HTML and confirm the image remains embedded.


## Cycle 38 — Organizer searches and browsable tags

**Scope / changes:** Saved queries appear as organizer smart folders and reopen their saved root/query/content setting. Explicit Refresh scans saved Markdown/text tags asynchronously, counts documents, and opens tag queries. Quote/list-contained fences are excluded. Independent cancellation/generation guards discard stale root results. Document shortcuts now target the active window: native two-tab QA exposed ambiguous application-wide Command-S.

**Tests:** Build and 66 tests pass. New 110-document fixture verifies counts, repeated-tag deduplication, nested code exclusion and root-reset behavior. Native sample tag scan shows two writing documents, tag click returns both files, and Save query creates a sidebar smart folder. Sample screenshots reviewed.

**Limits:** Manual refresh, saved files only; no filesystem watcher/incremental index. Bounds: 20,000 directory entries, 256 KiB/file, 32 MiB total, 2,000 tags; hidden/symlink/build/dependency paths excluded. Very large-library latency/memory and full Markdown-container grammar remain acceptance work. Quick Open results have their own limits, so counts can exceed visible results.

**Artifact / exercise:** `dist/Omawrite Dev.app` and `dist/Omawrite.app`. Open research/cycle-38/sample/Workbench.md, Show in Library, show organizer, Refresh Tags, click #writing and save the query. See [combined evidence](../research/cycle-38/README.md).

### Combined native verification — Cycles 34c–38

Native Dev opened the synthetic workbench without changing existing writing. Preview displayed the nested chapter, quoted CSV, literal code and multiline/repeated footnote. Tag counts and tag-to-Quick-Open passed; saved query appeared in organizer. Reference Copy/Paste retained both annotation ranges, and one Undo removed pasted text. Optional previous-version capture produced a native restore entry after File → Save; history preference returned to off. PDF save picker opened and Cancel returned to the saved document. PDF pages were separately rendered and inspected under cycle-37/output-final. All committed screenshots contain synthetic writing; Recents was collapsed.

Outstanding: full cross-app clipboard matrix, annotation-row focus timing, footnote click/long-scroll matrix, title-page/custom-template/printer checks, external-display and OS-shutdown/device-failure tests. These five checkpoints advance the plan; they do not close every acceptance criterion in Cycles 34–38.

Final native check: after rebuilding, Command-S saved the active Workbench.md with two tabs restored. The persisted #writing smart folder returned both sample documents after restart. QA app quit normally with saved documents.

## Cycle 39 — Native writing review

**Scope / changes:** macOS spelling suggestions, explicit language choice and opt-in grammar review with individual Replace; replacement verifies expected text and is one Undo. Review masks fenced/indented/inline code, URL destinations and raw tags while preserving UTF-16 offsets. Writing Review refreshes while its non-modal panel is open. Speak Selection and Stop Speaking use the system voice. Annotation row selection waits for its dialog to close.

**Tests:** Build and 67 tests pass, including real macOS dictionary suggestions, code exclusions and stale replacement/Undo. Native dialogs and speech checked during final combined QA.

**Limits:** First 50,000 characters, 100 spelling/grammar issues, 1,000 analysis results. Dictionaries/language grammar quality depend on macOS; no automatic correction while typing, full Markdown grammar or iA-equivalent linguistic model. Speech uses AppKit's supported but deprecated synthesizer. Hardware/audio perception and all languages need user acceptance.

**Artifact / exercise:** Combined stable Dev/everyday bundles. On a sample containing `mispellled` and fenced code, use Edit → Spelling and Grammar, replace a suggestion and Undo. Try Writing Review and Speak Selection. Evidence: research/cycle-39/logs.


## Cycle 40 — Command access and interaction

**Scope / changes:** Palette now covers file/path, save/save-as, duplicate/rename/move, reveal, quick-open/tags, export, themes and writing/annotation tools. Disabled state checks match required file/library context. Palette execution waits for closure. Path tooltips use independent two-second hover timers; annotation range selection waits until its dialog is closed. Paginated Preview shares PDF/print painting and offers fit/page/orientation controls.

**Verification:** Final combined build and 70 tests pass; palette routing/disabled rename/pagebreak Undo tested. Native palette → Spelling and Grammar and Paginated Preview passed. Both preview pages visually inspected.

**Limits:** Remaining reference submenu/context-menu and rapid-hover timing matrix is open, as are native tab-action enabled states and title/toolbar fading. See release-acceptance.md.

**Artifact / exercise:** Stable Dev and ordinary RC1. Command-Shift-P → Paginated Preview; try fit controls and Cancel. Hover a library path and report any premature tooltip.

## Cycle 41 — Integrated QA and fixes

**Scope / changes:** Dark preview links now use readable theme colors and underlines; editor accessibility name added. Session capture retains normal bounds, display name and minimized/fullscreen/maximized state; restoration clamps to the available display. Native long-document QA found Qt drops empty named anchors; rendered-link fallback now resolves footnote destinations and repeated references. Synthetic tag limits and custom title-page output validated.

**Verification:** Build and 70 tests pass, including off-screen geometry, tag caps/oversized skip, actual Qt footnote anchors and command behavior. Native spelling Replace/Undo, grammar request, paginated preview, fullscreen two-tab restart and long-note forward jump passed. Speech commands invoked; audible quality not independently verified. Custom three-page PDF rendered and all pages reviewed. Synthetic screenshots only.

**Limits:** Hardware shutdown/disks/displays, minimized matrix, VoiceOver/cross-app clipboard, exact hover timing and language breadth are not certified. A failed native resize attempt is excluded; automated width checks pass. No blanket accessibility/parity sign-off.

**Artifact / exercise:** Both RC1 bundles; research/cycle-41 contains samples, screenshots and PDF evidence. Open Long-notes.md and follow/return from the note; try the spelling panel on Closeout.md.

## Cycle 42 — Versioned Mac release candidate

**Scope / changes:** App version 0.2.0-rc1, bundle 0.2.0/build 42. Updated user guide and current acceptance ledger. Ordinary packaging now refuses to overwrite a running app, matching Dev. Archive script verifies committed compiled inputs and produces a packaged ZIP, source/build manifest and SHA-256 checksums; MIT/font licenses retained. GitHub prerelease archives the binary separately from source Git history.

**Verification:** Final build, 70 tests, shell syntax checks and deep/strict bundle-signature verification. Package and archive evidence under research/cycle-42/logs. Both stable bundles refreshed. Release metadata identifies the source commit.

**Limits / status:** RC1 is for personal use, Apple Silicon/macOS 14+, ad-hoc signed. Developer ID/notarization and other-machine install are not complete. All numbered areas now have implementation checkpoints, but outstanding software parity and hardware acceptance in release-acceptance.md remain open; this does not reset the strict remaining-work count to zero.

**Artifact / exercise:** dist/Omawrite.app, dist/Omawrite Dev.app and GitHub mac-v0.2.0-rc1 prerelease archive. Use a copy of a document for a final personal acceptance session; report any issue against the candidate tag.

Final RC1 native follow-up: modeless Writing Review stayed visible during input and updated its word classes. Long-note forward and return clicks both passed (preview scroll 100% → approximately 2% on return). Sample edit undone and saved normally.

## Cycle 43 — Location and file context menus

Scope: Andrew's iA Writer screenshot: Rename in Locations, Remove from Locations, Copy Library Path, Show in Finder; applicable path/Finder actions on library folders/files, favorites and recents.

Implemented a shared QML context menu with right-click and Menu-key invocation. Location labels persist separately from disk names. Removing a location removes only the shortcut; missing locations remain available for shortcut maintenance. Copy Path writes a decoded local filesystem path. Show in Finder targets the clicked entry, without opening it in the editor. No file deletion or arbitrary on-disk rename added to this menu.

Validation: ./bin/build and ./bin/test: 71 passed, zero failures. New coverage checks label persistence, invalid labels/targets, clipboard paths with spaces, and file preservation after shortcut removal. Initial sandbox run could not access native spelling/recovery services; rerun with required access passed. Native Dev: location menu, rename/apply/restore label, file menu without document switch, and Finder selection of Workbench.md verified. Sample screenshots/logs: research/cycle-43/. Keyboard Menu-key, dark appearance, unavailable-volume menu and VoiceOver not verified natively.

Runnable artifacts: dist/Omawrite Dev.app and dist/Omawrite.app (same current source; GitHub RC1 archive remains the previous checkpoint).

Optional exercise: right-click a location and give it a shorter label; right-click a file, copy its path, and reveal it in Finder. Confirm the folder's real name has stayed unchanged.

## Cycle 44 — Full file and folder context action sets

Scope: implement Andrew's two supplied iA screenshots for editable Markdown/text files and folders, retaining the separate Cycle 43 location-shortcut menu. Hidden submenu contents were not supplied; Omawrite uses its supported output formats.

Changes:
- Files: Open in New Tab / Window, Get Info, Favorite toggle, Duplicate, Rename, Move to Trash, Show in Finder, native Share picker, Export HTML/PDF, Print rendered/source/preview, Copy path/Markdown/plain text/HTML, New File/Folder, Sort By and View Options.
- Folders: Open as library root, Get Info, Favorite toggle, recursive Duplicate, Rename, Trash, Finder, native Share, Copy Library Path, New File/Folder inside that folder, Sort By and View Options.
- File output/duplication uses the live buffer when already open; closed-file output uses an isolated backend without claiming recovery slots or replacing an editor document. Existing documents retain one editing owner; opening an already-open file focuses it. Tabs use native AppKit groups; new windows/tabs inherit the originating library root.
- Filesystem rename relocates library roots, location labels, favorites, recents and folder navigation paths. Location rename remains a sidebar alias.
- Trash asks before moving to the system Trash, never permanently deleting. Dirty open files must be saved/closed first. A clean open file becomes an unsaved untitled draft after Trash, preserving its content for Save As. Folder rename/duplicate/Trash requires documents inside it to be closed. Folder copies stage before publication, refuse links/special files, and cap at 20,000 entries / 1 GB; larger copies can use Finder. Filesystem roots cannot be renamed, duplicated or trashed. Sharing sends the saved file/folder through the system picker, not unsaved buffer text.

Validation: ./bin/build and ./bin/test pass, 73 tests, zero failures. New tests cover clicked-file output/clipboard, refusal to export over the source, duplicate collisions, traversal rejection, recursive copying and symlink refusal, folder rename remapping, unsaved-buffer duplication/rename, dirty Trash and open-descendant folder guards. A canonical /var → /private/var rename mismatch was found and fixed.

Native Dev QA on synthetic samples: complete file/folder menus; duplicate then Trash of the created copy; New File in the clicked folder; native new-tab grouping; Get Info; correct clicked-document paginated preview while another document is active; Copy Markdown pasted into the filter then cleared; native file sharing picker displayed and cancelled without transmission. Submenu popup visibility and action-after-dismissal bugs found and fixed. Sample screenshots in research/cycle-44/screenshots. Existing documents were preserved through normal app quits.

Known gaps: no physical print job or actual sharing transmission; dark/VoiceOver/keyboard-only context traversal, unavailable volumes and cross-process editing in the separate Dev/everyday identities were not exercised. Saved smart-search roots are not migrated by folder rename. Folder copy is a bounded synchronous operation and does not preserve every macOS extended attribute/ACL. Unseen iA submenu parity remains unverified.

Artifacts: dist/Omawrite Dev.app and dist/Omawrite.app. Versioned RC1 GitHub release remains the older checkpoint; this cycle is recorded in source history and local bundles.

Optional exercise: right-click an unopened note, duplicate it, rename the copy, export/preview it and move the copy to Trash. Right-click a folder and create a file inside it; try the sorting and date/excerpt controls. Report whether the menu order feels natural.

## Cycle 45 — Locations rename the real folder

Scope: Andrew found that renaming a Location changed only its sidebar label while Finder retained the original disk name. Replace the alias-only menu action with a physical folder rename.

Changes: Locations now offer **Rename Folder…**, using the same validated filesystem operation as library folders. The dialog starts with the actual disk name. Successful rename clears that folder's legacy sidebar alias and updates location URLs, root, favorites, recents, navigation and saved-search roots. Old aliases are not automatically applied to disk; run Rename Folder to perform the intended rename. Remove from Locations remains shortcut-only.

Validation: ./bin/build and ./bin/test pass (74 tests, zero failures). Regression checks cover a legacy alias followed by physical rename, name collision rejection, preserved child file, persisted new root/location name, recents, saved-search root and copied path. Existing tests cover open-descendant refusal and unsaved-buffer preservation. The first test run exposed test settings leaking into a later search test; scoped restoration fixed test isolation.

Native Dev: normal quit/restart retained the three existing sample tabs. Opened cycle-45/sample/Before through Open by Path, right-clicked its Location, chose Rename Folder, and renamed to After. Sidebar and header changed; Show in Finder selected the actual cycle-45/sample/After folder. Filesystem check confirmed Before absent and Note.md unchanged. Sample screenshot and verification logs are in research/cycle-45/. Everyday app closed normally before packaging.

Known limits: documents inside a folder must be closed before rename; cross-process editing in another app is not coordinated. Unavailable volumes, permissions failures, case-only renames and VoiceOver were not exercised natively. Rename uses a dialog, not iA's inline text field.

Artifacts: dist/Omawrite Dev.app and dist/Omawrite.app. GitHub RC1 downloadable binary remains Cycle 42; source history records this fix.

Optional exercise: close documents inside the location, right-click it → Rename Folder, then Show in Finder. Confirm the new name and contents in both places. An old alias such as sample_b needs this real rename once.

## Cycle 46 — Non-overlapping Locations and child Favorites

Scope: implement Andrew's approved Locations/Favorites distinction and preserve the reasoning. Locations represent independent directory trees. Favorites provide shortcuts into those trees.

Changes: explicit Locations + rejects parent/child overlap in either direction and exact duplicates, identifies the conflicting Location, and offers Add to Favorites instead. Paths are canonicalized and compared on component boundaries. Favorite addition is idempotent. Navigation/open-by-path can browse overlapping folders without creating Locations. On startup, existing overlaps are normalized to outermost Locations and nested entries are retained as Favorites; no files are moved. See [decision record](location-design.md) for migration and evidence versus inference about iA.

Validation: ./bin/build and ./bin/test pass, 75 tests, zero failures. New coverage checks child-first/parent-first rejection, exact duplicates, similarly named independent folders, symbolic links, invalid URLs, Favorite idempotence, navigation/back, and persisted order-independent migration. Both app bundles refreshed and deep/strict signatures verified.

Native Dev: three existing sample tabs survived normal quit/restart. Existing nested Locations migrated under Favorites with repo retained as the outer Location. Locations + selecting cycle-46/sample/Parent/Child produced the expected overlap dialog identifying repo. Add to Favorites instead created Child; clicking it showed Note.md and left Locations unchanged. Dialog and resulting sidebar screenshots use synthetic writing. Everyday app reopened with its prior document and migrated organizer. Logs/screenshots in research/cycle-46/.

Known limits: parent-first and child-first backend paths are tested, but only the child-add case was checked through the native picker. Dark appearance, VoiceOver, unavailable-volume aliases and cross-process concurrent settings changes were not certified. This does not implement a global library index. Existing alias names on migrated child shortcuts become their actual folder names in Favorites.

Artifacts: dist/Omawrite Dev.app and dist/Omawrite.app. GitHub RC1 archive remains Cycle 42.

Optional exercise: use Locations + on a folder inside repo, choose Add to Favorites instead, then click its Favorite. Verify the folder opens without adding another Location and no files move.

## Cycle 47 — Unified preview/output templates

Scope: Andrew's View → Template screenshot. Unify selection across preview and output and add useful named presets with honest limits.

Changes: View → Template replaces Output Style and the separate Preview Typeface menu. Modern, Classic, Manuscript Mono, GitHub, Helvetica, Palatino, MLA Draft and Custom share persisted selection, font and C++ block/table formatting. Existing Sans/Serif/Mono quick commands now select the corresponding template. Preview footer names the active template; screen text size remains a reading adjustment. Markdown and editor typography are unchanged. PDF/print pagination now uses consistent point-based geometry and formatted-HTML normalization fixes Qt table-header alignment in preview and output. See [reasoning and preset details](template-design.md).

Validation: ./bin/build and ./bin/test pass, 76 tests, zero failures. New regression exercises all seven built-ins and return to Modern, live preview font/line-height, persisted selection, HTML output content, and unchanged source/modified state. Existing custom-style, page-break, rollback, recovery, links and Markdown tests pass. Tests now load bundled Mono fonts for representative export evidence. Existing Qt font alias/SplitView teardown warnings remain.

Visual QA: generated HTML/PDF specimens for all seven presets; rendered and inspected the seven single-page PDFs. This caught and fixed high-DPI geometry, decoration font scaling and first-header-cell alignment. Native Dev verified View → Template menu, GitHub selection, persisted GitHub after restart, corrected table rendering, MLA Draft, and return to Modern; source stayed clean. Both apps closed normally before refresh. Evidence under research/cycle-47/.

Known gaps: Duo/Quattro, full MLA compliance, iA template-package import, physical printing, dark/narrow/VoiceOver matrix, browser HTML rendering and long multi-page table stress remain unverified or unimplemented. Templates share typography/content styles, not identical line wrapping between differently sized screen/page surfaces. Continuous preview omits page decorations; use paginated preview.

Artifacts: dist/Omawrite Dev.app and dist/Omawrite.app. GitHub RC1 binary remains the older Cycle 42 download.

Optional exercise: open research/cycle-47/sample/Templates.md and use View → Template to compare Classic, GitHub and Palatino. Export or use Paginated Preview; report the preferred reading style and spacing.

## Cycle 48 — Selectable library date display

Planned scope: begin the screenshot-backed [menu closeout sequence](menu-closeout-plan.md) with View → View Options → Show Date. Make Date Modified, Date Created and None exclusive, expose the same choice through library menus, and migrate the old date visibility setting without changing sort order.

Changes: the library model now exposes separate created and modified date labels; an unavailable creation time is shown honestly. Native View, toolbar library options, sort popover and file context View Options share the one persisted date mode. The former `showDates` bool migrates to Modified or None on first use. None hides the row date; all three choices leave file sorting unchanged.

Validation: `./bin/build` and `./bin/test` pass, **78 tests, zero failures**. Added independent filesystem date and preference-migration cases; existing native command-state checks include the three choices. Dev bundle was prepared at the stable bundle ID and the ordinary app was packaged with a local ad-hoc signature. Logs and synthetic sample are in [research/cycle-48](../research/cycle-48/README.md).

Native Dev: opened `research/cycle-48/sample/First.md` in a new QA window, leaving the previously open documents untouched. View → View Options → Show Date presented all three entries. With the sample file's birth time at 2 January 2025 and modified time at 15 June 2026, the library row showed **2 Jan** and **15 Jun** respectively. None removed the date. The sort bar stayed **Sort by Date Modified** through the switches. The toolbar library menu exposed the same nested choices. Cropped sample-only screenshots show the two visible states.

Known gaps: native relaunch persistence, context-menu date choice, dark/narrow/VoiceOver and creation-time-unavailable volumes were not exercised in this live pass; persistence and invalid creation time are covered by tests. The creation-time sample needs `research/cycle-48/prepare-sample.sh` after a fresh checkout because Git does not retain birth/modified timestamps. Other View menu gaps remain on the audit.

Runnable artifacts: `dist/Omawrite Dev.app` and `dist/Omawrite.app` from Cycle 48. The GitHub RC1 binary remains the earlier Cycle 42 checkpoint.

Optional exercise: run `research/cycle-48/prepare-sample.sh`, open its `sample/First.md`, and switch View → View Options → Show Date among the three choices. The file list should show 2 Jan, 15 Jun, then no date, without changing the sort order.

## Cycle 49 — Format menu closeout

Planned scope: match the captured Format groups and order; put Body at top level and Page Break under Format; add Ordered Task List and a conservative Clear Styles action. Keep unobserved formatting semantics and Omawrite's Change Case extra distinct from claims of iA parity.

Changes: Format now follows the observed Headings, Lists, Blockquote/Body, Structure, inline formatting, code, additions, rule/page-break and Clear Styles order. Ordered tasks generate numbered checkboxes, normalize existing list prefixes, preserve completed task state and support completion toggling without changing the numeric marker. Clear Styles acts on an explicit selection: it removes supported outer inline emphasis/strike/highlight wrappers and simple whole-line heading/quote/list/task prefixes, or refuses ambiguous code, links and nested constructs without mutation. Each replacement uses one undoable edit.

Validation: `./bin/build` and `./bin/test` pass, **80 tests, zero failures**. The added cases cover list conversion, completion, indentation, fenced refusal, nested supported styles and one-step Undo/Redo. The ordinary app confirmed the native Format hierarchy, ordered-task creation, completion and Undo with a synthetic file. That pass found a Clear Styles refusal for a bold heading; the source fix and regression test passed, and the refreshed Dev app then changed `## **Styled sample**` to `Styled sample`. Undo restored the original Markdown, and the sample was saved back to its original bytes. Both local bundles were refreshed and their signatures verified.

Known gaps: Clear Styles deliberately supports a bounded subset; complex mixed or nested Markdown, links and code remain untouched. Deep nested list behavior and cross-app Markdown rendering need later acceptance. The UI inspection tool timed out after closing disposable QA windows in both app identities; its cause remains unverified and belongs in the Window-cycle checks. No user writing was edited or force-closed. The [sample-only screenshot and native log](../research/cycle-49/README.md) record the corrected result. GitHub RC1 remains the older Cycle 42 binary.

Runnable artifacts: `dist/Omawrite Dev.app` and `dist/Omawrite.app` from Cycle 49.

Optional exercise: open `research/cycle-49/sample/Format-actions.md`, choose Ordered Task List on its two task lines, complete one item and Undo, then select `## **Styled sample**`, choose Clear Styles and Undo. Confirm the menu placement and plain Markdown result.

## Cycle 50 — File and Edit menu hierarchy

Planned scope: align the observed File and Edit top-level hierarchy and group existing local commands without changing their safety semantics. Keep uncaptured iA submenu children unimplemented rather than inferred.

Changes: File now presents New-in-Library actions, Open/Recent, Close, Save, document operations, Versions, Finder/library reveal, Share, Export, Print and Page Setup in the captured groups. Omawrite-only file commands remain in an explicit Extras group. Edit now presents Undo/Redo, focused-field Cut/Copy/Paste/Delete/Select All, Copy Formatted/HTML/Markdown, Paste As, Find, Spelling and Grammar, Transformations, Speech and manual authorship actions. The macOS-injected Emoji & Symbols command remains at the end; Omawrite's duplicate entry was removed. Source-only copy formats require an editor selection; ordinary editing commands continue to follow the active editable field.

Validation: `./bin/build` and `./bin/test` pass, **81 tests, zero failures**. The new native-menu regression covers saved versus untitled guards, editor-focus copy-format enablement, Markdown/HTML/formatted clipboard results without source mutation, Find-field disabling of source-only copies, and a dirty File → Close → Cancel path that retains the draft. The full suite retains document, recovery and file-operation coverage.

Native Dev: the File/Edit hierarchy was inspected with `research/cycle-50/sample/Menu-actions.md`. Copy formats were enabled only for the selected editor text and did not change the sample. A disposable edit followed by File → Close → Cancel retained the draft; it was discarded through the normal unsaved-work prompt before quitting. No print, export or share job was sent. The QA app was closed normally.

Known gaps: the iA screenshots still do not expose the children of Revert, Share, Print, Paste As, Paste Edits From, Mark As, Find, Spelling, Substitutions, Transformations, Speech or AutoFill, so their exact iA behavior remains unverified. AppKit injects AutoFill, Dictation and Emoji & Symbols; their availability depends on macOS. Cross-application clipboard, actual sharing/printing, and full keyboard, dark, narrow and VoiceOver matrices remain open.

Runnable artifacts: `dist/Omawrite Dev.app` and `dist/Omawrite.app` from Cycle 50. The GitHub RC1 binary remains the earlier Cycle 42 checkpoint.

Optional exercise: open `research/cycle-50/sample/Menu-actions.md`, select its bold phrase and try Copy Formatted, Copy HTML and Copy Markdown. Make a disposable edit, choose File → Close, then Cancel. Confirm the text remains and report any confusing order, name or disabled state.

**Cycle 50b follow-up — Transformations:** The 24 September iA capture and a synthetic iA text sample establish the four Edit → Transformations labels and order. Omawrite now presents Make Upper Case, Make Lower Case, Capitalize and Make Title Case in that order. Capitalize lowercases a selected plain-text range then uppercases each Unicode word initial; it uses the existing protected-selection refusal and one-step Undo path. A selected `tEST of THE wORLD` becomes `Test Of The World`, matching the observed iA sample. At the Cycle 50b checkpoint, Omawrite's Make Title Case used the same algorithm, while the iA sample yielded `tEST of the wORLD`; that distinct title-style behavior remains a gap. [Reference](../research/cycle-50/ia-capitalize-reference.txt).

Cycle 50b `./bin/build` and the full native-access `./bin/test` pass with **102 tests, zero failures and zero skips**. The new test covers captured menu order/labels, selection-preserving Unicode transformation, Undo and protected-link refusal. The refreshed Dev app reproduced the plain-text result and Undo in a disposable draft, then discarded that draft and returned to clean Second.md. The ordinary app was repackaged and reopened its saved document. Both bundles passed strict signature verification. [Native verification](../research/cycle-50/native-capitalize.txt). Known gaps include iA's distinct Make Title Case rules, punctuation/locale edge cases, native protected-context and accessibility checks. Runnable artifacts: current `dist/Omawrite Dev.app` and `dist/Omawrite.app`; neither bundle is in Git. Optional exercise: in a disposable draft, select `tEST of THE wORLD`, choose Capitalize, Undo, then compare Make Title Case and report expected treatment of small words.


**Cycle 50c follow-up — distinct Make Title Case:** A second isolated iA sample established `the QUICK BROWN fox and a DOG in new YORK` → `The QUICK BROWN Fox and a DOG in New YORK`; the earlier sample produced `tEST of the wORLD`. Omawrite now lowercases interior English minor words, capitalizes all-lowercase major words and preserves mixed/all-uppercase major words. This is an observed subset, not a general title-style or locale claim. Existing protected-selection refusal, selection preservation and atomic Undo remain.

`./bin/build` and the full native-access `./bin/test` pass **102/0/0**. The test covers both iA samples, Undo, Unicode Capitalize and protected-link refusal. Refreshed Dev reproduced the second Make Title Case sample and Undo in a disposable draft, then discarded it through File → Close and returned to clean Second.md. The ordinary app was repackaged and reopened clean README.md; both bundles passed strict signature verification. [iA reference](../research/cycle-50/ia-title-case-reference.txt), [native check](../research/cycle-50/native-title-case.txt), [build log](../research/cycle-50/build-50c.log), [test log](../research/cycle-50/test-50c.log). Unobserved punctuation, locale and style rules plus keyboard/dark/narrow/VoiceOver matrix remain gaps. Runnable artifacts: current local ordinary and Dev bundles. Optional exercise: select `the QUICK BROWN fox and a DOG in new YORK` in a disposable draft, choose Make Title Case, Undo, and report any capitalization you would prefer.

## Cycle 51 — Tree/List navigation and Preview menu

Planned scope: add the captured View Options → Navigation → Tree/List choice and View → Preview hierarchy, using the existing continuous and paginated renderers. Keep iA's uncaptured PDF submenu children and distinct Web semantics as explicit gaps.

Changes: the file library now persists Tree or current-folder List mode. List displays direct children; clicking a folder navigates into it with Back/Forward/Enclosing Folder history. Show in Library moves List to a nested document's containing folder. Switching modes at the same root retains the in-memory Tree expansion. Native View, library sort/options and file context menus share the mode and checked state. Preview Full/Split/Web uses the existing layout and continuous preview; PDF → Paginated Preview opens the existing page/fit controls. Editor Only remains a separate Omawrite action.

Validation: `./bin/build` and `./bin/test` pass, **82 tests, zero failures**. The added regression covers direct-child List rows, Tree expansion, folder history, nested Show in Library/reveal, mode persistence and native menu state. The ordinary and Dev bundles were refreshed and deep/strict signatures verified.

Native Dev: the synthetic three-note folder showed Child and Root-note in List; Tree expansion exposed Nested-note; clicking Child in List showed Nested-note and Grandchild, and Go → Back in Library returned to the root. Opening Deep-note by path revealed it in the List at Grandchild. The native View menu exposed Navigation → Tree/List and Preview → Full/Split/Web/PDF → Paginated Preview. Paginated Preview opened with Fit Page selected; Fit Width changed the checked fit control. No print job was sent. [Evidence](../research/cycle-51/README.md).

Known gaps: Tree expansion resets when changing the library root and is not restored across launches. List mode is a current-folder browser, not an all-files flattened list; this retains predictable folder actions and bounded scans. Web uses the existing continuous renderer, and iA's PDF submenu children remain uncaptured. Native dark/narrow, VoiceOver, keyboard-only traversal, unavailable folders and full menu-state relaunch were not exercised in this pass; mode persistence and unavailable-folder safety have automated coverage. No private writing was used for QA.

Runnable artifacts: `dist/Omawrite Dev.app` and `dist/Omawrite.app` from Cycle 51. GitHub RC1 binary remains the earlier Cycle 42 checkpoint.

Optional exercise: open `research/cycle-51/sample` by path, use View → View Options → Navigation to switch Tree/List, open Child and use Go → Back in Library. Open `Child/Grandchild/Deep-note.md`, then use File → Show in Library. Compare View → Preview → Full/Split and PDF → Paginated Preview; report any confusing behavior.

## Cycle 52 — View chrome and statistics (in progress)

Planned scope: reproduce the captured Title Bar and Toolbar menu groups, add Default/Stats Only with independently selectable metrics, and define counts explicitly without claiming linguistic or authorship provenance accuracy. Keep macOS traffic lights and native-menu access available when QML chrome fades or hides.

Source changes: the statistics dialog and compact footer now include sentences, speaking time, task count and fully labelled manual Human/AI/Reference source-word counts. Stats Only preserves ten independent metric choices as shown in the iA screenshot. Title Bar offers Fade In/Out and Always Show; Toolbar offers Fade In/Out, Always Show and Hide. The top strip stays 44 px for native controls and dragging. [Counting and fade rules](../research/cycle-52/counting-rules.md) describe Omawrite's chosen semantics.

Validation so far: `./bin/build` and `./bin/test` passed at this checkpoint with **84 tests, zero failures**; the combined Cycle 54a source later passed 89. Focused tests cover counting, UTF-16 manual labels, live updates/Undo, independent menu checks, persistence and keyboard-focus reveal. The stable Dev bundle was refreshed on 24 September after a normal Quit. Its [native menu check](../research/cycle-52/native-menu.txt) found the View hierarchy and the fixture counts in the statistics dialog; selecting a Stats Only metric changed the footer, and Default restored it.

Known gaps: the Stats Only display is in Omawrite's footer rather than iA's top toolbar, and long selections elide visually while retaining a full hover/accessibility value. Sentence/task rules are conservative heuristics; authorship labels are manual assertions. iA fade timing was not captured. Live counts after editing/annotation, fullscreen, narrow/dark, high-contrast, VoiceOver, restart and traffic-light visual acceptance remain pending native QA. Screenshot capture was unavailable.

Optional exercise once Dev is refreshed: open `research/cycle-52/sample/Statistics.md`, compare View → Toolbar → Stats Only metric checks with the footer, edit a sentence and a task, then try Title Bar and Toolbar Fade/Hide. Confirm window controls and native View menu remain reachable.

## Cycle 53 — Go menu and organizer (source in progress)

Planned scope: expose saved locations, queries, recent files and current-library hashtags through the captured Go hierarchy. Keep unavailable roots and recent documents visibly unavailable, and refresh bounded local tag information without presenting stale counts as current.

Source checkpoint 53a: Go now offers Back/Forward, library navigation, Enclosing Folder, Open Link, Quick Search, Command Palette, dynamic Locations, Smart Folders and Hashtags. A saved query restores its root, text and content-search choice; New Smart Folder opens a blank query and persists only when saved. Hashtag selection opens a current-root search. Missing roots are refused without redirecting the active library. Recent files whose paths disappeared are disabled. Smart Folders → Recents is currently a nested recent-file submenu; the screenshot shows a direct Recents action, so exact structure remains partial.

Source checkpoint 53b: a complete small-root hashtag scan attempts to watch at most 256 file and directory paths, debounces changes by 400 ms and rescans. Partial scans, failed watcher registration and unavailable folders explicitly require manual refresh. Changing the library root cancels the previous generation so its tags cannot be published in the new root.

Validation so far: `./bin/build` passes. The full `./bin/test` suite passed at this checkpoint with **88 tests, zero failures and zero skips** using native macOS access; the combined Cycle 54a source later passed 89. A restricted sandbox run had nine older spelling/recovery/process/watcher failures and is not the acceptance result. New coverage exercises query persistence, unavailable roots, hashtag routes, watcher fallback and recents. The refreshed Dev app exposed the [Go menu](../research/cycle-53/native-menu.txt), and a temporary one-file root automatically added #beta after an external saved-file edit. No private writing was used.

Known gaps: a full incremental content index and iA smart-folder semantics are outside this bounded pass. For a root over the watch cap, or when macOS rejects a watch, external edits can make counts stale until Refresh Hashtags; the menu states that limitation. Saved-query roundtrip, unavailable-root UI, atomic replacement, large-root fallback, keyboard and narrow/dark layout remain unverified. Screenshot capture was unavailable.

Runnable artifact: `dist/Omawrite.app` was packaged from Cycle 53 source and passed strict local-signature verification. `dist/Omawrite Dev.app` is now refreshed through Cycle 54a source; the Cycle 53 native checks above are partial. See the [Cycle 53 record](../research/cycle-53/README.md) and [usability checklist](../research/usability/cycle-53.md). Optional exercise: open the sample root, use Go → Quick Search and New Smart Folder, save a query, then edit a sample tag externally and check its status and refresh behavior.

## Cycle 54 — writing input (phases 54a–54c partial)

Planned scope: reproduce captured View → Show Completions and inspect Edit → Substitutions/AutoFill children before assigning automatic writing behavior. Preserve source, Undo, IME composition and recovery. The 24 September iA native menu capture now supplies child labels but not substitution triggers or Undo rules.

Source checkpoint 54a: Show Completions is a manual action with current-document Unicode candidates in a small popup. Acceptance revalidates source/caret and replaces one prefix atomically; there is no automatic insertion. Code and URL contexts are excluded. Candidate discovery is bounded to the first 50,000 UTF-16 code units, at most 256 matching occurrences and 12 results.

Source checkpoint 54b: Edit → Substitutions → Smart Quotes is a persisted, opt-in toggle. A directly typed straight double quote becomes an opening or closing curly double quote using the immediately preceding character; existing text is never scanned or rewritten. The literal keystroke and automatic replacement use separate document edits, so the first Undo restores the straight quote and the second removes it. The handler refuses selected text and IME composition, and does not run for paste. Fenced, indented and inline code, bare URLs, Markdown link destinations and incomplete raw tags retain straight input. Text Replacement, capitalization and automatic correction remain absent.

Source checkpoint 54c: Edit → Substitutions → Smart Dashes is a separate persisted opt-in toggle. Based on an isolated iA sample, only the directly typed second hyphen in a spaced prose pair such as `one -- two` becomes an em dash. The first Undo restores the literal pair and the second removes the second hyphen. It shares Smart Quotes' refusal of selections, IME composition, paste, modified shortcut input, fenced/indented/inline code, bare URLs, Markdown link destinations and incomplete raw tags. Line-leading rules/front matter, adjacent third hyphens and unobserved unspaced forms remain literal. Existing text is never scanned. No other dash rules are inferred.

Validation: Cycle 54c `./bin/build` and the full native-access `./bin/test` suite pass with **101 tests, zero failures and zero skips**. Focused Smart Dashes coverage checks persisted menu state, disabled behavior, the two-stage Undo sequence and code/URL/link/raw-tag refusals. In refreshed Dev, the menu exposed both independent toggles: default-off `--` stayed literal, enabled spaced prose became an em dash, one Command-Z restored the pair, and disabling the option left later input literal. The ordinary bundle reopened a clean saved document; both bundles passed strict signature verification. The Dev UI bridge timed out on the disposable draft's Discard action, so the final Dev close state is unverified. The preceding Cycle 54b native pass verified Smart Quotes typing/Undo; Cycle 54a displayed two local suggestions for `lan`, accepted one and restored the prefix with Undo. See [native evidence](../research/cycle-54/native-smart-dashes.txt), [iA observation](../research/cycle-54/native-smart-dashes-reference.txt) and [cycle gaps](../research/cycle-54/README.md).

Known gaps: candidates beyond the bounded window are omitted. Smart Quotes and the observed `--` Smart Dashes rule are implemented as bounded opt-in subsets. Text Replacement, Smart Copy/Paste, automatic correction/capitalization and AutoFill remain unimplemented; additional unobserved dash rules are deliberately absent. IME, paste, selection, non-US layouts, popup visuals, arrow keys, Escape, narrow/dark and accessibility remain pending native checks. [Optional sample exercise](../research/usability/cycle-54.md): request a completion for `lan`, accept and Undo; then try Smart Quotes and Smart Dashes in prose and protected Markdown contexts.

## Cycle 55 — Focus and review (phases 55a–55c partial)

Planned scope: align captured Focus menu hierarchy without changing the existing focus/highlighter behavior, then separately add live lexical/style review overlays with source and performance safeguards.

Source checkpoint 55a: Focus → Enable Focus Mode now contains Sentence, Paragraph and Typewriter in captured order. Sentence and Paragraph remain exclusive; Typewriter scrolling stays independent. Writing Review… follows as an Omawrite-specific action. No master toggle or Markdown/highlighter mutation was added.

Source checkpoint 55b: Focus → Enable Style Check → Custom is a persisted opt-in command. It highlights matches from the existing Writing Review custom-word list and refreshes about 200 ms after text or setting changes; turning it off clears the overlay immediately. The offset-preserving prose mask excludes fenced, indented and inline code plus URL syntax. Work is bounded to the first 50,000 UTF-16 units, 32 unique comma-separated terms of at most 64 units and 1,000 ordered, non-overlapping matches. Markdown styling and focus formatting are retained, while search highlighting keeps priority. Highlighting does not edit source, move the caret or add Undo history.

Source checkpoint 55c: Focus → Enable Style Check → Fillers is a second persisted toggle beside Custom. Its explicit starter set is `very`, `really`, `quite` and `just`, matched case-insensitively at Unicode whole-word boundaries. Custom and Fillers merge into one globally ordered, non-overlapping result stream under the shared 50,000-unit/1,000-match caps; an exact overlap is labelled Custom. Each category can be disabled immediately without clearing the other. The same prose mask excludes fenced, indented and inline code, URL destinations and raw tags. No Clichés, Redundancies or Show Syntax entries were added.

Validation: `./bin/build` and the full native-access `./bin/test` suite pass with **96 tests, zero failures and zero skips** at the Cycle 55 checkpoint. Focused tests cover merged UTF-16 offsets/order/caps, overlap precedence, independent toggle persistence/clearing, exclusions, composition with Markdown/focus/search formats and unchanged source/caret/Undo/modified/annotations. The refreshed stable Dev app exposed both Fillers and Custom. Toggling Fillers on the synthetic `research/cycle-47/sample/Templates.md` document left source and saved status unchanged. A later live CUA view of `research/cycle-55/sample/Fillers.md` visibly showed yellow highlights on prose `really`, `very`, `QUITE`, `just` and link-label `quite`, while `veryish`, the URL destination, inline/fenced/indented code and raw tag attributes had no yellow. Status remained `Opened Fillers.md`. No screenshot file could be persisted. [Fillers native record](../research/cycle-55/native-fillers.txt), [Custom native record](../research/cycle-55/native-custom.txt) and [Cycle 55 record](../research/cycle-55/README.md).

Known gaps: Clichés, Redundancies and parts-of-speech Show Syntax categories are unimplemented. The live Fillers color/exclusion matrix was visually checked, but no screenshot artifact was saved. Native checked marks, Custom color/overlap appearance, dimming/scroll feel, keyboard traversal, narrow/dark/fullscreen and VoiceOver remain unverified. Runnable artifact: refreshed `dist/Omawrite Dev.app` with current combined source; ordinary `dist/Omawrite.app` still contains Cycle 53 source. [Optional usability exercise](../research/usability/cycle-55.md).

## Cycle 56 — Authors setup (phase 56a partial)

Planned scope: implement only the captured pre-setup Authors → Set Up Authorship… flow. A required Name and optional Identifier form a local profile. Post-setup iA menus and assignment behavior were not observed, so this phase does not infer them.

Changes: macOS now places Authors between Format and View. Its setup sheet trims and persists a nonempty Name of at most 100 UTF-16 units and an optional Identifier of at most 200, rejecting control characters. Save remains disabled until the profile is valid; Cancel retains the previous profile. Reopening edits the saved profile as an explicit Omawrite choice. The profile never edits Markdown, adds Undo/recovery state, writes an authorship sidecar or labels text. Edit → Authorship Annotations remains the separate manual-label workflow.

Validation: `./bin/build` and the full native-access `./bin/test` suite pass with **94 tests, zero failures and zero skips**. Automated coverage verifies invalid and valid forms, trimming, Save, Cancel, persistence across reopen, and unchanged source, status, modified/Undo state, authorship ranges, sidecar and recovery contents. The refreshed stable Dev app exposed Authors → Set Up Authorship… and its blank sheet. Save was disabled until a synthetic Name was entered; Cancel returned to the unchanged sample source and status. Native Save/reopen was not exercised to avoid leaving a fake profile. Screenshot capture was unavailable. See the [native record](../research/cycle-56/native-menu.txt) and [Cycle 56 record](../research/cycle-56/README.md).

Known gaps: post-setup iA behavior is unknown. There is no author registry, automatic authorship assignment, verified provenance, Mark As or Paste Edits From parity. Native profile persistence, validation errors, keyboard traversal, dark/narrow layout and VoiceOver remain unverified. Runnable artifact: refreshed `dist/Omawrite Dev.app` with combined Cycle 55c/56a source; ordinary `dist/Omawrite.app` remains Cycle 53 source. [Optional usability exercise](../research/usability/cycle-56.md).

## Cycle 57 — bundled Help and Window Center (phases 57a–57b partial)

Planned scope: add useful local Help and What’s New pages without inventing an Online Support destination or duplicating macOS Help search behavior. Keep the viewer isolated from document editing and recovery.

Changes: Help now offers Omawrite Help and What’s New in Omawrite before the existing Keyboard Shortcuts entry. Both pages are bundled Markdown resources and open in a read-only, scrollable in-app view that closes with its button or Escape. A fixed backend allowlist exposes only the two resource IDs; unknown or missing pages return a safe local error. The content describes current local-file workflows, safety boundaries and recent features without private paths or network access.

Validation: `./bin/build` and the full native-access `./bin/test` suite pass with **97 tests, zero failures and zero skips**. Automated coverage verifies both resources, traversal refusal, safe missing-page text, menu routing, read-only rendering, Escape, retained Keyboard Shortcuts and unchanged dirty-draft source/caret/Undo/modified/status/recovery. The refreshed stable Dev app exposed the Help menu; macOS accessibility verified both bundled pages, Close and Escape. The synthetic `research/cycle-47/sample/Templates.md` source and status remained unchanged. Screenshot capture was unavailable. See the [native record](../research/cycle-57/native-help.txt) and [Cycle 57 record](../research/cycle-57/README.md).

Phase 57b changes: Window → Center now appears after Zoom, is enabled for a normal window and invokes AppKit’s native `NSWindow center`. It does not add Zoom All, Fill, tiling, display movement or a dynamic document list. The automated two-window geometry check centers only the target near its current screen’s available center, retains its size and the other window’s geometry, and preserves dirty source, modified status and Undo state.

Combined validation: `./bin/build` and the full native-access `./bin/test` suite pass with **99 tests, zero failures and zero skips**. The stable Dev app was refreshed with combined 57b/58a source. Window → Center was visible, enabled and invoked on synthetic Target.md with unchanged clean status. An app-only live capture showed the source and preview, but its crop did not establish the window’s on-screen center and no screenshot was saved. See the [Center native record](../research/cycle-57/native-center.txt).

Known gaps: Help screenshot/pixel styling, dark and narrow layouts, link/scroll behavior, VoiceOver and OS Help search were not checked. Center’s exact OS geometry and multi-display behavior remain unverified; geometry is automated offscreen only. Online Support remains absent because no destination was verified. Zoom All, Fill and remaining Window/application-menu AppKit closeout remain open. Runnable artifact: refreshed `dist/Omawrite Dev.app` with combined 57b/58a source; ordinary `dist/Omawrite.app` remains Cycle 53 source. [Optional usability exercise](../research/usability/cycle-57.md).

## Cycle 58 — local fragment navigation (phase 58a partial)

Planned scope: make explicit local Markdown/text heading fragments useful without broadening the parser or output stack. Keep fragment navigation behind the existing guarded open path and preserve plain Markdown, Undo and dirty-document safety.

Changes: inline links to `.md`, `.markdown`, `.mdown`, `.txt` and `.text`, matching wikilinks and same-file `#heading` links now resolve heading fragments from the source editor, Go → Open Link and rendered preview. Heading slugs follow the existing preview/table-of-contents rule, including duplicate suffixes such as `#same-1`. The fragment is removed before file/history/library identity is recorded. A successful cross-file open moves the source caret and queues the preview jump until the target Markdown has parsed; same-file navigation does not reload the document. Dirty Cancel and failed opens retain the original document, caret and scroll.

Validation: Cycle 58a initially passed 98 tests; the later combined 57b/58a `./bin/build` and full native-access `./bin/test` pass with **99 tests, zero failures and zero skips**. Automated coverage uses the synthetic [`Source.md`](../research/cycle-58/sample/Source.md) and [`Target.md`](../research/cycle-58/sample/Target.md) shape to verify inline and `[[wiki#fragment]]` links, duplicate `#same-1`, Go and preview routes, fragment-free identity, same-file navigation, unchanged source/Undo, dirty Cancel and failed-open caret/scroll retention. The refreshed stable Dev app opened Source.md by path; after the caret was placed in the inline link, Go → Open Link opened Target.md with clean status and no `#` in the title. A later live screenshot displayed the second `Same` heading with the caret at its start. No screenshot file was persisted, and precise preview scrolling was not confirmed visually. See the [native record](../research/cycle-58/native-fragments.txt) and [Cycle 58 record](../research/cycle-58/README.md).

Known gaps: a target already owned by another window is focused through the existing window routing, but the fragment jump is not transferred to that window. A valid document with a missing heading opens without a jump or dedicated missing-anchor message. Native visual scroll position, screenshots, dark/narrow layout, keyboard-only traversal and VoiceOver remain unchecked. This phase is local navigation support, not complete Markdown parser or output parity. Runnable artifact: refreshed `dist/Omawrite Dev.app` through 58a; ordinary `dist/Omawrite.app` remains Cycle 53 source. [Optional usability exercise](../research/usability/cycle-58.md).

## Cycle 59 — integrated acceptance checkpoint (implemented subset)

Scope: consolidate the current menu map, automated gate, bundle state and native evidence without treating matching labels or selected samples as full iA Writer parity. The tested product code is commit `5f8d64b` on macOS 27.0.

Build and artifacts: final `./bin/build` and the full native-access `./bin/test` suite pass with **99 tests, zero failures and zero skips**. `./bin/package-mac` succeeded. Both `dist/Omawrite.app` and `dist/Omawrite Dev.app` were refreshed, locally ad-hoc signed and strict signature-verified. The ordinary running app was normally quit after saving its open README.md status; the refreshed ordinary bundle reopened saved README.md with Authors present. Dev remained clean on synthetic Fillers.md. [Build log](../research/cycle-59/build.log), [test log](../research/cycle-59/test.log) and [native record](../research/cycle-59/native-qa.txt).

Integrated evidence: Cycles 48–58 inspected all ten top-level menu families through accumulated native passes. Latest live checks include visible Fillers matches/exclusions with unchanged status, both bundled Help pages, the fragment target caret at the second `# Same`, and Window → Center invocation with unchanged target status. A disposable Safety.md pass exercised File → Close on a dirty draft: Cancel retained the starred title, edit and Unsaved status; a later Discard closed only that disposable draft and returned to saved Second.md. The final disk SHA-256 matched its initial value. No user writing was edited or discarded. No screenshot file was persisted. This evidence is representative; it does not cover every saved, untitled and dirty state or every enabled-state transition.

Classification and gaps: **implemented-subset integrated checkpoint, not full closeout**. Cycle 54 substitutions remain absent. Cycle 55 lacks Clichés, Redundancies and Show Syntax. Cycle 56 lacks observed post-setup authorship semantics and iA-style Mark As/Paste Edits. Cycle 57 retains incomplete application/Window OS actions and unverified real/multi-display Center geometry. Cycle 58 retains parser/output/cross-app gaps plus cross-window and missing-anchor fragment behavior. VoiceOver, dark/narrow, multi-display and a complete ten-menu state matrix are unverified. Both current apps are local ad-hoc artifacts; the GitHub RC1 remains the older Cycle 42 release. [Cycle 59 record](../research/cycle-59/README.md) and [usability checklist](../research/usability/cycle-59.md).

## Cycle 60 — Fomawrite identity and repository rename

Planned scope: rename the visible product and ordinary/QA app identities, executables, build/package scripts, Linux package metadata and repository while preserving plain Markdown, recovery, authorship sidecars, upstream attribution and the historical record.

Changes: `Fomawrite.app` and `Fomawrite Dev.app` use new stable bundle IDs and Fomawrite menu/window/help labels. The qmake target, test binary, package names and current build/development docs follow the new name. First launch copies old Qt preferences and matching workspace/recovery files into the new namespace without removing the old state; a marker prevents repeat recovery import. Existing `.omawrite-authors.json` sidecars and clipboard MIME remain readable because they are established data formats. The GitHub repository is renamed to `andrewjngray/fomawrite_mac`; upstream history remains unchanged.

Tests: `./bin/build` passed; full native-access `./bin/test` passed **102/0/0**. Both bundles were packaged/prepared and passed strict signature verification. Native QA launched both newly named apps with saved sample documents, saw Fomawrite menus, and confirmed the previous library location, favorites and sort setting migrated after a first-launch migration correction. [Logs and native record](../research/cycle-60/README.md).

Known gaps: Linux packaging was edited but not run on Linux. macOS may request fresh per-app permissions because bundle IDs changed. The local checkout directory remains `omawrite_mac`; old release archives/tags retain their historical names. Developer ID signing, notarization, marketing/trademark clearance and a newly branded public download are separate distribution work.

Runnable artifacts: local `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`, both generated and excluded from Git. Optional usability exercise: open Fomawrite Dev with a disposable Markdown file, confirm your library shortcuts/settings, make an unsaved edit and test Quit → Cancel before saving or discarding it.

## Cycle 61 — local checkout path rename

Planned scope: rename the physical local Git checkout from `omawrite_mac` to `fomawrite_mac` without losing uncommitted work, breaking Fomawrite saved paths, or changing product behavior.

Changes: moved the existing checkout with its Git history and uncommitted sample files intact. Rebased only the exact old checkout prefix in the ordinary app's favorites, recents, saved-search roots, location labels, last-save directory and saved workspace snapshot; the Dev app had no old-path preferences. Generated qmake files were regenerated by the normal build and test scripts. No product source code changed. The old path has no compatibility symlink because the Codex executor rejects symlinked writable roots.

Validation: `./bin/build` succeeded at the new path and `./bin/test` passed **102 tests, zero failures and zero skips**. Both local app bundles passed strict signature verification. `dist/Fomawrite Dev.app` launched from the renamed checkout; the installed ordinary app reopened its clean saved document and showed a `fomawrite_mac` favorite. Preferences and workspace snapshots were checked for the exact old prefix after migration. See [verification record](../research/cycle-61/verification.txt). No screenshot was committed because the live sidebar contains private shortcuts.

Known gap: the Codex desktop app's saved project registration still points to the old folder and this task's filesystem permission remains anchored there. Re-add or relink the project at `fomawrite_mac` before starting the next Codex task. The physical folder rename itself is local and does not appear in Git; this record is the portable history. Other external applications with independently saved absolute paths were not exhaustively audited. Runnable artifacts: local `dist/Fomawrite.app` and `dist/Fomawrite Dev.app` under the renamed checkout. Optional usability exercise: open the new checkout favorite, a saved search and a recent sample Markdown file, and report any missing path.

## Cycle 62 — canonical Markdown and visual-edit mapping foundation

Planned scope: establish a conservative source-to-visual mapping before making the rendered pane editable. Preserve plain UTF-8 Markdown exactly across view changes, and identify syntax that must stay source-only.

Changes: added `SourceVisualMapping`, a UTF-16 offset projection for supported paragraph, ATX-heading, list-item, emphasis/strong and inline-link-label text. It retains the original source byte-for-byte and returns a bounded single-span replacement only for mapped text. Code, tables, footnotes, images, comments, raw HTML, escaped constructs and reference links are source-only. The architecture and fixture corpus are recorded in [research/cycle-62/README.md](../research/cycle-62/README.md).

Tests: `./bin/build` passed. Both focused mapping tests passed (4 total Qt test functions including setup/cleanup). The combined `./bin/test` run reported 103 passed and one failure in the existing native Window Center geometry check; a second agent run instead hit an existing recovery-file numbering check. The mapping tests passed in both runs. This cycle has no QML/native feature to verify, so no Dev app was replaced or inspected.

Known gaps: no editable preview yet, no multi-line/IME/paste or cross-marker transformations. The full-suite environment-sensitive failures need isolation before an integrated release claim. Runnable artifact: `build/Fomawrite.app` from the combined source checkout. Optional exercise for Andrew: none yet; Cycle 64 will provide the first visual-edit surface for review.

## Cycles 63–69 — visual writing and export closeout

The following records describe the combined source state on 25 September 2026. The integrated automated suite passes **111 tests, zero failures and zero skips**. Final `./bin/build` and `./bin/test` passed; the stable `dist/Fomawrite Dev.app` was refreshed and inspected with a disposable sample. Native acceptance remains partial as specified under Cycle 69.

### Cycle 63 — compact writing controls

The top-right chrome now has accessible Bold, Italic, Link and paragraph-format controls, with a single Format control at narrow widths. Link opens a target/title popover and uses the established Markdown command; bold and italic reuse existing wrapping. `ChromeButton` labels inherit italic styling. Native keyboard-only, VoiceOver, dark-mode, narrow-layout and link-title checks remain pending. [Exercise](../research/usability/cycle-63.md).

### Cycle 64 — visual editing baseline

Visual Edit is a separate plain-text projection over canonical Markdown. One bounded inline change is applied through `Backend::applyVisualEdit`; stale, syntax-bearing, multiline and source-only edits are rejected. It uses document Undo, preserves bounded selection, waits for IME composition, routes Control-Z/Control-Y to source Undo/Redo and offers a Source route. Destructive native Edit actions are disabled while it is focused. This is not general WYSIWYG Markdown: tables, code, footnotes, images, comments, raw HTML, escaped constructs, reference links and structural/multiline changes are source-only. [Exercise](../research/usability/cycle-64.md).

### Cycle 65 — conservative visual styling

`VisualTextHighlighter` applies presentation-only heading, strong, emphasis and link-label styling from mapping-owned spans, and differentiates source-only blocks. The editable subset remains ordinary paragraphs, headings, list-item content and supported inline text; no Markdown is inferred from formatting. Simple quote bodies and task-item bodies are editable while their Markdown markers remain protected. Nested structures, image insertion, multiline paste/drag/drop and visual-surface formatting actions remain unimplemented; source-formatting commands explicitly route visual focus back to Source. [Exercise](../research/usability/cycle-65.md).

### Cycle 66 — unified export hub

The upper-right Export control and File → Export HTML/PDF open `ExportHub`, which selects PDF/HTML, style, paper, orientation and destination. It exposes Share Markdown, a clearly labelled continuous preview and separate native paginated preview. Paper/orientation persist through the existing page-layout model. The direct File → Share Markdown native action remains available. Continuous preview is not page-exact; native output comparisons, cancellation/failure, dark/narrow layout and accessibility checks remain pending. [Exercise](../research/usability/cycle-66.md).

### Cycle 67 — Fomawrite-owned output style gallery

The Export hub now has a responsive gallery for built-in Fomawrite styles and local user styles. A user can duplicate the current style and edit its name, font family, point size, header, footer and PDF header/footer and title-page settings, or delete it. The versioned app-data JSON catalog is capped at 64 styles/64 KiB, uses UUIDs and atomic save. Invalid/malformed/oversized catalogs are left untouched; built-in selection clears stale custom selection. No Ulysses presets/assets, thumbnails, margins/spacing/hierarchy controls or import are included. A disposable native style was duplicated, selected and deleted; restart persistence and HTML/PDF appearance comparisons remain pending. [Exercise](../research/usability/cycle-67.md).

### Cycle 68 — scoped output CSS

HTML export accepts a user-selected local CSS file. It must be a regular, non-symlinked UTF-8 file no larger than 64 KiB; external assets, `url()`, imports, font-face declarations, namespaces, embedded markup, URLs and control characters are refused. Valid CSS is embedded only into exported HTML and the local selection persists. PDF and Markdown remain unaffected. DOCX/ePub and Ulysses style import are absent; a future importer requires demonstrated mapping and rights review. [Exercise](../research/usability/cycle-68.md).

### Cycle 69 — integrated acceptance (partial checkpoint)

Scope and changes: integrate source mapping and guarded visual editing, compact writing controls, export hub, local style gallery and output-only CSS. The native pass found and fixed visual typing Undo grouping, accidental source formatting from visual focus, style-copy selection, and gallery button clipping in its 245 px panel. Simple quote/task bodies were added to the conservative editable subset. The iA Writer acceptance ledger remains open.

Tests and native verification: final `./bin/build` and `./bin/test` pass with **111/111 tests**. The refreshed stable `dist/Fomawrite Dev.app` opened a saved synthetic sample. Native QA verified source-preserving bold replacement, a single Command-Z for contiguous visual typing, quote-body edit/Undo, source-format command refusal while visual focus was active, built-in style switching, user-style duplicate/select/delete, PDF options, and the corrected gallery button layout. The ordinary installed app was left running and untouched. No screenshot was committed because the sidebar displayed private location shortcuts. [Sanitized native record](../research/cycle-69/README.md).

Known gaps: Visual Edit remains a bounded inline subset, not a general WYSIWYG editor. Multiline/complex nested Markdown, images, tables, code and several insertion commands stay source-only. The continuous export preview is not page-exact. Saved HTML/PDF visual comparisons, custom CSS selection in the final native bundle, style restart persistence, dirty/recovered/multiwindow/external-edit cases, long files, dark/narrow/fullscreen layout, keyboard-only navigation and VoiceOver remain for a later acceptance pass. DOCX/ePub and Ulysses style import are not built. These gaps prevent claiming the full Cycle 69 acceptance gate or iA Writer parity.

Runnable artifacts: `dist/Fomawrite Dev.app` is refreshed with the clean synthetic sample open for review. `dist/Fomawrite.app` was packaged from the same source; both generated bundles passed strict local signature verification. `/Applications/Fomawrite.app` was not replaced. [Exercise](../research/usability/cycle-69.md).

## Cycle 70 — selected macOS app icon

Planned scope: use Andrew's selected third concept, with Markdown source on the left, formatted text on the right and a blue divider, as Fomawrite's app icon. Keep the mark original and readable at Dock sizes, without changing editing behavior or the installed ordinary app.

Changes: added a source SVG and a reproducible QtSvg/iconutil generator for a multi-resolution `Fomawrite.icns`. qmake now copies the icon into the app bundle and the macOS Info.plist names it, so the ordinary and stable Dev bundles use the same product mark. The icon source is vector and the generated resource is committed for reproducible packaging.

Validation: `./bin/build` passed and `./bin/test` passed **111 tests, zero failures and zero skips**. The generated Info.plist was regenerated after qmake had retained its prior copy; both final bundles contain `CFBundleIconFile=Fomawrite.icns` and identical icon resources. Both passed strict local signature verification. The 1024 px and 64 px renders were visually inspected; refreshed Dev launched successfully. Finder/Dock appearance remains Andrew’s review item. See [Cycle 70 evidence](../research/cycle-70/README.md).

Known gaps: the light icon has not been checked in every macOS Dock appearance or at every scaled size. It is a local design asset, not a notarized public release. The installed `/Applications/Fomawrite.app` remains separate from generated `dist/` bundles.

Runnable artifacts: generated `dist/Fomawrite.app` and `dist/Fomawrite Dev.app` after packaging. [Optional usability exercise](../research/usability/cycle-70.md): compare the icon in Finder and the Dock at small and large Dock settings, then report whether the source/preview meaning remains obvious.

## Cycle 71 — simpler writing icon

Planned scope: follow Andrew's updated selection of the middle icon from the original three-concept board: two dark-grey writing lines and a blue insertion caret, with no grey connecting stroke. Retain the existing bundle identity and icon build workflow.

Changes: replaced the Cycle 70 split-view artwork in the editable SVG and regenerated the multi-resolution `.icns`. Both ordinary and Dev bundles continue to use the same icon metadata and product name. The previous artwork remains available in Git history.

Validation: the 1024 px and 64 px renders were visually inspected; `./bin/build` passed and `./bin/test` passed **111 tests, zero failures and zero skips**. Both generated bundles contain `CFBundleIconFile=Fomawrite.icns` and an identical icon resource, and both passed strict local signature checks. The refreshed Dev app launched to a clean Untitled document. Finder/Dock appearance at different scales remains Andrew’s review item. See [Cycle 71 evidence](../research/cycle-71/README.md).

Known gaps: Dock/Finder appearance across all scales, dark backgrounds and macOS icon caching still warrants Andrew's review. This is a local ad-hoc build, not a public release. `/Applications/Fomawrite.app` remains untouched.

Runnable artifacts: `dist/Fomawrite.app` and `dist/Fomawrite Dev.app` after refresh. [Optional exercise](../research/usability/cycle-71.md): compare the new Dock icon against the previous Cycle 70 design and judge legibility at your normal Dock size.

## Cycle 72 — final short-connector app icon

Planned scope: finish Andrew's side-by-side icon comparison by adopting the short grey connector beside the two dark writing lines and blue caret. Keep product identity and editing behavior unchanged.

Changes: selected the short-connector vector from the three-option comparison and regenerated the committed multi-resolution `Fomawrite.icns`. The no-stroke Cycle 71 and extended-stem alternatives remain documented by the comparison; the short connector is the final product mark.

Validation: the 1024 px and 64 px renders were inspected. `./bin/build` passed and `./bin/test` passed **111 tests, zero failures and zero skips**. `./bin/package-mac` and `./bin/prepare-dev-app` succeeded. Both generated bundles declare `CFBundleIconFile=Fomawrite.icns`, carry the same icon resource as the source `.icns`, and pass strict local signature verification. The refreshed Dev app launched to a clean Untitled window. See [Cycle 72 evidence](../research/cycle-72/README.md).

Known gaps: Dock/Finder rendering can be cached by macOS and has not been inspected across every scale and appearance. These are local ad-hoc bundles, not a notarized public release. The installed `/Applications/Fomawrite.app` was not replaced.

Runnable artifacts: `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`. [Optional exercise](../research/usability/cycle-72.md): glance at the Dev Dock icon at your usual Dock size and check that the short connector reads clearly.

## Cycle 73 — restore the running macOS Dock icon

Planned scope: address Andrew's screenshots showing a generic Dock placeholder after the final icon was installed. Keep the approved short-connector artwork and the existing product/bundle identities.

Changes: the macOS runtime no longer overrides its icon with an empty `QIcon::fromTheme("fomawrite")`. The icon generator now writes a matching PNG from the approved SVG, the Qt resource bundle embeds it, and the application and every QML window set it explicitly. Linux retains the themed icon path.

Tests and native verification: `./bin/build` passed and `./bin/test` passed **111 tests, zero failures and zero skips**. The old theme lookup was confirmed null; the compiled Qt resource rendered a non-null 64 px icon. `./bin/package-mac` and `./bin/prepare-dev-app` passed, with strict signature verification. Both refreshed apps launched to clean Untitled windows. The old installed app was closed normally, and `/Applications/Fomawrite.app` was updated and verified to match the generated executable and `.icns`. See [Cycle 73 evidence](../research/cycle-73/README.md).

Known gaps: the Dock itself was not captured through the UI tool. A pinned tile may retain macOS icon-cache state until refreshed; Andrew should check both ordinary and Dev tiles. This remains a local ad-hoc build, not a notarized public release.

Runnable artifacts: `/Applications/Fomawrite.app`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`. [Optional exercise](../research/usability/cycle-73.md): glance at both Dock tiles and confirm the writing icon replaces the generic grid.

## Cycle 74 — rounded macOS app tile

Planned scope: address Andrew's Dock screenshots showing the short-connector mark on a small white card inside an oversized white square. Make the icon read as a distinct rounded app tile comparable in scale to Outlook, preserving the selected writing mark.

Changes: the SVG now has a transparent canvas with one large cool-gray rounded tile, subtle border and shadow, and larger writing lines and blue caret. Regenerated the PNG and multiresolution `.icns` from the same vector source.

Tests and native verification: `./bin/build` passed and `./bin/test` passed **111 tests, zero failures and zero skips**. `./bin/package-mac` and `./bin/prepare-dev-app` succeeded. The old ordinary and Dev windows were clean and closed normally before replacement. Both refreshed apps launched to clean Untitled windows; the installed executable and icon match `dist/Fomawrite.app`, and strict signature verification passed. macOS's `NSWorkspace` initially returned its cached old icon; after Launch Services registration and a Dock restart it returned the new rounded tile. See [Cycle 74 evidence](../research/cycle-74/README.md).

Known gaps: the Dock itself was not captured through the UI tool, so Andrew should judge the result at his own Dock size and desktop appearance. This remains a local ad-hoc build, not a notarized release.

Runnable artifacts: `/Applications/Fomawrite.app`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`. [Optional exercise](../research/usability/cycle-74.md): compare both Dock tiles with Outlook against light and dark backgrounds.

## Cycle 75 — white-face rounded app icon

Planned scope: correct the Cycle 74 interpretation of Andrew's Outlook comparison: keep the rounded gray border but return the icon face to white.

Changes: replaced the gray-filled tile with a white face and narrow cool-gray rounded rim; retained the short-connector writing mark and blue caret. Regenerated the PNG and multiresolution `.icns` from the vector source.

Tests and native verification: `./bin/build` passed; `./bin/test` passed **111 tests, zero failures and zero skips**. `./bin/package-mac` and `./bin/prepare-dev-app` succeeded. With the app closed, the signed build was installed in `/Applications`; the executable and `.icns` match the packaged app. It launched, and `NSWorkspace` resolved the installed icon to the white-face design after macOS icon registration and a Dock restart. See [Cycle 75 evidence](../research/cycle-75/README.md).

Known gaps: Andrew's visual check in his own Dock remains useful; the Dock itself was not captured. This is a local ad-hoc build, not a notarized release.

Runnable artifacts: `/Applications/Fomawrite.app`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`. [Optional exercise](../research/usability/cycle-75.md): compare the white face and gray rim beside Outlook at your usual Dock size.

## Cycle 76 — pure-white app icon face

Planned scope: match Andrew's OneDrive reference more closely by removing the visible gray border from the white Fomawrite tile while retaining subtle three-dimensional depth.

Changes: replaced the bordered tile with a single pure-white rounded face over a soft offset shadow. The short connector, dark writing lines and blue caret remain unchanged. Regenerated the PNG and multiresolution `.icns` from the vector source.

Tests and native verification: `./bin/build` passed; `./bin/test` passed **111 tests, zero failures and zero skips**. `./bin/package-mac` and `./bin/prepare-dev-app` succeeded. Andrew saved and quit the installed app before replacement. The installed executable and `.icns` match the signed packaged app; it launched, and `NSWorkspace` resolved the borderless white icon after Launch Services registration and a Dock restart. See [Cycle 76 evidence](../research/cycle-76/README.md).

Known gaps: Andrew's visual check in his own Dock remains useful; the Dock itself was not captured. This is a local ad-hoc build, not a notarized release.

Runnable artifacts: `/Applications/Fomawrite.app`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`. [Optional exercise](../research/usability/cycle-76.md): compare the white tile beside OneDrive at your usual Dock size.

## Cycle 77 — cmux-inspired off-white icon

Planned scope: soften the pure-white Cycle 76 tile to match the slightly off-white face in Andrew's cmux Dock screenshot while keeping the borderless shape and gentle dimensional shadow.

Changes: sampled the cmux screenshot's near-white top and light-gray bottom, then applied a subtle `#fefefe` to `#eeeeee` vertical gradient to the Fomawrite face. The writing mark, blue caret and shadow remain unchanged. Regenerated the PNG and multiresolution `.icns`.

Tests: `./bin/build` passed; `./bin/test` passed **111 tests, zero failures and zero skips**. `./bin/package-mac` and `./bin/prepare-dev-app` succeeded. See [Cycle 77 evidence](../research/cycle-77/README.md).

Native verification and known gaps: the installed app was left untouched while it was running, then replaced after it exited. The installed signed executable and `.icns` match the packaged bundle. The app launched, and `NSWorkspace` resolved the off-white icon after Launch Services registration and a Dock restart. Andrew's visual judgment in his own Dock remains useful; the Dock itself was not captured. This is a local ad-hoc build, not a notarized release.

Runnable artifacts: `/Applications/Fomawrite.app`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`. [Optional exercise](../research/usability/cycle-77.md): compare Fomawrite beside cmux at your usual Dock size.

## Cycle 78 — running Dock icon lifecycle

Planned scope: keep the approved light icon when Fomawrite is closed and show Andrew's first, cmux-matched dark concept while the macOS process is running; return to light only after a completed quit.

Changes: added a separate running-icon SVG/PNG resource generated by `./bin/make-app-icon`. macOS windows use the dark icon and AppKit temporarily overrides the Dock tile at launch. The existing guarded quit path remains intact; `aboutToQuit` clears the override and restores the light icon. Finder and the static bundle `.icns` remain light, and Linux's icon path is unchanged.

Tests and native verification: `./bin/build` passed and `./bin/test` passed **111 tests, zero failures and zero skips**. `./bin/package-mac` and `./bin/prepare-dev-app` succeeded; both signed bundles retain the unchanged light `.icns`. The running SVG matched the first approved dark concept at 128 px, and the refreshed Dev app launched. After both old processes exited, `/Applications/Fomawrite.app` was replaced with the signed packaged app; its executable and light `.icns` match the package, its Finder-facing icon resolved to light, and it launched. See [Cycle 78 evidence](../research/cycle-78/README.md).

Known gaps: the running dark Dock tile and return to light after Quit still need a direct visual check. The desktop capture showed a remote Windows taskbar rather than the macOS Dock. This remains a local ad-hoc build, not a notarized release.

Runnable artifacts: `/Applications/Fomawrite.app`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`. [Optional exercise](../research/usability/cycle-78.md): inspect the Dock tile through launch, canceled Quit, and completed Quit.

## Cycle 79 — export and Visual Edit presentation

Planned scope: respond to Andrew's export-dialog and split-view screenshots with a calmer dialog hierarchy, stronger labels and buttons, and more readable Visual Edit text that better resembles the rendered document without changing canonical Markdown.

Changes: the export controls align to the top of the modal, with a concise subtitle, clearer section labels and a shorter Save action. The style gallery uses flat bordered selection cards rather than the native gray pill treatment; secondary actions are quieter. Visual Edit has an 18 px minimum body size independent of compact output styles, and supported bullet/numbered list lines display protected markers. Its bounded inline editing and source-only protections remain unchanged.

Tests and native verification: `./bin/build` passed and `./bin/test` passed **112 tests, zero failures and zero skips**, including an export-hub open/readability regression and protected list-marker mapping checks. Both ordinary and Dev bundles were refreshed; the Dev bundle passed strict local signature verification and launched with a disposable Markdown sample. The running `/Applications/Fomawrite.app` was not replaced or inspected. No private-screen capture was taken. See [Cycle 79 record](../research/cycle-79/README.md).

Known gaps: Andrew's visual judgment of the revised dialog and split view is still needed. Visual Edit remains limited to supported inline source spans; source-only blocks do not become WYSIWYG, and visual layout is an approximation of formatted output rather than page-exact. The full Cycle 69 acceptance matrix and notarized release remain open.

Runnable artifacts: `dist/Fomawrite Dev.app` for immediate review and `dist/Fomawrite.app` as the packaged ordinary build. The older `/Applications/Fomawrite.app` remains running. [Optional exercise](../research/usability/cycle-79.md): compare the Dev export dialog and Visual Edit with the screenshots.

## Cycle 80 — Source button leaves Visual Edit

Planned scope: fix Andrew's report that clicking Source in Visual Edit produced no visible change in Split view.

Changes: Source now turns off Visual Edit, selects the existing editor-only workspace mode, and focuses the Markdown source editor. It keeps the current document and cursor; the visual projection is not serialized or saved by switching modes.

Tests and native verification: `./bin/build` passed and `./bin/test` passed **113 tests, zero failures and zero skips**. A focused regression invokes the document pane's Source button from Visual Edit and confirms editor-only mode, disabled visual mode and unchanged Markdown. `./bin/package-mac` and `./bin/prepare-dev-app` succeeded; both bundles passed strict local signature verification. Andrew authorized closing Dev; it had already exited when checked, so the refreshed Dev bundle was installed and launched on a disposable sample. The running `/Applications/Fomawrite.app` was not touched. See [Cycle 80 record](../research/cycle-80/README.md).

Known gaps: the refreshed button still needs Andrew's direct click check in his window. Visual Edit remains the conservative inline subset recorded in Cycle 69; source-only Markdown constructs are unchanged. This is an ad-hoc local build, not a notarized public release.

Runnable artifacts: `dist/Fomawrite Dev.app` for review and `dist/Fomawrite.app` as the packaged ordinary build. [Optional exercise](../research/usability/cycle-80.md): click Source from Visual Edit, then return to Split and Visual Edit.

## Cycle 81 — export gallery width and dialog fit

Planned scope: fix Andrew's screenshots showing the export style cards and heading clipped horizontally, with the scrollbar covering the card edges. Keep the dialog usable as its host window becomes narrower.

Changes: the style gallery now reserves a scrollbar gutter, uses a slim thumb, fixes its content to one column, and stacks the style-copy field and button so their combined implicit width cannot create horizontal scrolling. The export dialog fits within the host window, switches to a compact layout before the preview gets cramped, and scrolls its options independently; the wide gallery grows to 280 px. This dialog adapts with the parent window but is not a separate resizable window.

Tests and native verification: `./bin/build` passed and `./bin/test` passed **113 tests, zero failures and zero skips**. The export UI regression verifies a wide gallery without horizontal overflow and checks that the compact dialog remains inside a 720 × 520 window. `./bin/package-mac` and `./bin/prepare-dev-app` succeeded. The previously running Dev app exited normally before replacement, and the refreshed Dev bundle launched on a disposable sample. The running `/Applications/Fomawrite.app` was not replaced. See [Cycle 81 record](../research/cycle-81/README.md).

Known gaps: Andrew's direct visual check of the middle column at his display scale is still needed. The separate-window resizing idea has not been built; export preview remains continuous rather than page-exact. Full Cycle 69 acceptance and public signing remain open.

Runnable artifacts: `dist/Fomawrite Dev.app` for review and `dist/Fomawrite.app` as the packaged ordinary build. [Optional exercise](../research/usability/cycle-81.md): open Export at normal and minimum app sizes and inspect the style cards and controls.

## Cycle 82 — export scrollbar and button clarity

Planned scope: address Andrew's screenshots of a scrollbar mark beside “Built-in styles” and secondary export controls that lacked an obvious button state.

Changes: both export scrollbars are placed at the right edge of their own clipped panes. Style cards and secondary actions now have a visible resting fill and border, a stronger accent outline on hover, and a clear selection outline. Split and Full in the export preview now switch between a read-only source/rendered split and the full rendered view.

Tests: `./bin/build` passed; `./bin/test` passed **113 tests, zero failures and zero skips**. The export UI check verifies that Split reveals source and Full hides it, alongside the existing wide/narrow layout assertions. `./bin/package-mac` and `./bin/prepare-dev-app` produced locally signed bundles; strict signature verification passed. See [Cycle 82 record](../research/cycle-82/README.md).

Native verification and known gaps: the old processes exited, allowing the Dev bundle and `/Applications/Fomawrite.app` to be refreshed. The installed executable matches the signed ordinary package, and Dev launched on a disposable sample. Andrew's direct visual review of scrollbar placement and control contrast is still open. The public signing and broader Cycle 69 acceptance work remain open.

Runnable artifacts: `/Applications/Fomawrite.app`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`. [Optional exercise](../research/usability/cycle-82.md): inspect export scrollbars and button states at normal and narrow widths, including dark mode.

## Cycle 83 — resizable export layout

Planned scope: respond to Andrew's Cycle 82 screenshot by redesigning the whole Export dialog, not just individual buttons. Define reusable guidance for large task dialogs.

Changes: added the [dialog style guide](dialog-style-guide.md). Export now has a lower-right resize grip, draggable three-band splitters with minimum widths, a compact single-scroll layout, reserved scrollbar gutters, preview actions grouped with their content, and a fixed footer with equally sized Cancel and Save actions. The dialog stays bounded by its host window.

Tests: `./bin/build` passed; `./bin/test` passed **113 tests, zero failures and zero skips**. The export UI test drags the outer grip and first divider, checks band minimums and equal footer sizes, and verifies compact and preview-mode behavior. Synthetic wide/compact light and wide dark renders were visually inspected. `./bin/package-mac` and `./bin/prepare-dev-app` passed with strict signature verification. See [Cycle 83 record](../research/cycle-83/README.md).

Native verification and known gaps: the refreshed Dev app launched on a disposable sample; Andrew's direct review at his display scale remains open. The installed Applications copy stays at Cycle 82 until review. The dialog is resizable within the app window, not a separate native window. Exact PDF page breaks remain in paginated preview rather than the continuous live view; broader Cycle 69 acceptance and public signing remain open.

Runnable artifacts: `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`. [Optional exercise](../research/usability/cycle-83.md): drag the dialog edge and dividers, then inspect wide, compact and dark layouts.

## Cycle 84 — horizontal access in narrow Export

Planned scope: answer Andrew's follow-up that the compact Export layout hid the right bands. Let the dialog become thinner while retaining access to all three bands.

Changes: Export keeps the output, styles and live preview bands side by side at their useful minimum widths. Below the combined width, a persistent horizontal scrollbar moves between them; each band keeps its vertical scrolling. The title and action footer stay fixed. The resize grip allows 380 px, and the [dialog style guide](dialog-style-guide.md) now documents this behavior.

Tests and visual verification: `./bin/build` and `./bin/test` passed **113 tests, zero failures and zero skips**. The Export regression drags the horizontal scrollbar, checks that styles and preview remain present at 400 px, and scrolls the preview into view. Synthetic wide and narrow renders were inspected, including both ends of the scrollbar. `./bin/package-mac` produced a locally signed ordinary bundle. See [Cycle 84 evidence](../research/cycle-84/README.md).

Native verification and known gaps: after the prior Dev process exited normally, the stable Dev bundle was refreshed, passed strict signature verification and launched on a disposable Markdown sample. The stopped Applications copy was replaced with the signed package; its executable matches the package and strict signature verification passed. Andrew's direct display-scale review remains open. The dialog is still bounded by the host window; exact PDF page breaks are available through Paginated preview, not the live view. Public signing and broader Cycle 69 acceptance remain open.

Runnable artifacts: `/Applications/Fomawrite.app`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app` are current. [Optional exercise](../research/usability/cycle-84.md): shrink Export, scroll across all three bands, then expand it again.

## Cycle 85 — Studio tones and writing appearances

Planned scope: move the Ulysses-inspired work to the everyday writing surface while retaining canonical plain Markdown. Give the three columns distinct calm tones and offer reversible typography choices.

Changes: added an optional Studio light theme with a gray organizer, near-white file list and warm off-white editor. Writing Appearance in View and the Aa menu now offers Manuscript (existing mono default), Editorial (larger system sans, wider measure and more top space) and Book (serif). The choices persist separately from theme and export style. The source editor retains its existing document-level 140% line spacing, caret, selection and Markdown highlighter; no source conversion occurs on appearance changes.

Tests and evidence: `./bin/build` and the full `./bin/test` suite passed (116 tests, zero failures). The appearance regression switches all three choices without changing text, cursor, Undo state, workspace layout or output style, then saves and reopens exact UTF-8 source. Theme tests check Studio persistence and pane-tone separation. Synthetic wide/narrow Studio, Book and Dark captures are in [Cycle 85 evidence](../research/cycle-85/README.md). Andrew's visual review at his display scale remains open.

Known gaps: Studio is optional and does not replace existing users' theme. The current appearance presets choose font, size, measure and top space; paragraph spacing beyond the existing 140% line height is not yet independently adjustable. This is an original Fomawrite palette, not Ulysses assets or themes.

## Cycle 86 — writing chrome and selected states

Planned scope: make the document more prominent without removing writing controls that already work in narrow layouts.

Changes: Editorial and Book show a compact word-count chip at the top of the editor. The existing upper-right Bold/Italic/Link/paragraph cluster remains the primary wide-window control group; the lower toolbar remains available when that cluster collapses. Studio gives the selected file row a distinct resting tone while retaining a separate hover tone. The chip opens Document Statistics, so it is an actual control.

Validation and gaps: the full QML suite covers the appearance-dependent chip and existing toolbar commands. Synthetic wide and narrow images check that the chip clears the text column and the upper cluster collapses. VoiceOver and long-filename review on Andrew's display remain open. See [Cycle 85 images](../research/cycle-85/README.md) and the [Cycle 86 note](../research/cycle-86/README.md).

## Cycle 87 — safe paragraph breaks in Visual Edit

Planned scope: allow a useful multi-line edit without converting the whole rendered document back to Markdown.

Changes: Return in Visual Edit inserts a Markdown paragraph break in a mapped plain paragraph; Shift-Return inserts a single line break. The source mapping accepts the edit only when reprojecting the candidate source produces exactly the expected visual text. CRLF input, headings, list markers, styled spans whose syntax would change, source-only blocks and mixed-text multiline paste remain protected. Unsupported Return shows a route back to Source. Existing list/quote/task body edits remain available; creating new list items in Visual Edit is still source-only.

Validation and gaps: regressions exercise the mapping, actual Return key, protected list case, one-step Undo and exact source preservation. Visual Edit is still a bounded editor, not a general WYSIWYG surface: image cards, link-target editing, tables and arbitrary nested Markdown require further source mapping and native QA. See [Cycle 87 note](../research/cycle-87/README.md).

Packaging and native status: `./bin/package-mac` produced a locally signed bundle and strict signature verification passed. `/Applications/Fomawrite.app` was refreshed from that bundle with a backup of the previous copy; the installed and packaged executables have the same SHA-256 hash. The stable `dist/Fomawrite Dev.app` remains on its prior build while the open Dev process holds it; its replacement awaits a normal close so unsaved work is preserved. The editor changes are pushed to `origin/master`.

Runnable artifacts: `/Applications/Fomawrite.app` and `dist/Fomawrite.app` contain this cycle. [Optional exercise](../research/usability/cycle-85.md): choose Studio and Editorial from Aa, compare the wider writing page with Manuscript, then try Return in a plain Visual Edit paragraph and in a list. Andrew's display-scale and VoiceOver acceptance, long-document scrolling and general rendered-content editing remain open for Cycle 88 and later work.

## Cycle 89 — right-click file actions and explicit new views

Scope: audit the library context menu after Andrew reported no-op New Tab/New Window actions. Cycle 88's editor acceptance remains open.

Changes: explicit New Tab/New Window creates an additional editor for a saved file, while ordinary open continues to focus existing documents. Native tabs attach only the newly created view to the requesting window. Added file validation and visible errors, fixed Favorite alias recognition, and made Finder/Share failures reportable. New views require saving the target's draft first. Independent views protect saves against a changed disk baseline and permit Reload; Rename/Trash requires closing other views. Clipboard/output operations select the active view's draft deterministically.

Verification: `./bin/build` and all **118 tests pass**. The QML menu regression covers target-specific actions, dialogs, file mutations, Trash, clipboard formats, exports and presentation controls while retaining an unrelated unsaved draft. `./bin/test-window-routing` verifies the actual production window manager with native Cocoa windows and disposable data: separate windows, correct tab group, ordinary-open reuse, all three print-dialog cancellations, and Finder/share request dispatch. No print job or sharing transmission occurred. See the [action-by-action audit](../research/cycle-89/README.md).

Artifacts and limits: the updated package is `dist/Fomawrite.app`; running Applications/Dev bundles await normal close before refresh. Third-party share completion, Finder selection, export destination-picker interaction and VoiceOver remain manual checks. Independent views do not live-sync text; conflicts require Reload, an explicit decision to keep a version, or Save As. [Review exercise](../research/usability/cycle-89.md).


## Cycle 90 — workspace UI design planning

Status: planning only, 3 October 2026. Andrew requested a comprehensive Ulysses-inspired design pass before implementation. Read the [workspace UI redesign plan](workspace-ui-redesign-plan.md) for the proposed Cycles 90–97, standards, review gates and definition of done.

Completed planning: reviewed the supplied three-app composition and current code; obtained independent structure and visual-design reviews; specified pane-owned headers, blue-folder standards, round/capsule controls, writing appearances and desired-versus-effective layout state. The separate interactive concept explores presentation and panel visibility; it is not a runnable app build or proof of native behavior.

Verification: checked the proposal against existing command paths, workspace persistence and source/undo protections. No app source changed and no build, test, install or push was performed for this planning pass. Cycle 89 remains the code baseline, and Cycle 88 acceptance remains open.

Next review: approve the complete composition and compact behavior, then explicitly authorize implementation. The first implementation work is component and native-titlebar feasibility before changing the installed app.

## Cycles 90–96 — approved workspace implementation

Status: Andrew approved the complete workspace direction after the Cycle 90 planning pass. The work is integrated as one candidate; the cycle labels describe implementation scope and do not imply seven separate accepted releases. The original planning-only record above remains historical.

| Cycle | Changes |
| --- | --- |
| 90 — Design foundation | Approved complete composition, blue folders, pane ownership and responsive collapse rules. |
| 91 — Shared components | Added round toolbar actions and capsule groups, semantic surface/control palette roles and consistent navigation/control states. |
| 92 — Column-owned shell | Replaced toolbar width arithmetic with a 52-pixel header aligned to actual organizer, files and document panes. Guarded document history, Find, formatting, Appearance, Export and Workspace routes remain reachable. |
| 93 — Responsive column state | Added independent desired/effective visibility and widths, ordered auxiliary contraction, split fallback, 48-pixel restoration hysteresis, temporary navigation drawers and versioned per-window checkpoint state. Divider resizing stores user changes rather than temporary contraction. |
| 94 — Writing finish | Refined Editorial/Manuscript/Book measure and insets, set source line height to 150%, kept explicit Source / Visual Edit / Preview choices and the document statistics control, and removed permanent duplicate formatting actions. Existing stored theme and writing choices remain; new installations default to Studio and Editorial. |
| 95 — Navigation finish | Refined organizer sections and contextual removal, retained blue folder identity, normalized compact rows and two-line file previews, and kept folder/file search and history scopes distinct. |
| 96 — Supporting UI sweep | Applied shared control treatment to menus, search actions, outline, preview and dialog buttons while preserving Export's independent vertical panes, horizontal access and fixed footer. |

Verification: production build and **122 regression tests pass** (zero failures/skips). Native production-main checks pass pane alignment/resizing, per-window checkpoint independence, New Window/Tab and source/undo preservation. App-only wide/compact/Book/dark captures were inspected. See the [Cycle 97 evidence record](../research/cycle-97/README.md) for actual results and limits.

Known gaps: Andrew's complete composition/writing acceptance, VoiceOver, display scaling, inactive-window states, full screen and the broader native tab/window restoration matrix remain to be reviewed. Visual Edit remains a bounded source projection; general image/list/table editing, parser/output parity and public distribution are separate work. Private writing and pre-existing dirty sample files are untouched by this cycle.

## Cycle 97 — workspace candidate and acceptance handoff

Candidate identity: application version `0.3.0-dev1`; macOS bundle version `0.3.0`, build `97`. This is an acceptance candidate, not a parity or public-release declaration.

Planned scope: complete the appropriate automated checks, native resize/navigation/editing verification, synthetic screenshots, bundle identity comparison and documentation for the approved Cycles 90–96. Checkpoints must preserve Markdown bytes, undo, dirty/recovery state, active writing surface and per-window layout intent. Package/Dev/Applications status must be recorded individually after preparation; a shared version label alone does not prove equal executables.

Tests and native verification: `./bin/build`, all 122 tests, native workspace integration, focused appearance and control checks passed within the recorded scope. Export fixture synchronization/window-size issues and a transient frontmost native draft mismatch are documented with their reruns. See [research/cycle-97](../research/cycle-97/README.md).

Runnable artifacts: refreshed `dist/Fomawrite Dev.app`, ordinary `dist/Fomawrite.app`, and `/Applications/Fomawrite.app`, all version 0.3.0/build 97. All pass strict signatures; packaged/installed executables match. Dev has the same source with local Qt linkage. Previous Applications copy backed up. [Optional review exercise](../research/usability/cycle-97.md).

Next recommended scopes: Cycle 98 user feedback and accessibility/display-scale/tab acceptance; Cycle 99 safe visual blocks and image/list/table projection; Cycle 100 bounded workflow/parser/export gaps from the acceptance ledger; optional Cycle 101 distribution after acceptance. Each remains a separately scoped increment.

## Cycle 98 — workspace keyboard and native acceptance

Planned scope: complete feasible workspace acceptance checks and fix concrete keyboard/focus regressions before expanding Visual Edit.

Changes: F6/Shift+F6 now traverse visible workspace regions without consuming writing Tab input. Compact navigation takes focus on Close and restores its initiating control on Escape. Automatic contraction rescues focus from a hidden pane. Toolbar focus rings belong to the active window, with neutral selection treatment when inactive. The shared workspace standards and in-app shortcut help document these rules.

Verification: the production-main native fixture compiles and passes **111 assertions across Cycles 98 and 100**, including QAccessible button/text interfaces, actual keyboard shortcuts, compact navigation, dark/inactive state, full-screen entry/exit, native tab detach/regroup, independent per-window pane checkpoints and exact draft/selection/Undo preservation. Five synthetic app-only captures were inspected. [Evidence](../research/cycle-98/README.md), [native log](../research/cycle-98/native-workspace.log), [optional exercise](../research/usability/cycle-98.md).

Known gaps: listening to physical VoiceOver, actual macOS display setting changes/multiple displays, arbitrary-workspace quit/relaunch and Andrew's normal writing review remain open. The fixture boundary now waits for document-load cursor reset before preparing its independent selected draft; production cursor behavior was not weakened.

Runnable artifact target: stable `dist/Fomawrite Dev.app`, ordinary/demo `dist/Fomawrite.app` and `/Applications/Fomawrite.app`, all intended for the combined `0.3.0-dev2` / `0.3.0 (100)` handoff. The aggregate gate passes below; Dev, package and Applications are synchronized and strict-signature verified as of 4 October 2026.

## Cycle 99 — bounded visual blocks, images, tables and lists

Planned scope: make supported Visual Edit structures more useful while preserving canonical Markdown and exact source edits.

Changes: project simple tables into readable rows and allow edits within mapped cells; allow safe inline-code body edits; show local image thumbnails and edit image alt text without rewriting its destination/title; continue a supported list at the end of an item, reset a continued task to unchecked and exit an empty item. Replacements respect grapheme boundaries and support an empty document. The visual caret scrolls into view during typing; unsupported operations retain a visible Source route.

Verification scope: mapping and integrated editor fixtures cover exact UTF-8 source, Unicode/graphemes, image metadata preservation, table/code boundaries, list Return/exit, canonical Undo/Redo, unsupported edits and caret behavior. **134 integrated tests and native Visual Edit checks pass**, recorded in the [Cycle 99 record](../research/cycle-99/README.md) and combined Cycle 100 handoff.

Known gaps: arbitrary multiline paste, splitting within a list item, complex/nested table/list grammar, spatial image placement and a full editable image/table grid remain Source work. This is a bounded projection, not general WYSIWYG or complete Ulysses/Typora parity. Runnable artifacts are the combined build-100 targets above. Optional exercise: edit a table cell, image alt text and task list in the Cycle 99 sample, Undo each action, and compare the exact Markdown.

## Cycle 100 — heading links across open windows and integrated handoff

Planned scope: close the specific Cycle 58 navigation gaps after the workspace acceptance pass, then identify and refresh the review artifacts.

Changes: preserve heading fragments through ordinary open routing and explicit new views; navigate to headings in another window's live Markdown without reloading either buffer; report missing/invalid headings without moving the caret; retain the unsaved-source Cancel guard. Queued preview completions now carry a revision so a stale parse cannot consume a newer pending heading or raise a false missing-heading message.

Verification: cross-window encoded Unicode headings, unsaved targets, missing feedback, source Cancel and exact Undo pass in the integrated native fixture. Focused navigation regressions also pass. See the [Cycle 100 record](../research/cycle-100/README.md).

**Final integrated gate: passed.** `./bin/build` and all **134 tests** pass, with zero failures/skips. Native Visual Edit/navigation checks pass **5/0/0** including setup/cleanup; the workspace fixture passes 111 assertions. Application `0.3.0-dev2`, macOS `0.3.0 (100)`, is packaged and installed in Applications; both pass strict local signatures and executable hashes match. On 4 October, after Andrew quit Dev, `./bin/prepare-dev-app` refreshed the stable Dev copy to build 100. Dev passes strict signature verification and matches the prepared replacement; all three review copies are current. [Bundle identities](../research/cycle-100/artifacts.json). No public release or notarization is implied.

Next: review the combined build during normal writing and complete physical accessibility/display checks. Product Cycle 102 should address the highest-value image/table/list ergonomics from that review, with exact-source safeguards. Cycle 101 remains an optional distribution decision. Broader parser/output/interoperability gaps remain in the acceptance ledger.

## Cycle 102 — pane restoration and direct template access

Planned scope: address Andrew’s 4 October review of vanished collapse controls, View menu wording/order, and the inert Preview template label.

Changes: retain independent reopen buttons in the surviving document header, including faded/hidden toolbar and compact Drawer layouts; lead View with Toggle Library / Toggle Organizer; isolate Template below writing controls; turn the Preview template label into a compact dropdown sharing the backend output style. No document rewriting.

Verification: final build and **137 regression tests pass**; native control tests pass **5/0/0**, and the native workspace fixture passes **111 assertions**. Dev, demo/package and Applications are synchronized to `0.3.0-dev3`, macOS `0.3.0 (102)`, with strict local signatures verified. See the [Cycle 102 record](../research/cycle-102/README.md). [Optional usability exercise](../research/usability/cycle-102.md). Physical VoiceOver/display checks remain open. Next product scope: review-led image/table/list ergonomics, Cycle 103; Cycle 101 remains optional distribution.


## Cycle 103 — implement the approved workspace reference

Scope: Andrew’s side-by-side review showed a material gap between the approved concept and build 102. Replace the remaining native chrome/navigation details and ensure the installed app actually opens in the approved writing composition.

Changes: single aligned pane rules with generous invisible resize targets; original tabbed blue folder and pane/compose glyphs; 32px circular tools and divided capsules; regular-weight UI labels; dated document cards drawn from bounded local-file summaries; quiet library caption and preserved folder/list actions; warm Manuscript source editing with controlled line length, word-count pill, quiet search and footer. A one-time, per-copy presentation migration applies Source/Manuscript and cards across restored windows while preserving documents, recovery, output templates and future user preferences. The explicit Aa → Reference writing layout action remains available.

Validation and native screenshot evidence are recorded in [Cycle 103](../research/cycle-103/README.md). Runnable artifacts: stable Dev, ordinary demo/package and Applications, version `0.3.0-dev4` / macOS `0.3.0 (103)`. Andrew’s visual sign-off and physical VoiceOver/display checks remain open. [Optional exercise](../research/usability/cycle-103.md). Next cycle should follow his visual review before broadening image/table/list work; optional distribution Cycle 101 remains separate.


## Cycle 104 — folder heading and quick text sizing

Planned scope: address Andrew’s follow-up on the central folder heading, thin formatting glyphs and missing direct zoom controls.

Changes: blue-folder tonal heading button retaining the native picker; bold B, italic I and typographic paragraph glyph; separate minus/plus capsule before Aa using existing saved text-size commands. Remove hidden size floors after the zoom offset in Preview/Visual Edit so the first minus click visibly works. Condense narrow headers without obscuring zoom or pane restoration. Document the shared toolbar behavior.

Verification: final build/package and **144 regression tests pass**. Three focused tests pass offscreen and native Cocoa (**5/0/0** each including setup/cleanup). Actual pointer clicks verify rendered line-height changes in Source/Preview/Visual Edit, first-minus behavior, bounds, exact Unicode Markdown, disk contents, dirty state, Undo/Redo and output-style preservation. Resize checks cover 720–1440px across layouts, plus collapsed navigation; the folder heading routes to the existing picker action. Native synthetic screenshots were inspected. [Evidence](../research/cycle-104/README.md).

Runnable artifacts: stable Dev, ordinary demo/package and Applications are refreshed to `0.3.0-dev5`, macOS `0.3.0 (104)`, with strict signatures and matching demo/Applications executables. No public release. [Optional exercise](../research/usability/cycle-104.md). Physical VoiceOver/display acceptance and Andrew’s visual review remain open. Next product scope follows that review; bounded image/table/list ergonomics remains the subsequent work, with optional distribution separate.


## Cycle 105 — cleaner toolbar glyphs and capsules

Planned scope: match Andrew’s supplied Ulysses toolbar treatment more closely in the actual Qt build while retaining Fomawrite’s folder indicator, quick zoom and existing actions.

Changes: 34px circle/capsule surfaces, 32px segments, 18px glyphs, darker theme-aware toolbar ink and softer edges; uninterrupted formatting/zoom groups and an inset history divider; simpler original compose, link, search, share, pilcrow and sidebar geometry. Keep blue folders, hover/press/focus/disabled states and all existing command routing. Capture resting controls by moving the test pointer off the toolbar.

Validation: **144 regression tests pass**, build passes, and six existing native Cocoa checks pass (**8/0/0** including setup/cleanup). Native wide/reference/narrow/dark screenshots received a separate visual review with no blockers. Existing tests cover command clicks, exact Markdown/Undo/export preservation, resize fit and pane restoration. [Evidence](../research/cycle-105/README.md). No tests added merely to mirror visual properties.

Runnable artifacts: stable Dev, demo/package and Applications, all `0.3.0-dev6`, macOS `0.3.0 (105)`, strict-signature verified; demo/Applications executables match. Physical VoiceOver/display review and Andrew’s visual acceptance remain open. [Optional exercise](../research/usability/cycle-105.md). Next product work follows that feedback; bounded image/table/list ergonomics and optional distribution remain separate scopes.


## Cycle 106 — consistent, functional workspace footers

Planned scope: fix Andrew's report that bottom controls differ from the header, fail on clicks and do not line up between columns.

Changes: shared 52px workspace footer and header-derived 13px rounded controls; one aligned rule across navigation/Source/Preview; removal of the orphan global status strip; grouped Split/Full; direct appearance menu; visible dropdown chevrons; focused Visual Edit entry; accessible layout selection; rounded Library filter. Status stays visible when its usual pane is collapsed. Immediate menu transitions prevent a closing fade from swallowing a rapid reopening click. Radio-style menu items retain selection on reselection. Shared standards are recorded in the workspace UI guide.

Verification: production/final package build passes; **147 regression tests pass**; five affected native Cocoa workflows pass (**7/0/0** including setup/cleanup). Actual clicks cover all footer mode/template/appearance actions, rapid repeats, selection state and exact draft/Undo preservation. Geometry and native screenshots cover short/narrow/light/dark layouts and 320px Preview. Native mouse-only QA is isolated from unsolicited OS typing; separate filter keyboard checks remain. [Evidence and limits](../research/cycle-106/README.md).

Runnable artifacts: stable Dev, ordinary demo/package and Applications are synchronized to `0.3.0-dev7`, macOS `0.3.0 (106)`, with strict signatures verified and matching demo/Applications executables. [Optional exercise](../research/usability/cycle-106.md). Physical VoiceOver/display-scale checks and Andrew's normal writing acceptance remain open. Next product work follows his footer review; image/table/list ergonomics and optional public distribution remain separate scopes.


## Cycle 107 — stable document view switching

Planned scope: deeply check Andrew's report that lower controls relocate through Source, Split, Full and Visual Edit, and fix inconsistent view and scrolling behavior.

Changes: a single document footer keeps all four view controls at the lower right; independent writing-appearance/template controls remain at the left or in a compact Aa menu. Split visibly disables below its minimum usable width. Source-only and Full use matching minimum widths. View changes preserve reading positions while layout and rendering settle, suppress hidden-caret/synchronized-scroll feedback, restore writing focus and retain F6 navigation. Preview no longer counts the hidden visual document's stale height. Button presses dismiss tooltip overlays. Shared standards now require stationary controls and repeat-cycle verification.

Verification and artifact status: see the [Cycle 107 evidence record](../research/cycle-107/README.md) for the final test gate, native screenshots, limits and exact bundle identities. Target version: `0.3.0-dev8`, macOS `0.3.0 (107)`, stable Dev, ordinary demo/package and Applications. [Optional review exercise](../research/usability/cycle-107.md).

Next: Andrew's review of the actual repeated-switch behavior. Physical VoiceOver/display-scale checks remain open. Image/table/list ergonomics and optional public distribution remain separate scopes.


## Cycle 108 — shipped-footer verification and responsive view consistency

Planned scope: investigate Andrew's repeated reports of moving Source controls; exercise the actual app, repeated mode changes and resize boundaries before refreshing all local copies.

Changes: keep all footer action controls together at fixed right-hand positions and use a mode-independent compact-style budget. Correct hidden Visual Edit indication, one-click Visual Edit from contracted Source, Visual Edit-off from contracted Full, and explicit Split at the resize restoration boundary. Classify writing focus from the actual editor item rather than stale focus bindings. Footer and menu checks track the effective layout, and repeated workspace-mode selection preserves checkmarks. About identifies the running application version/path.

Verification: production build/package pass; **152 regression tests pass**, including rapid mode clicks, long Unicode drafts, selection/Undo and scroll preservation, both focus-dependent contraction paths and repeated menu choices. Cycle107 fixtures now load qrc:/Main.qml. An explicit diagnostic in the real executable runs 124 native states and saves captures with isolated temporary settings/documents; its wrapper compares executable identity, embedded UI and version metadata with the checkout. See the [Cycle108 evidence record](../research/cycle-108/README.md) for each local bundle's results and actual screenshots.

Limits: external macOS Accessibility was unavailable, so native verification uses synthetic Qt events in the application's own isolated window, not physical OS input. Original screenshots match the old separate-footer presentation, but their originating process is unproven. Physical VoiceOver/display scaling and arbitrary restored workspaces remain acceptance checks. Public RC1 is unchanged.

Runnable artifacts: `0.3.0-dev9`, macOS `0.3.0 (108)` at stable Dev, ordinary/demo and Applications paths; final identities in the [artifact record](../research/cycle-108/artifacts.json). [Optional review exercise](../research/usability/cycle-108.md).


## Cycle 109 — safe list splitting in Visual Edit

Planned scope: improve everyday list writing with bounded mid-item Return, preserving Markdown and Undo.

Changes: supported single-line bullet, ordered and task items split at the caret using one validated insertion. Marker style, indentation, spacing and line endings survive; new tasks are unchecked and following numbered source is not rewritten. Unicode graphemes, inline-construct interiors and nested/continued structures remain protected. Redundant typography writes are skipped to preserve remaining Redo history.

Verification: exact-source mapping and bundled-QML keyboard tests cover supported/refused splits, stale projections, follow-on typing, Undo/Redo and saved-file preservation. All159 regressions and the focused native checks pass; refreshed-bundle verification is recorded in the [combined Cycle111 handoff](../research/cycle-111/README.md). Target: `0.3.0-dev10`, macOS `0.3.0 (111)`. [Scope and limits](../research/cycle-109/README.md), [optional exercise](../research/usability/cycle-109.md). Shift-Return/list restructuring and general WYSIWYG remain outside this scope.

## Cycle 110 — searchable document outline

Planned scope: make long-document heading navigation easier without altering source or selection while browsing.

Changes: heading search, result counts, explicit hierarchy and current Source section; Up/Down/Home/End navigation; exact-offset activation after the drawer closes; Escape/Close focus restoration; clear empty/no-match feedback. Narrow Full Visual jumps reveal Source through the shared view route. Search fields use the shared rounded 34px treatment.

Verification: bundled-QML tests cover Unicode/fenced/YAML headings, keyboard filtering, cancellation, selection, exact destination and Undo preservation. All outline regressions and final native scenarios pass; integrated checks are recorded in the [Cycle111 handoff](../research/cycle-111/README.md). [Detailed scope](../research/cycle-110/README.md), [optional exercise](../research/usability/cycle-110.md). No heading reordering or parser-parity claim is included.

## Cycle 111 — Find/Replace polish and integrated writing review

Planned scope: provide a clear, consistent Find/Replace surface and finish the authorized daily-writing sprint after Cycles109–110.

Changes: rounded search/replacement fields, stable grouped navigation, useful match counts, nearest-caret search, advance-after-replacement behavior, keyboard Return/Shift-Return/Escape, narrow/dark support, and cancellation of obsolete view restoration when Find opens. Source typing updates results without selecting another match; hiding Source closes Find.

Verification and artifacts: **159 regression tests pass**,11 focused native checks and4 final Find checks pass. Each Dev/demo/Applications bundle passes 124 footer states plus seven writing workflows, strict signatures and embedded-resource/version comparison. The checker now explicitly renders frames before measuring layout, retaining every geometry assertion after two intermediate-layout failures. See the [handoff record](../research/cycle-111/README.md). All local copies are `0.3.0-dev10`, macOS `0.3.0 (111)`. [Optional exercise](../research/usability/cycle-111.md).

Limits: diagnostic input is synthetic Qt input in the actual app's isolated temporary window. Physical VoiceOver/input/display acceptance remains open. No Cycle112 implementation, notarization or public release is included; further product work should follow Andrew's review.


## Cycle 112 — independent pane zoom and split sizing

Planned scope: Andrew requested separate size adjustment for both sides of Split. Provide matching compact − percentage + controls, independent saved 75–200% magnification, percentage presets/reset, optional linked zoom, a more readable Preview baseline, and discoverable divider drag/double-click balance.

Changes: shared header control and zoom state, one-time legacy Source-size migration, active-pane routing for text-size commands, matching header baselines and compact fit. Preview and Visual Edit share their pane’s zoom. Retained logical text anchors prevent rewrap drift through repeated zoom/reset; real reading/editing interaction invalidates the saved anchor. Deferred Preview parsing/styling remains covered by the viewport transaction. Screen zoom does not write Markdown or output typography. The document divider balances within the existing Source 480px / Preview 320px minimums.

Validation: **163 regression tests and 8 focused native checks pass**. The real executable passes 124 footer states, seven daily-writing workflows and five pane-zoom workflows, including strict 2px anchor bounds (measured 0px). Final artifact identities: see [Cycle112](../research/cycle-112/README.md). Target: `0.3.0-dev11`, macOS `0.3.0 (112)`, stable Dev, ordinary demo/package and Applications. [Optional exercise](../research/usability/cycle-112.md). Native verification uses synthetic Qt input in isolated app windows; physical mouse/keyboard, VoiceOver and display-scale acceptance remain open.

Andrew’s zoom request supersedes the proposed Cycle112 focus-safety scope. Formatting focus safety and safe link editing remain the next bounded improvements, followed by everyday-writing acceptance and review-led image/table/list ergonomics. Public distribution remains separate.


## Cycle 113 — formatting follows Source focus

Planned scope: prevent toolbar, keyboard and native formatting commands from editing an old Source selection while another field or writing surface owns focus.

Changes: a shared Source-ownership guard covers wrapping, block commands, direct insertions, transformations and formatting enabled states. Actual Source focus establishes ownership; deliberate formatting controls retain it, while Find/Replace, outline/filter/link fields, navigation, Preview and Visual Edit revoke it. Hidden Source, read-only state and active composition block formatting. Keyboard traversal into Source formatting preserves its selection. Typewriter labels now explicitly identify their Source-only scope; file navigation stays available independently.

Validation: new positive/negative formatting regressions and Dev/demo/Applications actual-executable focus checks pass. The final integrated gate passes 173 regressions and 13 focused native checks (11 workflows plus setup/cleanup); all three copies pass their actual-bundle/resource/signature checks. [Cycle116 handoff](../research/cycle-116/README.md). [Cycle113 scope](../research/cycle-113/README.md). Target: `0.3.0-dev12`, macOS `0.3.0 (116)`. Physical IME/accessibility acceptance remains separate.

## Cycle 114 — safe inline-link editing

Planned scope: make the existing link popup consistent and prevent stale dialog targets from replacing unrelated writing.

Changes: a shared Insert/Edit link dialog with labelled text, destination and optional title fields, consistent actions, explicit errors and one-step replacement. All Source link-entry routes share the dialog. A captured document session rejects intervening edits, file changes and reloads, even when text is later restored; entered fields remain available after refusal. Explicit Cancel/Escape restores the safe original Source selection and reading position. Outside-click dismissal respects the clicked destination. Supported ordinary inline links retain decoded fields and untouched syntax when saved without changes; ambiguous syntax stays in Source.

Validation: four focused syntax/dialog regression slots pass, as do Dev/demo/Applications actual-executable link/Cancel/Undo and narrow/dark stale-session groups. The full integrated gate passes as recorded in the combined handoff. [Scope and limits](../research/cycle-114/README.md). Runnable target is the combined build116 after verified refresh; no broad Markdown link-grammar or cross-app parity claim is included.

## Cycle 115 — actual-executable daily-writing acceptance

Planned scope: extend the existing isolated installed-bundle checker to verify everyday writing interactions and continuous resizing in the executable Andrew will run.

Changes: six acceptance groups cover formatting focus, link insert/edit/cancel/Undo, stale-link refusal in narrow dark layout, real table Tab navigation, continuous 720–1440px by 520–800px resizing with workspace restoration, and access to all narrow export bands through horizontal scrolling plus Cancel. They supplement existing footer, daily-writing and pane-zoom checks. Reports identify the executable and compare embedded resources/version with the checkout. The wider keyboard-routing pass also repairs F6/Shift-F6 skipping read-only Preview after footer consolidation; traversal now focuses the actual rendered surface and retains Source selection.

Validation: All three review copies each pass 124 footer states, seven writing workflows, five zoom workflows and all six added groups. Resource/version comparisons and strict signatures pass with zero QML warnings. The integrated suite passes 173 regressions and 13 focused native checks (11 workflows plus setup/cleanup). The separate native-window smoke passes 79 assertions, then macOS refuses app activation; later native keyboard/full-screen/Drawer/tab-detach and print/share scenarios remain unverified. [Cycle115](../research/cycle-115/README.md). Detailed logs, synthetic captures and raw reports remain local and ignored; only reviewed summaries belong in the public repository. Synthetic Qt input in an isolated app window does not replace physical mouse, IME, VoiceOver or display tests.

## Cycle 116 — keyboard navigation within visual tables

Planned scope: improve one bounded daily-writing task without expanding into a general editable grid: move among supported Markdown table cells in Visual Edit.

Changes: Tab/Shift-Tab navigate supported cells in row order, including empty cells, preserving table alignment, spacing and source bytes. Navigation creates no edit or Undo unit. First/last cells and unsupported rows remain boundaries; cross-cell selections and stale projections are refused. Typing within the destination cell uses the existing validated source mapping and canonical Undo/Redo. F6 remains a route out of the writing surface.

Validation and artifacts: all 173 regressions and 13 focused native checks pass (11 workflows plus setup/cleanup), along with every actual-bundle check in all three refreshed local copies. Tests cover CRLF, Unicode/graphemes, formatting, empty cells, unsupported separators/rows, selection, stale projections, typing and Undo/Redo at narrow/dark/larger zoom. Target `0.3.0-dev12`, macOS `0.3.0 (116)`, verified in stable Dev, ordinary/demo and Applications, with exact identities in the handoff. [Handoff](../research/cycle-116/README.md), [optional exercise](../research/usability/cycle-116.md).

Remaining scope: Andrew’s normal composition review and physical input/accessibility/display checks; safe richer image placement, table row/column restructuring and complex list editing; broader parser/export/interoperability and multi-window acceptance. Public distribution remains a separate decision.


## Cycle 117 — pane-owned controls and compact file rows

Planned scope: address Andrew’s review of footer placement, missing Preview document identity and oversized library excerpts.

Changes: Source appearance and Preview template controls follow their respective panes; Visual Edit centres within available rendered-pane space, clamps and compacts as needed; only Source / Split / Full remain fixed at the far right. Preview headers show a document icon, bold actual filename and separate Edited state. Library rows restore document icons and actual filenames with short grey dates alongside, tighter grey two-line excerpts and consistent indentation from real tree depth. Current-folder List rows remain siblings. File actions, stored display choices, Markdown and Undo semantics are retained.

Validation: **176 regressions and 12 focused native checks pass, zero failures/skips**; the native count includes setup/cleanup. Dev, ordinary/demo and Applications each pass 124 footer states, seven daily-writing workflows, five zoom workflows, six acceptance groups and three pane-chrome groups, with zero QML warnings. All three copies are refreshed to `0.3.0-dev13`, macOS `0.3.0 (117)`, with matching resource/version identities and valid strict signatures. [Build identities](../research/cycle-117/verified-builds.json). [Scope and current status](../research/cycle-117/README.md), [optional review exercise](../research/usability/cycle-117.md).

Limits: the previous wider native-window run’s activation refusal remains open, along with physical keyboard/IME, VoiceOver, display-scale and historical parser/export/multi-window acceptance. This presentation cycle is not a parity or public-release claim.


## Cycle 118 — separate editing and layout choices

Planned scope: resolve Andrew’s report that Visual Edit, Source, Split and Full mixed different concepts in the footer. Provide independent **Source / Visual Edit** editing and **Single / Split** layout controls, preserving his chosen editor during layout changes and his layout during editing changes.

Changes: Single displays the chosen editor alone. Split displays that same editor at left with read-only output Preview at right, including Visual Edit + Split. Preview-only is an explicit reading action outside the two footer capsules. Both capsules retain fixed widths and positions at a fixed document-area width; pane-local appearance/template menus compact before the controls. Headers identify Visual Edit versus output Preview, and source formatting remains guarded against editing an inactive surface. Editor appearance/zoom remains independent from output-template/Preview zoom. Compact Aa menu labels remain readable and pane-tool width budgets prevent transient overlap during relayout. Source formatting, clipboard and completion guards cannot target a hidden Source selection; pending completions close on leaving Source. Find explicitly reveals Source without changing Single/Split and closes when leaving Source. Version2 workspace state accepts old version1 checkpoints, converting Full Visual Edit to Visual Edit + Single and preserving read-only Preview-only; version2 remembers the last editing layout. Responsive contraction retains the chosen editor.

Validation: **178 regressions passed, 0 failed, 0 skipped**, including setup/cleanup. Dev, ordinary/demo and Applications each pass **147 actual-executable checks**, matching embedded UI/version and **zero QML warnings**. All three copies are refreshed to build118 and strict-signature verified; demo/Applications have identical executables. **12 focused Cocoa checks pass, 0 failed, 0 skipped**, including setup/cleanup. Review build `0.3.0-dev14`, macOS `0.3.0 (118)`. [Cycle118 status](../research/cycle-118/README.md), [optional exercise](../research/usability/cycle-118.md). Production source commit: `4cfdfd80793fc2b773a9bd9c702d14d5dc79aaf9`; [verified build identities](../research/cycle-118/verified-builds.json).

Limits: Visual Edit remains a bounded projection over canonical Markdown, with explicit Source fallback for unsupported structures. This control/state change does not add general WYSIWYG, richer table/list/image restructuring or export/parser parity. The earlier native foreground-activation limitation, physical mouse/keyboard/IME, VoiceOver, display-scale and historical multi-window acceptance remain open. Public release/notarization is separate.


## Cycle 119 — editing, publishing and quiet document bars

Approved scope: Source / Visual Edit belongs to the left editor; Web / PDF belongs to the read-only right publishing pane. Separate pane controls from the overall Single / Split layout choice. Hide document bars during typing/scrolling, reveal the matching edge on hover, and preserve keyboard/menu access and geometry.

Implemented: pane-owned controls with readable compact menus; shared export/preview HTML and PDF rendering; bounded private preview files; persisted publishing choice and auto-hide preference; View toggle, F6/keyboard recovery, independent edge reveal, and asynchronous Web reading-anchor preservation through zoom/divider changes. Added QtWebEngine and QtQuick.Pdf dependencies.

Verification and runnable copies: [Cycle119 handoff](../research/cycle-119/README.md). [Optional review exercise](../research/usability/cycle-119.md). Dev, demo and Applications each pass 149 actual-executable checks, strict signatures and matching UI/version with zero QML warnings. The full regression run passed 181 cases plus one stale label expectation; that expectation was corrected and the final focused rerun passed 8/8, including the native Web selector. Production commit `19b3f89127709205f740793a37fb24e45b9b667d`. Packaging fixes bundle-local dependencies for the nested browser helper. Physical IME, VoiceOver, multi-display acceptance and general WYSIWYG/parser parity remain open. No public release or notarization in this cycle.


## Cycle 120 — persistent edge reveal and separate pane tones

Andrew’s follow-up: keep an edge-revealed document bar visible after the pointer leaves, until editing or scrolling resumes. Use iA Writer’s soft grey for the editing surface and retain the warm publishing surround.

Implemented: independent top/bottom reveal latches reset on every editing/scroll gesture; ordinary publishing clicks no longer hide controls. Existing open-menu and keyboard protections, always-visible preference and fixed geometry remain. Studio uses grey for Source/Visual Edit and their matching header/footer; publishing retains its cream surround and actual output page colors. Other themes keep their existing surfaces.

Verification, runnable copies and any remaining limitations: [Cycle120 handoff](../research/cycle-120/README.md). [Optional review](../research/usability/cycle-120.md).

Validation: **183 passed, 0 failed, 0 skipped**, including setup/cleanup. Dev, demo and Applications each pass **149 native checks**, with matching UI/version, valid strict signatures and zero QML warnings. Review build **0.3.0-dev16 / macOS 0.3.0 (120)**. No public release. Physical IME, VoiceOver, multi-display and broader parser/export parity remain open.


## Cycle 121 — explicit fade and always-visible menu choices

Requested: expose Fade In/Out and Always Show in View → Title Bar, as in Andrew’s iA Writer reference. Existing labels were still connected to a legacy hover-opacity preference that conflicted with document auto-hide. Both choices now use the current persisted document-bar preference, with synchronized checkmarks and the existing Auto-Hide Document Bars checkbox. Fade retains sticky edge reveal; Always Show keeps top and bottom bars visible through typing/scrolling. Separate toolbar-content preferences remain unchanged.

[Verification and runnable copies](../research/cycle-121/README.md). [Optional review](../research/usability/cycle-121.md).

Validation: **183 regressions pass**, including preference persistence after reopening. Dev, demo and Applications each pass **149 native executable checks** with matching embedded UI/version and strict signatures. Installed-copy verification is recorded in the handoff. Build **0.3.0-dev17 / macOS 0.3.0 (121)**.


## Cycle 122 — literal, readable Source

Requested: consistently visible Markdown in Source and clearer typography/list structure following Andrew’s iA Writer reference. Source now always shows syntax; Visual Edit is the formatted alternative. Manuscript uses bundled iA Writer Mono at an 18px base with 155% line spacing. Hanging indents align wrapped lists, tasks and quotes without changing Markdown. Literal code/fences/math and conservative nested-code handling retain their layout. Typography joins actual edits and defers font-refresh formatting while Redo is pending.

[Verification and limits](../research/cycle-122/README.md), [optional review](../research/usability/cycle-122.md). Build 0.3.0-dev18 / macOS 0.3.0 (122). **185 regression tests pass, 0 failed/skipped**; all three refreshed app copies pass **150 native checks** each, matching UI/version and strict signatures with zero QML warnings. Source commit `a8f2c7902c011d670dae127a19bf28ef2d0124be`.


## Cycle 123 — Code appearance and aligned Manuscript headings

Requested scope: add a fourth Code writing appearance with familiar syntax colours and indentation, hang Manuscript heading markers beside aligned body text, and make fenced backgrounds continuous.

Implemented: Code in the Source Aa, compact/native Writing Appearance menus and command palette; Menlo 15px/135%, line numbers, four-column guides and horizontal scrolling without wrapping. Tab/Shift-Tab and Return preserve/infer indentation, adding a level after `{`, `[`, `(` or `:`; Code retains straight quotes/dashes and plain pasted URLs. Lightweight file/fence lexers cover JS/TS, Python, JSON, C/C++, shell, CSS and HTML. Manuscript uses a seven-character block gutter and negative heading indent so words and wraps align with the body. Fenced backgrounds cover empty lines and closing markers in every appearance; inline code remains compact. Pending-Redo typography refresh remains deferred to preserve history.

Verification: **189 regressions passed, 0 failed/skipped**. Dev, demo and Applications each passed **151 native checks** with matching UI/version and strict signatures. Native captures were visually reviewed. Build **0.3.0-dev19 / macOS 0.3.0 (123)**. [Verification](../research/cycle-123/README.md), [optional exercise](../research/usability/cycle-123.md).

References: official VS Code editing conventions and Qt syntax-highlighter documentation; no external code or new dependency copied. This adds a lightweight code appearance, with no language server, autocomplete, linting or debugging. Physical input/IME, VoiceOver, multi-display and historical parser/export/native-activation gaps remain open. The stable Dev, ordinary/demo and Applications bundles are refreshed and verified.


## Cycle 124 — CSS publishing themes and shared browser output

Approved scope: make publishing themes useful CSS document styles, add Claude Like from Andrew's actual local Typora theme, and render Web/PDF output consistently in the publishing pane, export dialog and exported files. Retain independent Source/Visual Edit appearance and exact Markdown/Undo behaviour.

Implemented: semantic MD4C HTML preserves headings, nested lists/quotes, tables, code, links, supported footnotes/highlights and page breaks while escaping arbitrary raw HTML. Claude Like adapts the original document colours, spacing, tables, quotes and code to semantic output with Georgia fallback. Footer and native Publishing Theme menus expose built-in/imported CSS themes plus import, folder and reload actions. Managed imports flatten bounded local CSS imports and embed supported raster images/fonts; remote resources and unsafe markup are rejected, and Typora export-only directives are omitted with an advisory. Legacy font/page presets remain labelled basic themes. Shared Chromium output supplies Web and vector PDF export; Export Hub now previews the actual Web/PDF result, with separate asynchronous PDF jobs per preview. Native rendered printing rasterizes the browser PDF at 300 dpi. Ambiguous text-based PDF heading destinations are refused.

Verified **0.3.0-dev20 / macOS 0.3.0 (124)**: **193 regressions pass, 0 failed, 0 skipped**. Dev, ordinary/demo and Applications each pass **154 native checks** with matching UI/version, strict signatures and zero QML warnings. Checks include the actual Claude CSS import, painted Web/PDF themes, paper settings, repeated publishing/editing/layout changes, exact Undo/Redo and unchanged saved source. Native runs use synthetic Qt pointer/key events on Cocoa; physical input remains separate acceptance work. A read-only private-document comparison produced two A4 pages with pixel-identical first pages for the bundled and imported Claude theme using Georgia fallback, with source unchanged. Private documents, exports and raw captures remain outside the public repository. [Verified builds and evidence](../research/cycle-124/README.md), [optional exercise](../research/usability/cycle-124.md).

Compatibility limits: remote fonts/assets are blocked, compatible local assets embed into standalone CSS/HTML, and Typora UI/editor selectors have no corresponding output elements. Math is retained without typesetting. CSS escapes, SVG theme assets, parent-relative theme resources and advanced import modifiers remain unsupported. Physical printing, VoiceOver, display/IME and broader Markdown/template interoperability remain unverified. No public release or notarization is included. Primary references: [Typora themes](https://support.typora.io/About-Themes/) and [Typora export](https://support.typora.io/Export/).

## Cycle 125 — theme-folder discovery and reliable selection

User feedback exposed stale theme menus, multiple checkmarks after failed selection, local theme resources rejected by the stored-theme loader, and unclear basic-settings actions. CSS files now load directly from the themes folder, including ordinary mixed-case/underscore names and local imports/fonts/images; a watcher refreshes catalog and selected output after additions, edits or removal. Import uses the same rules, with missing optional fonts falling back visibly. The checked menu state follows only accepted selection. Basic font/page settings have explicit labels and an edit action; long menus scroll within available space above their button. Failure notices remain visible.

Review build **0.3.0-dev21 / macOS 0.3.0 (125)**. All **195 regressions pass, 0 failed, 0 skipped**. A read-only probe loads all 13 existing top-level user themes without failures. Native checks include repeated valid/repeated/rejected choices, CSS application, live file edits, reload, basic settings and dialog callbacks; **157 checks pass per app copy**, with matching UI/version and strict signatures, as recorded in the [handoff](../research/cycle-125/README.md). Private themes and writing remain outside Git. [Optional user exercise](../research/usability/cycle-125.md).

Compatibility remains bounded: top-level CSS plus resource folders, supported local fonts/images, no remote downloads or arbitrary Typora UI compatibility. Physical input, VoiceOver and printer output remain separate acceptance work.

## Cycle 126 — Get More Themes

Added **Get More Themes…** to the publishing footer menu, View → Publishing Theme and Export and share. It opens Andrew's exact requested address, https://theme.typora.io/, in the default browser. Downloaded CSS/resources still use the existing themes folder or Import Theme action. Build **0.3.0-dev22 / macOS 0.3.0 (126)**. Existing build, regression and native menu checks are recorded in the [handoff](../research/cycle-126/README.md); no new tests were added for this small link. [Optional browser exercise](../research/usability/cycle-126.md). No change to theme compatibility or outstanding physical/accessibility acceptance.


## Cycle 127 — readable publishing previews and continuous refresh

Requested scope: investigate Andrew's empty publishing pane and image-error notice, and reduce flashing while publishing previews refresh. Keep the document visible when a local image is missing, unreadable, damaged or over budget, or when an image points to a remote URL. Preserve strict export/print behavior and exact writing history.

Implemented: Web and PDF preview replace unavailable images with escaped, visible alt-text placeholders and a warning while rendering the rest of the document. Compatible local images embed within the existing 5 MiB each / 20 MiB total limits; image decoding rejects damaged data. Successful preview results cache per consumer by generated HTML, output format, document base URL and warning, capturing local asset bytes and print-layout CSS. Repeated unchanged refreshes reuse the URL, and identical pending PDF requests reuse their request ID. Superseded jobs disconnect and cancel before applying results; all consumers release pinned files and cache state when destroyed. Publishing surfaces keep a painted snapshot while changed output loads and avoid navigation on unchanged URLs. Preview work does not edit source Markdown or saved files.

Review build **0.3.0-dev23 / macOS 0.3.0 (127)**. Build/package and all **197 regressions** pass. Dev and the installed copy each pass **159 native checks**, with zero unexpected QML warnings; deliberate missing-image resource diagnostics are retained separately. Inspected Web/PDF held-frame and missing-image screenshots. All three bundles have matching verified product identities and strict signatures; the packaged executable matches the installed copy. [Current handoff](../research/cycle-127/README.md), [optional physical exercise](../research/usability/cycle-127.md). The previous Cycle126 results remain historical evidence, not validation for this change.

Remaining acceptance: Andrew should retry the document and repeated refresh workflow that exposed the blank pane and flashing. Synthetic native checks cannot establish the physical report resolved. Placeholder previews are intentionally incomplete; export/print still refuse unavailable images. Physical input, VoiceOver, multi-display and broader CSS/Markdown compatibility remain separate acceptance work. No public release is included.


## Cycle 128 — preview document identity and natural loading

**Requested:** repair ordinary-app blank PDFs and Dev showing the previous document; update tests to catch both.

**Changed:** removed screenshot-gated loading/retained overlays; invalidate output on document changes; bind backend/UI results to document lifetime/source/caller tokens; restore already-loaded cached PDF pages; hide empty Web output; bounded loading error/retry.

**Verification:** 202 regressions pass. Component tests first reproduced stale A→B output and a compositor-dependent replacement stall. Native tests now verify actual selected-document identity and marker pixels after natural frame presentation, including A/B/C, hidden/show and Web/PDF/theme transitions. Final candidate passes 163 native checks. [Bundle verification and deployment](../research/cycle-128/README.md).

**Limits:** synthetic Cocoa tests, not physical input. Cold component coverage does not establish full persisted-workspace relaunch. No forced quit or unsaved-work discard. [Optional review](../research/usability/cycle-128.md).


## Cycle 129 — stop theme notifications starving normal startup

**Requested:** fix continuing blank previews across app copies and explain why prior verification missed the failure; prepare a candid developer handoff.

**Changed:** reconcile watcher paths instead of removing/re-adding all paths; cache theme content snapshots and notify only actual changes; invalidate cache on file events to detect preserved-mtime/same-size edits; re-arm atomic replacements; retain forced Reload Themes. Pending empty-URL output shows loading status.

**Verification:** old normal native startup with a populated copy of Andrew's themes fails: 67 theme signals/backend in 15 seconds and no output. Corrected cold/restored normal Main startup passes with QML caching enabled, guarded quit and no forced frames. Actual Dev saved workspace renders matching PDFs; OS capture inspected privately. Final full suite: **204 passed, 0 failed, 0 skipped**. Expanded final watcher fixture: four passes (two cases plus setup/cleanup). Added offscreen watcher tests also pass old code, so they are not the red/green evidence. Dev, packaged ordinary and installed build129 each pass four focused preview groups/eight pane paints, strict signatures and zero QML warnings; ordinary/installed executable hashes match. The full offscreen suite retains two Qt Material SplitView warnings. [Build identities](../research/cycle-129/verified-builds.json). [Current evidence](../research/cycle-129/README.md).

**Limits:** Cycle128's document-identity/screenshot fixes remain valid, but its component checks missed this separate startup loop. Native startup harness is compile-only; it does not establish physical input or full packaged UI acceptance. Keep private documents/themes/raw reports outside Git. Review build **0.3.0-dev25 / macOS 0.3.0 (129)**. [Optional review](../research/usability/cycle-129.md).
