# Cycle 156 — Windows Output Style and Live theme menu candidate

22 October 2026. Bounded menu continuation from the Focus/chrome checkpoint `ecf777d6c054446cef4635fc56d194b6e8bba4eb`, preserving the uncommitted Go/Open Recent work. This record covers the View/Output slice; current build and package identities remain in [Windows STATUS](../../docs/windows/STATUS.md).

## Change

| Finding | Candidate behavior |
| --- | --- |
| Windows lacked the native menu's Output Style routes. | View → Output Style exposes all eight existing basic styles, the shared dynamic Custom Themes catalog, Edit Basic Settings, Load Custom Settings, theme import, folder, reload and existing theme-gallery link. |
| Windows lacked the existing Live exact-theme switch. | View → Live Follows Theme Exactly uses the existing command registry and its Live-only guard. |
| Checkable menu controls toggle before validation, including repeat or rejected choices. | Each Windows publishing choice dispatches `choosePublishingStyle` and restores its checked binding. Opening either publishing menu also restores bindings recursively. Basic and custom choices retain exactly one backend-selected check. |

Every route uses the existing shared handlers, dialogs and backend. Dispatch retains the existing Windows editing-focus restoration. The NativeMenuBar, Main.qml and publishing backend are unchanged. All menus inherit the existing bounded Windows popup geometry and focus ownership.

## Regression coverage

`previewTemplateFooterAndNativeMenuShareState` first asserts the correct production platform menu. On the host Mac it then explicitly activates the existing Windows menu Loader, as the portable menu fixtures already do, and requires actual Windows menu nodes. This is a host component test, not an assertion that Mac production loads Windows menus.

The tightened existing case retains its Mac native-menu checks and footer pointer interaction, while requiring the Windows Output Style root, eight real root style entries, dynamic custom-theme delegates and the Live exact-theme command. It selects every basic style and checks footer synchronization and one-selection state. Physical Qt popup clicks exercise repeated basic/custom selections and a rejected unsafe local theme; the latter retains the previous basic selection and opens the existing validation notice. A temporary test-only theme is added and removed through the real Reload Themes route. The existing PDF settings hub and settings/theme file dialogs open and close through actual Windows menu triggers. Live exact-theme is checked enabled in Live, disabled in Source, and inert when its disabled signal is invoked. The unsaved draft and Undo/Redo are retained.

The old-menu witness is the required `desktopOutput && desktopThemes && desktopPalatino && desktopHelvetica && exact` assertion: baseline `0e9ef4a71d0c43e21440d9efe0c1d1110998d6c5` has none of these Output Style/Live exact-theme nodes. The former Windows footer-only fallback has been removed.

## Verification and limits

- Required `./bin/build`: passed, Qt 6.11.2 ARM64/macOS. Complete local output: `evidence/build.log`.
- First focused attempt: **2 passed, 1 failed, 0 skipped**, including init/cleanup, **16.280 seconds**. It reached all real menu nodes and basic-style signal assertions, then failed because the fixture opened a submenu while its ancestor View menu was closed. The fixture now opens the full ancestor chain before pointer activation. Output retained in `evidence/focused-first-failed.log`; this is fixture correction evidence, not old-product red evidence.
- Corrected focused batch: Output Style and `documentChromeKeepsMenusKeyboardAndNativeToggleAccessible` both passed. A coordinated batch also established four expected missing Edit/Help menu failures for the parallel slice: **4 passed, 4 failed, 0 skipped**, including init/cleanup, **5.870 seconds**. Full compile/test output: `evidence/combined-focused-pre-edit.log`. These four failures are outside this View slice and must turn green with the separate Edit/Help change.
- Host red/green: replaced only the Windows View block with baseline `0e9ef4a` while preserving Go, Focus and Edit/Help bytes. The test failed at the expected missing Output Style/Live exact-theme node assertion: **2 passed, 1 failed, 0 skipped**, **0.438 seconds**. Exact full-menu bytes were restored in `finally`, with matching before/after SHA-256 `f06868f581d45741cf36345ab3d1cee2fcb31d807ab165265ffe11c942d8d827`. The restored Output Style and document-chrome cases then passed: **4 passed, 0 failed, 0 skipped**, **5.359 seconds**. Logs: `evidence/view-old-menu-red.log`, `evidence/view-restored-green.log`; hashes and baseline: `evidence/view-red-green.json`.
- `git diff --check`: clean after the View change and scoped source restoration. No tests were removed and this existing case retains its name.
- This slice runs no host clipboard tests, full suite, VM commands, or packaged Windows input qualification. Windows ARM64/x64 production loading, keyboard/pointer traversal, submenu placement, scrolling, external theme-gallery/folder launching, and ordinary packaged behavior remain pending.
- File dialogs are opened and cancelled with the test suite's native-dialog suppression; no import/settings file is accepted by this case. Existing backend theme validation is exercised by selecting local CSS through a real dynamic Windows menu delegate.
- No commit, push, package promotion, primary Mac checkout/app update, or menu-label-only parity claim is made.

Complete local logs stay in this cycle's ignored `evidence/` directory. This is a candidate record; integration with the parallel Edit/Help slice and native Windows qualification remain pending.
