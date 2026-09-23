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
