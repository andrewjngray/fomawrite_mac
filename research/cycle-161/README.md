# Cycle 161 — Windows Center and existing shortcut bindings

22 October 2026. Bounded continuation after the Cycle158 checkpoint and the separate Format159/File160 menu slices. Current build and qualification identity remain in [Windows STATUS](../../docs/windows/STATUS.md); the downloaded checkpoint158 diagnostic packages do not contain these changes.

## Change

Windows adds the existing Window → Center command and five native-menu counterpart bindings: Open Path (`Ctrl+Shift+O`), Command Palette (`Ctrl+Shift+P`), Review (`Ctrl+:`), Next Issue (`Ctrl+;`) and Previous Issue (`Ctrl+Alt+;`). Each shortcut has one Windows-only `WindowShortcut` registration, reuses the existing guarded command, and goes through the established focus dispatcher. Existing Mac menu commands and shortcut registrations remain unchanged.

Center moves only the attached Windowed window's native frame to the center of its own screen's available geometry, without resizing. Missing windows/screens and non-Windowed states do nothing. The menu follows the same Windowed guard. The existing Mac implementation remains separate and unchanged.

## Regression and evidence

- Tests-only old-code red: **2 passed, 6 failed, 0 skipped**, 1.263 seconds, exit 6. The six failures require the actual missing Center node and five missing shortcut registrations. Log: `evidence/center-shortcuts-old-red.log`.
- The existing Center regression retains its native Mac production row and adds an explicit Windows-component row on non-Windows hosts. It checks actual menu activation, frame centering, repeated centering, unchanged size, other window, document, dirty/Undo state and original bytes. Native Windows additionally checks disabled menu/direct-call behavior for maximized, full-screen and minimized states, plus a missing target.
- Five shortcut rows require exact sequences, `WindowShortcut` scope and one registration. On Mac they require disabled Windows registrations and unchanged independent native counterparts. On Windows they deliver Qt key events to the actual owning window, check dialog activation/modal blocking or real bundled-Harper Review navigation, unavailable-engine guards, another window's nonactivation, and retained draft/Undo/disk bytes. No synthetic engine readiness is used.
- The first post-change run was **14 passed, 1 failed, 0 skipped**. The fixture's Mac Command Palette lookup selected the explicitly loaded Windows Go item with the same name, yielding an empty native `shortcut`. This diagnostic is retained as `evidence/center-shortcuts-first-green-failed.log`. Scoping the native lookup outside the Windows menu tree fixes only that fixture collision; the exact native binding assertion remains.
- Final focused gate: **15 passed, 0 failed, 0 skipped**, **10.513 seconds**, exit 0. Log: `evidence/center-shortcuts-focused-green.log`. It covers both Center rows, all five registrations, bundled Review toggle/navigation, existing Go/Recent routing and immediate menu-format Save.
- Required `./bin/build` passed, Qt 6.11.2 ARM64/macOS. A final resource refresh after a comment-only correction also passed. Logs: `evidence/build.log` and `evidence/build-final.log`.

Focused command:

```sh
./bin/test nativeWindowCenterMovesOnlyTargetAndPreservesDocument windowsShortcutsUseExistingWindowCommands nextAndPreviousIssueWrapRoundTheDocument:bundled-harper reviewPaneTogglesWithTheCommandAndTheMenuTickFollows:bundled-harper navigationHistoryAndQuickOpenStayIndependent goMenuRoutesSavedQueriesHashtagsAndGuardedRecents recentMenuTracksLibraryAndGuardsOpen windowsMenuFormattingKeepsImmediateSaveShortcut
```

## Integrated suite and menu fixture correction

The first complete integrated run returned **415 passed, 6 failed, 1 skipped** in **569.264 seconds**. All six failures were existing View-menu rows whose entries moved below the small test window's viewport after the additional routes. The failed log is retained as `evidence/integrated-full-suite-first-failed.log`.

The fixture now opens ancestor menus with actual Home/Down/Right navigation and uses actual pointer activation for every final action. Clipped root-level targets use timestamped wheel input with positive layout readiness and strict viewport/ancestor clipping checks. It does not write contentY, visibility or action state. Repeated checked bindings, editor focus, Unicode selection, revision, Undo and dirty-state assertions remain. Focused result: **22 passed, 0 failed, 0 skipped**, **49.327 seconds**, `evidence/view-navigation-focused-green.log`. Intermediate failed diagnostic approaches are retained locally and are not counted as passing evidence. The complete required suite must still be rerun.

## Checkpoint158 native CI and fixture repair

Windows run **39787797715**, attempt1, source **2f0f272770f031019a44fa2a7b0ebc99fecd8026**, returned **342 passed, 2 failed, 0 skipped** on each native architecture (Qt6.11.3/MSVC2022 offscreen; full-suite exit2). Native appearance passed **3/0/0** on each. The two failures were identical: the empty hashtag item was effectively invisible while its Go ancestor was closed; the recent placeholder scan counted Material label children as well as actions, producing6 rather than2. Both Focus/chrome/Go proof steps were skipped after that failure; skipped does not qualify the proof.

The explicit Windows-component host rows reproduced those exact assertions: **2 passed, 2 failed, 0 skipped**,17.892seconds (`evidence/native-fixture-host-red.log`). The repair opens Go → Hashtags through actual pointer clicks before the unchanged visibility/disabled assertions and counts only real `triggered()` menu roles while retaining the exact two File/Go placeholders and all dirty-buffer, selection and Undo guards. Platform production assertions run before the explicit Windows loader is activated. Both original Mac and Windows-component rows then passed: **6/0/0**,4.432seconds (`evidence/native-fixture-host-green.log`). This is a fixture correction; it does not claim a new native Windows pass.

