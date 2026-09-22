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
