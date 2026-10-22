# Cycle 158 — Windows Save shortcut immediately after a menu command

22 October 2026. Bounded response to the exact `0e9ef4a` Windows ARM64 selection/format/save observation. Package and current build identities remain in [Windows STATUS](../../docs/windows/STATUS.md). Host red/green establishes a menu-closing shortcut defect; native Windows qualification of this candidate remains pending.

## Finding and change

The native zero-gap input observation selected `input.` in Live, pointer-activated Format → Bold, then sent Ctrl+S without sleeping or inspecting the document between inputs. The resulting Live text was correctly bold and dirty, while the saved copy retained its old bytes. Evidence is retained under `dist/windows-testing/results/final-0e9-arm64-selection-save-2ab43f81942e4f3e9220bc369951ee4d/evidence/`.

The host regression isolates the trigger. Windows popups inherit Material's exit transition: 220 ms for scale and 150 ms for opacity. A closing popup remains visible and blocks the main window's `Qt.WindowShortcut`. Restoring Live focus before dispatch does not release that popup boundary. The settled Live control activated Save and wrote the expected bytes; the immediate Live case had real Live focus and a real selection-format request, but **zero Save shortcut activations** while the menu was visible and closing. The persistence code was never entered by that chord.

The product change is only `exit: null` on WindowsMenuBar's shared `WorkspaceMenu` component. Dismissing a Windows menu now releases the popup boundary immediately, before the next writing shortcut arrives. It does not change Save, Live selection/persistence ordering, focus restoration, shortcut scope, modal dialogs or the native Mac menu. Enter transitions are retained.