The same checkpoint's x64 deployed document-view report and package verification passed. ARM64 exited **3221225477 / 0xc0000005**, emitted no final report, and last captured `folder-theme-restored.png` before the theme picker stage. This does not prove the crash location. The x64 log warns that its native file-dialog cleanup thread did not finish; the previous d573 ARM64 report passed the same picker stage. Shared run **39787521524** passed independently. API receipts, archive digests, extracted logs and package manifests remain under ignored `dist/windows-testing/results/ci-2f0/`; exact per-file identities and parsed outcomes are retained here as `evidence/checkpoint158-native-identity.json` and `evidence/checkpoint158-native-summary.json`.

## Synthetic picker checker boundary

Only the Windows checker workflow that supplies `selectedFile`/`accepted` itself now sets `DontUseNativeDialog` on the CSS-import and basic-settings FileDialogs before first opening. It checks option writes/readback, closes both dialogs, and verifies original options are restored by a scope guard. Actual menu opening/visibility, all three callbacks, installed CSS and authoritative selections, Georgia14 settings, invalid JSON retention/notice, and unchanged editor/disk bytes remain required. Production picker defaults and Mac picker behavior are unchanged. Theme-stage and picker-boundary messages flush to stderr so a failed process leaves its last completed boundary.

Required `./bin/build` passed (`evidence/checker-build.log`). The direct host offscreen diagnostic completed in87seconds, exit1: its theme-import/basic-settings group passed with all three picker open/callback/close boundaries recorded, while the complete diagnostic failed12 assertions across five other theme groups, beginning with a cascading-menu hover failure and dependent theme expectations. This is not a complete checker pass or a proven baseline regression; no assertions were relaxed. All four later preview groups passed. The full log and report are `evidence/checker-host-offscreen.log` and `evidence/checker-host-offscreen-report.json`. The Windows-only picker-option branch still requires new native CI. Forcing a Qt picker in this synthetic callback diagnostic does not qualify ordinary Windows native picker input or resolve the earlier ARM64 access violation without further evidence.

## Limits

Host green proves actual Windows QML registrations/component actions alongside preserved Mac behavior. It does not execute the Windows-only Backend Center branch or Windows shortcut-delivery branch. The native Windows CI suite uses offscreen QPA: those branches and logical Qt key delivery will execute there, while desktop appearance, physical input and native placement remain separate ordinary-app checks. The checkpoint158 native failures above are retained; native CI for these current repairs remains pending.

`Key_Colon` with Control proves logical registration, not physical `Ctrl+Shift+OEM_1` mapping; punctuation/layout-dependent `Ctrl+Alt+;` likewise needs physical Windows qualification. Ordinary Center placement, multi-monitor behavior and native Print Source cancellation are pending separate desktop acceptance. No VM, clipboard, print/spool action, full suite, commit, push or promotion was performed by this slice. The parent owns integrated full-suite, native CI and exact-package qualification.

## Maintained desktop driver correction

The three reviewed r10 Exit changes are now in `bin/windows/normal-launch-input.cs`, `run-normal-launch-guest.ps1` and `run-publishing-guest.ps1`. The helper retains immutable owned popup/window identity before scrolling, reacquires that same popup and fresh bounds afterward, and no longer requires an unrelated Page Setup accessibility row. Unique visible Exit OCR, stable pixels/coordinates, ownership, fresh physical click and ordinary Exit/relaunch assertions remain intact. The five other maintained counterparts were byte-identical; no unrelated temporary driver was imported.

Normal/Live/deployment report guards passed **74 tests** in **0.416 seconds**, `evidence/maintained-native-driver-guards.log`. Exact unchanged r10 sources already passed the normal and selected Live Save workflows on the checkpoint ARM64 package; these offline guards alone are not runtime proof. See [reviewed checkpoint desktop evidence](../../docs/windows/M2-DESKTOP-2F0.md).

## Second complete run and exact proof names

The second complete suite returned **422 passed, 1 failed, 1 skipped** in **573.493 seconds**. The remaining failure was `goMenuRoutesSavedQueriesHashtagsAndGuardedRecents(windows-component)` at the pointer exposure of its intermediate Hashtags submenu entry; all six previous View failures passed. Log: `evidence/integrated-full-suite-second-failed.log`. The same bounded actual Home/Down/Right ancestor navigation is now applied, preserving the original top Go pointer, empty-item visibility, disabled-state and all unsaved-document assertions. Focused Go/Recent plus six View rows passed **12/0/0** in **23.808 seconds** (`evidence/go-ancestor-focused-green.log`). The final required app rebuild passed (`evidence/checkpoint-build.log`). The complete suite has not yet been rerun after this final fixture correction; native/shared CI and the subsequent integrated cycle remain required. This failed run is not a complete passing gate.

The optional native menu red/green proof now explicitly selects and matches `production-platform` for the two newly data-driven Go/Recent cases. It retains exactly eight cases plus init/cleanup, zero skips, the precise missing-node failure expressions and their current source locations. The offline contract failed before this fix and **75 normal/Live/deployment guards passed** afterward. Actual native proof remains required.
