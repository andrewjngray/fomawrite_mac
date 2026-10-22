# Windows workstream status

Updated: 22 October 2026. Windows only; the root `STATUS.md` records the Mac baseline.

## Current state — integration candidate, not promoted

**M2 qualification remains open.** Cycles 155–158 form the integrated repair checkpoint on `windows/port`, based on Focus/chrome checkpoint `ecf777d6c054446cef4635fc56d194b6e8bba4eb`. Required host builds, the focused checks below and the complete updated regression suite pass. Exact candidate ARM64/x64 CI and ordinary packaged Windows input qualification remain pending; these host results do not establish complete desktop parity.

The last package checkpoint with fully green exact Windows/shared CI is **`0e9ef4a`**. It now has a confirmed rapid Format → Bold → Ctrl+S failure: formatting succeeds, but the closing menu swallows Save. Cycle 158 fixes that cause in host red/green; its unchanged zero-gap Win32 input sequence still needs to pass on the new package before promotion. The older **`d573f16` testing shelf remains available but has a subsequently discovered rapid Live-save defect**; its historical promotion does not make it a qualified current build.

| Integration slice | Completed host evidence; counts are passed/failed/skipped |
| --- | --- |
| [155 — Go and Open Recent](../../research/cycle-155/README.md) | Required build passed; three actual-menu cases **5/0/0**, including init/cleanup, **3.250 s**. Native menu proof and packaged pointer/picker follow-through remain pending. Separate native UI Automation provider-recovery proof passed **13/13**; that is driver evidence. |
| [156 — Output Style and Live exact theme](../../research/cycle-156/README.md) | Required build passed; actual Windows component failed at the expected missing nodes with the old View block (**2/1/0**, **0.438 s**). Exact menu bytes restored; Output Style/chrome checks **4/0/0**, **5.359 s**. Includes repeated/rejected selection checks and existing guards. |
| [157 — Transformations, Substitutions and What's New](../../research/cycle-157/README.md) | Four old Windows-component rows failed at their intended missing routes. After the additions, native Mac and explicit Windows-component rows plus View/chrome controls passed **12/0/0**, **11.465 s**. |
| [158 — Save immediately after a menu command](../../research/cycle-158/README.md) | Required build passed. Settled Live saved; immediate Live failed with **zero Save shortcut activations** while the popup was closing (**2/1/0**, **2.314 s**). Removing only Windows menu exit transitions gives immediate Live/Source and existing persistence/menu checks **7/0/0**, **8.990 s**, with exact disk bytes and clean state. |

These focused counts overlap and are not a full-suite total. The complete integrated host suite passed **375/0/1**, **539.771 s**. The sole local skip is the Windows-only appearance check. Deployment/input guards passed **36 + 25**, and Live/Qt-install guards passed **25** under Python 3.14; the initial system-Python run lacked `hashlib.file_digest` and is retained separately. Native Windows component loading, nested-menu pointer/keyboard traversal, overflow, external theme/folder routes and the repaired rapid Save sequence remain required.

### Exact Focus/chrome CI checkpoint

For `ecf777d`, [Windows CI 39779416196](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39779416196) is **red on ARM64 and x64**, each **342 passed, 1 failed**. The failing unit assertion still expected the Windows chrome menu to be absent after Cycle 154 added it; the deployed checker carries the same obsolete expectation. Integration now corrects both the unit and deployed assertions and requires the actual Windows Title Bar and Output Style controls. These are retained failures, not a green candidate CI claim. [Shared CI 39779415478](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39779415478) passed **358 Mac Qt tests / 0 failures / 13 capability skips** and the editor job.

## Historical promoted repair candidate — `d573f16`

Candidate code: **`d573f1666d68efaee0b60f43f04d3fe1e96781c4`**, branch `windows/port`, checkout `/Users/andrewgray/repo/projects/fomawrite_windows`. **M2 qualification remains open.** Both CI runs, final package identities, the standard ARM64 VM normal-launch gate, a shared-document supplement, the packaged ARM64 starter and the separate 124-scenario ARM64 guest document-view diagnostic have passed. Verified packages are promoted under `dist/windows-testing/builds/d573f16-{arm64,x64}/`; the ARM64 entry is `dist/windows-testing/START-WINDOWS-ARM64.cmd`. x64 native CI and the ordinary VM workflow under ARM64 emulation also passed; no physical x64-machine test is claimed.

The candidate reserves the Organizer scrollbar gutter so invisible scrollbars cannot intercept action-button centres, bounds automatic Live renderer recovery to one delayed retry with visible failure/retry controls, preserves canonical edits during reconnect, and includes the manifest-verifying Windows-local portable starter. The [repair record](M2-REPAIR-EVIDENCE.md) separates these changes from the earlier diagnostic comparisons.

- Final local `./bin/test`: **312 passed, 0 failed, 1 skipped**, 339.8 seconds, including the Organizer fix. The earlier 310-pass run preceded that fix.
- Focused Organizer centre-click regressions: **4 passed, 0 failed, 0 skipped**, including setup/cleanup, with red/green evidence on short and scrolling lists.
- Focused Live recovery regressions: **6 passed, 0 failed**, with red/green evidence. The local app build passed.
- Portable starter: **9 non-UI fixture checks**, **2 real UNC staging/reuse checks**, and the final packaged ARM64 command-file launch passed. The packaged starter ran the verified local executable through ordinary startup with an isolated native profile and exited normally through File → Exit.
- [Windows CI 39741241624](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39741241624): **success**, ARM64 and x64. Each passed **274 Qt tests** (0 failures, 39 skips), **3 native appearance checks**, and **124 deployed scenarios** (0 failures, 0 QML warnings). Each passed **36 deployment guards + 25 normal-launch verifier tests + 9 starter checks**.
- [Shared CI 39741241651](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39741241651): **success**; **300 Mac Qt tests** (0 failures, 13 skips), **321 editor tests** (0 failures/skips), typecheck, asset build and committed-asset verification passed.
- Final ARM64 VM: the **unmodified standard centre-click normal-launch gate passed**, including native folder/file pickers, exact save, actual Live pixels/OCR, Source return, clean exits and normal restore. Strict offline verification passed. A separate run changing only the disposable writing path to shared storage also passed. Package/report hashes and the retained initial environment-blocked attempt are in the [repair record](M2-REPAIR-EVIDENCE.md).

## Last fully green CI package checkpoint — `0e9ef4a`, not promoted

`0e9ef4a71d0c43e21440d9efe0c1d1110998d6c5` repairs the confirmed selection/Save deadline defect and the three fixture-readiness failures below. Required local `./bin/build` passed and `./bin/test` passed **370/0/1**, 555.523 seconds. The one local skip is the Windows-only appearance test. Red/green and 14 focused checks are in [Live persistence](M2-LIVE-PERSISTENCE.md#delayed-selection-and-save-deadline). Exact [Windows CI 39771839535](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39771839535) passed on both architectures: each **343 Qt passes, zero failures/skips**, **3 appearance passes**, and **124 deployed scenarios** with 96 captures. Exact [shared CI 39771839462](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39771839462) passed **358 Mac Qt tests / 0 failures / 13 capability skips** plus the editor job. ARM64 package and Windows-local staging identities are verified. Ordinary ARM64 and x64-emulated normal launch/restore passed strict verification; all five ARM64 Live timing cases also passed. Publishing passed nine stages with 95 evidence hashes and eight independently reviewed captures; its nonzero Parallels wrapper status is retained. The later selected-formatting follow-through reproduced a separate dropped Save shortcut during menu exit; Cycle 158's host fix still requires exact-package Windows verification. Clipboard and nested-View supplements remain rejected and open. [Exact desktop checkpoint](M2-DESKTOP-0E9.md), [Save repair](../../research/cycle-158/README.md). No package promotion is claimed.

## Previous candidate — `7c37078`, not promoted

`7c37078b0d135f78b68f412444083ae7f919b339` implements the View and clipboard/search menu routes. Its local required build/full suite passed **367/0/1**. Exact Windows CI `39766241460` passed ARM64 **340/0/0** but failed x64 **339/1/0**: immediate formatting after Save As was absent from the saved copy. This exposed a confirmed selection/Save deadline defect: a one-second selection fallback failed formatting despite the existing three-second persistence budget. The fix passes 14 focused checks after a real-page red reproduction; complete local/native reruns remain required. [Repair evidence](M2-LIVE-PERSISTENCE.md#delayed-selection-and-save-deadline). Both architectures passed appearance and deployed diagnostics. Shared CI `39766241338` passed editor checks but failed Mac Qt **352/3/13**. Three test-readiness corrections subsequently passed **29/0/0** locally; native reruns remain required. [Detailed evidence](M2-MENU-PARITY.md).

The exact ARM64 package is verified and staged only for ordinary VM diagnostics. Neither this candidate nor the earlier red-CI `6558d35` package is the recommended shelf build.

## Previous verification checkpoint — not promoted

Code `6558d3505b8cff0284f3dc3cc0e1c6b92a1336fc` contains the Live persistence barrier, portable output-font fallbacks and pinned Qt mirror recovery. **CI is red; do not promote this package.** [Complete checkpoint, hashes and retained failures](M2-DESKTOP-6558.md).

| Check | Exact result |
| --- | --- |
| Local parent build/test | `bb62989`: build passed; 351 Qt passes, zero failures, one Windows-only skip. |
| Native Windows CI `39760440078` | ARM64: 323 passed / 0 failed / 1 skipped. x64: 321 / 2 / 1. Both: 3 appearance passes and 124 deployed scenarios without failures. Overall failed. |
| Shared CI `39760440072` | Mac Qt: 338 passed / 1 failed / 13 skipped. Editor tests and committed-bundle verification passed. Overall failed. |
| Ordinary ARM64 VM | Six-stage normal launch and independent verifier passed. All five maintained Live timing cases passed, including immediate Undo→Save. |
| Ordinary ARM64 publishing | Nine stages passed; all eight captures independently reviewed. Correct document identity, Web/PDF, native exports and sans/serif distinction. |
| Ordinary x64 VM, emulated | Initial second-launch Exit accessibility lookup failed. One repeat with exception diagnostics passed; the first failure remains unexplained and retained. |
| Updater desktop | Final ordinary ARM64 update/install/rollback/relaunch passed; 36 evidence hashes and four engine captures independently verified. Earlier observation failures are retained. Native failure/rollback regressions also passed at the earlier `2d84cc4` checkpoint. |

The shared Mac/x64 footer clean-state assertion and x64 stale-page delivery witness were corrected in the subsequent `7c37078` checkpoint; its separate failures are recorded above. The initial ARM64 imported-theme fixture-write failure at `d145a0f` has not recurred; its root cause is still unproven. Original Qt checksum pins are unchanged; official mirror fallback passed 12 new checks plus 74 existing deployment/input guards.

## Current work and next gates

- Run exact ARM64/x64/shared CI for the completed integration, then verify Cycle 158's immediate selected Bold → Ctrl+S using the unchanged native input sequence. The prior `0e9ef4a` green CI and Undo → Save passes do not qualify this newly reproduced menu-closing defect. Preserve exact bytes, clean state and ordinary restore; package promotion remains pending.
- The existing View-menu slice exposes existing sorting, date, excerpts and navigation controls plus Paginated Preview. The former portable menu-state skip is replaced with actual Windows menu tests. The combined View/clipboard/search candidate `7c37078b0d135f78b68f412444083ae7f919b339` passed the required local build and complete suite: **367 tests, zero failures, one Windows-only skip**, 540.931 seconds. The exact native/shared CI failures are recorded above; final ordinary desktop menu testing remains incomplete. [Menu evidence](M2-MENU-PARITY.md).
- The ordinary updater workflow is complete on `6558d35`; repeat affected desktop acceptance on the final package. Preserve exact UTF-8 bytes, undo/recovery, correct preview identity and the rapid Live-save regression. [Live repair](M2-LIVE-PERSISTENCE.md), [maintained Live driver](LIVE-EDIT-ACCEPTANCE.md), [publishing driver](PUBLISHING-ACCEPTANCE.md), [updater evidence](M2-UPDATER-PARITY.md).
- Resolve or explicitly document portable UI gaps from the [parity ledger](PARITY-LEDGER.md). Mac-only Cocoa/NSSpellChecker exclusions must not hide portable behavior. Passing Backend tests do not establish an absent menu route.
- Complete the open clipboard and nested-View supplements and remaining portable routes in the [desktop checkpoint](M2-DESKTOP-0E9.md) and [parity ledger](PARITY-LEDGER.md). Then promote and sync only final verified packages with exact architecture, commit and manifest identities. The d573 shelf is not a qualified beta/release.

The primary Mac checkout, root status and installed apps remain untouched. Generated captures are restored after local test runs; no reference image has been changed to obtain a pass. Historical [checker evidence](M2-CHECKER-PARITY.md), [repair evidence](M2-REPAIR-EVIDENCE.md) and failed attempts remain available.

## What the diagnosis established

The original `b45cb34` ARM64 package repeatedly failed to load the Live editor page from Mac-shared storage; a QtWebEngine renderer crash was observed. The same executable hash from Windows-local storage completed native File Open, View Source and File Save. The underlying shared-storage renderer mechanism remains unresolved.

A separate Organizer defect was confirmed by hit testing: an invisible Material scrollbar covered the Add Library Location button centre. An unobstructed-edge diagnostic click completed the local workflow and Live OCR, and remains a historical probe. The final repair subsequently passed the unchanged standard centre-click gate.

The `d8b8580` diagnostic candidate added normal-startup profile isolation only. Its [ARM64/x64 CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39736778817) passed, and its ARM64 artifact was strictly verified. It remains separate diagnostic evidence, not the current repair candidate or an accepted shelf build.

## Historical M1 evidence

M1 code `b45cb34` completed executable and bounded diagnostic acceptance. [Windows CI 39681549835](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39681549835) passed on ARM64 and x64: each offscreen Qt suite had **269 passed, 0 failed, 39 planned skips**; each native appearance gate had **3 passed, 0 failed, 0 skipped**; each deployed checker passed **124 scenarios, 0 failures, 0 QML warnings**. Both packages also passed 124 checker scenarios in the Parallels Windows 11 ARM64 guest, with exact executable/package/receipt verification; x64 ran under emulation.

[Shared CI 39681549825](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39681549825) passed **295 Mac Qt tests** and **321 editor tests**, with zero failures and 13 expected Qt skips. Earlier affected local regressions passed 10 tests, and the isolated Mac app passed 124 scenarios. These historical results are retained in [M1 evidence](M1-EVIDENCE.md); they do not establish ordinary startup or Windows input.

Toolchain baseline: Qt 6.11.3, MSVC 2022 v143 `14.44.35207`, Windows SDK `10.0.26100.0`, app-local v143 CRT `14.44.35211.0`; native ARM64 primary and separate native x64 secondary. [Target evidence](TARGET.md). Mac source baseline: `6bc58f91878e26cbc916b0355b31a63325e3ac70`. Mac checkout, root status and installed apps remain unchanged; no compiler or Qt SDK was installed in the Windows guest.

## Remaining scope

M1's synthetic Qt input does not qualify ordinary startup/restore or hardware input. Wider profiles, storage paths, DPI, desktop parity, installer execution/upgrade/uninstall and distribution/licensing acceptance remain open. The shelf package still uses bundled Harper; ordinary ARM64 updater qualification passed on `6558d35`; exact `0e9ef4a` CI is green, but its newly reproduced menu-closing Save failure and later integration changes require their own exact package verification. [Skip scope](M1-EVIDENCE.md#windows-skip-scope), [M2 menu limits](M1-EVIDENCE.md#windows-m2-menu-limits), [plan](PLAN.md) and [test matrix](TEST-MATRIX.md).

This is an internal repair candidate, not a qualified beta or release. Its inherited `0.3.0-dev49` label is insufficient identity; use architecture, commit and manifest hashes. See [testing builds](TESTING-BUILDS.md) and [portable starter](PORTABLE-STARTER.md).