The mechanism agrees with [Qt 6.11's shortcut-context matcher](https://raw.githubusercontent.com/qt/qtdeclarative/6.11/src/quicktemplates/qquickshortcutcontext.cpp): window shortcuts are rejected when a blocking popup with a modal or CloseOnEscape boundary lies outside the shortcut's item. Material's exact transition values were read from the installed Qt 6.11.2 `QtQuick/Controls/Material/Menu.qml`.

## Regression

The new `windowsMenuFormattingKeepsImmediateSaveShortcut` case checks the correct production menu boundary, then explicitly activates the actual Windows component on the host Mac. It uses the shipped Live page, a genuine page selection of `input.`, actual pointer activation of the Format menubar entry and Bold MenuItem, and the real Ctrl+S Shortcut. No registry substitute, direct Backend Save, fabricated bridge reply, clipboard operation or selection timeout is used for the critical pair.

The case runs three scenarios: settled Live control, immediate Live, and immediate Source. The settled control waits until the menu is invisible. The immediate pair calls `QTest::mouseClick` and `QTest::keyClick` consecutively, with no intervening wait, focus repair, page request, observation or file poll. QtTest processes each input through its normal QWindow input path. Spies record the actual Bold trigger, Live selection request, Save shortcut activation and Save success/failure. Assertions require the expected formatted bytes on disk, matching Source/Live text and a clean document. Source setup completes the queued document-load caret restoration before establishing its selection.

This host component test cannot certify Win32 SendInput ordering, native Windows focus/window ownership or the deployed executable. The unchanged native zero-gap selection/format/save sequence remains required after packaging.

## Verification

- Required `./bin/build`: **passed**, Qt 6.11.2 ARM64/macOS; complete output is retained as `evidence/build.log`.
- Genuine old-product red, with the default Windows menu exit transition: **2 passed, 1 failed, 0 skipped**, including init/cleanup, **2.314 seconds**, Qt 6.11.2 ARM64/macOS. The settled Live pair took 263 ms and activated Save once; the immediate Live pair took 5 ms, retained the real Live render item's focus, and requested selection once, but the menu was still visible/opened=false and the Save shortcut activation count was **0**, expected **1**. Log: `evidence/menu-save-old-red.log`.
- Corrected focused green: **7 passed, 0 failed, 0 skipped**, including init/cleanup, **8.990 seconds**. Cases: `windowsMenuFormattingKeepsImmediateSaveShortcut`, `liveUndoSaveWaitsForCurrentPage`, `liveSaveAsAndSelectionCommandsPreserveOrdering`, `previewTemplateFooterAndNativeMenuShareState` and `documentChromeKeepsMenusKeyboardAndNativeToggleAccessible`. The immediate Live pair took 1 ms and Source 15 ms, with the menu already invisible and the Save shortcut activated exactly once; each scenario then verified the expected saved bytes and clean state. Full compile/test output: `evidence/menu-save-green.log`.
- The first post-fix batch had **6 passed, 1 failed, 0 skipped**, **26.835 seconds**. Both Live scenarios already passed; the optional Source scenario exposed the fixture's queued document-load caret reset, so it formatted at position zero. Source preparation now settles the load before selecting. This output is retained as `evidence/menu-save-first-green-failed.log` and is not old-product red evidence.
- That first post-fix batch mistakenly included the existing clipboard-using `windowsMenuRoutesPortableCommands` case. It passed under offscreen; an attempted targeted stop reached the process after completion. The parent was informed immediately, and the corrected green batch explicitly excludes it. No full suite or dedicated clipboard case ran in this slice; any concurrent native clipboard evidence must be assessed by the parent.
- `git diff --check`: clean.
- No VM commands, driver changes, Win32 input workaround, commit or push were performed. Primary Mac checkout/apps are unchanged. Full regression, native ARM64/x64 execution, packaged zero-gap input qualification and promotion remain pending.


## Shared dirty-state limitation

A separate ordinary `0e9ef4a` clipboard run restored the exact native-opened Unicode fixture through Delete → Paste Plain Text → Undo → Undo, but retained a starred title and “Unsaved” status. This is existing shared behavior: `Backend::editorTextChanged()` conservatively marks changed text dirty, including Undo back to saved text, until an explicit Save. It is not evidence of a Windows clipboard/Unicode transport failure or a regression from this menu-exit change. Evidence: `dist/windows-testing/results/final-0e9-arm64-clipboard-146d3bc8eadf436fa197a59f396689bc/evidence/clipboard-menu-report.json`, SHA-256 `122d0abbd2532b3c604b3737495671521179bedc154a59ac4450d527e79e5214`; the gate failed its clean-title expectation and is not a passing qualification. This cycle leaves dirty-state semantics unchanged to preserve authorship sidecar, explicit-save and persistence-failure safeguards. No user-clipboard preservation claim follows from this observation; the earlier r3 restoration remains unknown.

Full local logs remain in the ignored `evidence/` directory. The two-line comment and one property in WindowsMenuBar are the entire product fix; this record does not claim native package qualification.

## Integrated acceptance correction

The subsequent integration also replaces stale deployed-checker assumptions that Windows omits Title Bar and Output Style menus. The checker now requires and activates actual Windows Title Bar items and requires a single authoritative selection in both the footer and Windows Output Style submenu. Mac native-menu assertions remain. The first integration build caught an invalid bare return in the top-level theme-check include; it was corrected before execution, and the required build passed. The earlier failed compile log is retained at `/private/tmp/fomawrite-158-integration-build.log`; the corrected log is `/private/tmp/fomawrite-158-integration-build-r2.log`.

Independent review found and corrected a test masking issue: the Output Style fixture now asserts native Helvetica immediately, resets to Palatino, and only then exercises Windows Helvetica. One route cannot repair the other's failed state before its assertion.

## Complete integration gate

After the independent menu-test correction and deployed-checker update, required `./bin/build` passed and the complete `./bin/test` suite passed **375 / 0 / 1**, **539.771 seconds**. The sole skip is Windows-only native appearance on the Mac host. Generated Cycle134 reference captures were restored; no golden was changed to obtain green. Deployment/input verifier fixtures passed **36 + 25**; Live/Qt-install fixtures passed **25** using Python3.14. The initial system-Python run was incompatible with `hashlib.file_digest` (seven errors), retained separately, then rerun on the supported runtime. Exact native CI and packaged input qualification remain pending.
