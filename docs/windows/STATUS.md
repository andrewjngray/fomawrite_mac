# Windows workstream status

Updated: 22 October 2026. Windows only; the root `STATUS.md` records the Mac baseline.

## Current repair candidate

Candidate code: **`d573f1666d68efaee0b60f43f04d3fe1e96781c4`**, branch `windows/port`, checkout `/Users/andrewgray/repo/projects/fomawrite_windows`. **M2 qualification remains open.** Both CI runs, final package identities, the standard ARM64 VM normal-launch gate, a shared-document supplement, the packaged ARM64 starter and the separate 124-scenario ARM64 guest document-view diagnostic have passed. Verified packages are promoted under `dist/windows-testing/builds/d573f16-{arm64,x64}/`; the ARM64 entry is `dist/windows-testing/START-WINDOWS-ARM64.cmd`. x64 has CI verification only, with no final VM normal-launch run.

The candidate reserves the Organizer scrollbar gutter so invisible scrollbars cannot intercept action-button centres, bounds automatic Live renderer recovery to one delayed retry with visible failure/retry controls, preserves canonical edits during reconnect, and includes the manifest-verifying Windows-local portable starter. The [repair record](M2-REPAIR-EVIDENCE.md) separates these changes from the earlier diagnostic comparisons.

- Final local `./bin/test`: **312 passed, 0 failed, 1 skipped**, 339.8 seconds, including the Organizer fix. The earlier 310-pass run preceded that fix.
- Focused Organizer centre-click regressions: **4 passed, 0 failed, 0 skipped**, including setup/cleanup, with red/green evidence on short and scrolling lists.
- Focused Live recovery regressions: **6 passed, 0 failed**, with red/green evidence. The local app build passed.
- Portable starter: **9 non-UI fixture checks**, **2 real UNC staging/reuse checks**, and the final packaged ARM64 command-file launch passed. The packaged starter ran the verified local executable through ordinary startup with an isolated native profile and exited normally through File → Exit.
- [Windows CI 39741241624](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39741241624): **success**, ARM64 and x64. Each passed **274 Qt tests** (0 failures, 39 skips), **3 native appearance checks**, and **124 deployed scenarios** (0 failures, 0 QML warnings). Each passed **36 deployment guards + 25 normal-launch verifier tests + 9 starter checks**.
- [Shared CI 39741241651](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39741241651): **success**; **300 Mac Qt tests** (0 failures, 13 skips), **321 editor tests** (0 failures/skips), typecheck, asset build and committed-asset verification passed.
- Final ARM64 VM: the **unmodified standard centre-click normal-launch gate passed**, including native folder/file pickers, exact save, actual Live pixels/OCR, Source return, clean exits and normal restore. Strict offline verification passed. A separate run changing only the disposable writing path to shared storage also passed. Package/report hashes and the retained initial environment-blocked attempt are in the [repair record](M2-REPAIR-EVIDENCE.md).

## Next stability cycle

- Port applicable Mac regression fixtures onto the bundled Harper backend and Windows menu objects. Audit the 39 Windows skips individually; do not count skips as passes.
- Extend actual desktop acceptance beyond the completed ARM64 workflow to the remaining capabilities in the test matrix.
- Final x64 VM normal-launch acceptance remains untested; x64 CI is green.

The separate ARM64 guest diagnostic passed **124 scenarios, 0 failures, 0 QML warnings**, with exact package/receipt verification. The original ZIPs, extracted applications and copy receipts are in the versioned testing folders; older packages remain preserved.

The first standard ARM64 attempt stopped before workflow input because an older instance's dialog blocked foreground activation. That attempt is retained. After Andrew authorized cancel/normal close and the old process was confirmed absent, a fresh run of the unchanged gate passed. [Exact evidence](M2-REPAIR-EVIDENCE.md#final-arm64-vm-normal-launch-evidence).

## What the diagnosis established

The original `b45cb34` ARM64 package repeatedly failed to load the Live editor page from Mac-shared storage; a QtWebEngine renderer crash was observed. The same executable hash from Windows-local storage completed native File Open, View Source and File Save. The underlying shared-storage renderer mechanism remains unresolved.

A separate Organizer defect was confirmed by hit testing: an invisible Material scrollbar covered the Add Library Location button centre. An unobstructed-edge diagnostic click completed the local workflow and Live OCR, and remains a historical probe. The final repair subsequently passed the unchanged standard centre-click gate.

The `d8b8580` diagnostic candidate added normal-startup profile isolation only. Its [ARM64/x64 CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39736778817) passed, and its ARM64 artifact was strictly verified. It remains separate diagnostic evidence, not the current repair candidate or an accepted shelf build.

## Historical M1 evidence

M1 code `b45cb34` completed executable and bounded diagnostic acceptance. [Windows CI 39681549835](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39681549835) passed on ARM64 and x64: each offscreen Qt suite had **269 passed, 0 failed, 39 planned skips**; each native appearance gate had **3 passed, 0 failed, 0 skipped**; each deployed checker passed **124 scenarios, 0 failures, 0 QML warnings**. Both packages also passed 124 checker scenarios in the Parallels Windows 11 ARM64 guest, with exact executable/package/receipt verification; x64 ran under emulation.

[Shared CI 39681549825](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39681549825) passed **295 Mac Qt tests** and **321 editor tests**, with zero failures and 13 expected Qt skips. Earlier affected local regressions passed 10 tests, and the isolated Mac app passed 124 scenarios. These historical results are retained in [M1 evidence](M1-EVIDENCE.md); they do not establish ordinary startup or Windows input.

Toolchain baseline: Qt 6.11.3, MSVC 2022 v143 `14.44.35207`, Windows SDK `10.0.26100.0`, app-local v143 CRT `14.44.35211.0`; native ARM64 primary and separate native x64 secondary. [Target evidence](TARGET.md). Mac source baseline: `6bc58f91878e26cbc916b0355b31a63325e3ac70`. Mac checkout, root status and installed apps remain unchanged; no compiler or Qt SDK was installed in the Windows guest.

## Remaining scope

M1's synthetic Qt input does not qualify ordinary startup/restore or hardware input. Wider profiles, storage paths, DPI, desktop parity, installer execution/upgrade/uninstall and distribution/licensing acceptance remain open. Windows updater commands stay disabled until W12; bundled Harper is supported. [Skip scope](M1-EVIDENCE.md#windows-skip-scope), [M2 menu limits](M1-EVIDENCE.md#windows-m2-menu-limits), [plan](PLAN.md) and [test matrix](TEST-MATRIX.md).

This is an internal repair candidate, not a qualified beta or release. Its inherited `0.3.0-dev49` label is insufficient identity; use architecture, commit and manifest hashes. See [testing builds](TESTING-BUILDS.md) and [portable starter](PORTABLE-STARTER.md).
