# Windows workstream status

Updated: 22 October 2026. Windows only; the root `STATUS.md` records the Mac baseline.

## Current repair candidate

Candidate code: **`d573f1666d68efaee0b60f43f04d3fe1e96781c4`**, branch `windows/port`, checkout `/Users/andrewgray/repo/projects/fomawrite_windows`. **M2 qualification remains open.** Both CI runs, final package identities, the standard ARM64 VM normal-launch gate, a shared-document supplement, the packaged ARM64 starter and the separate 124-scenario ARM64 guest document-view diagnostic have passed. Verified packages are promoted under `dist/windows-testing/builds/d573f16-{arm64,x64}/`; the ARM64 entry is `dist/windows-testing/START-WINDOWS-ARM64.cmd`. x64 native CI and the ordinary VM workflow under ARM64 emulation also passed; no physical x64-machine test is claimed.

The candidate reserves the Organizer scrollbar gutter so invisible scrollbars cannot intercept action-button centres, bounds automatic Live renderer recovery to one delayed retry with visible failure/retry controls, preserves canonical edits during reconnect, and includes the manifest-verifying Windows-local portable starter. The [repair record](M2-REPAIR-EVIDENCE.md) separates these changes from the earlier diagnostic comparisons.

- Final local `./bin/test`: **312 passed, 0 failed, 1 skipped**, 339.8 seconds, including the Organizer fix. The earlier 310-pass run preceded that fix.
- Focused Organizer centre-click regressions: **4 passed, 0 failed, 0 skipped**, including setup/cleanup, with red/green evidence on short and scrolling lists.
- Focused Live recovery regressions: **6 passed, 0 failed**, with red/green evidence. The local app build passed.
- Portable starter: **9 non-UI fixture checks**, **2 real UNC staging/reuse checks**, and the final packaged ARM64 command-file launch passed. The packaged starter ran the verified local executable through ordinary startup with an isolated native profile and exited normally through File → Exit.
- [Windows CI 39741241624](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39741241624): **success**, ARM64 and x64. Each passed **274 Qt tests** (0 failures, 39 skips), **3 native appearance checks**, and **124 deployed scenarios** (0 failures, 0 QML warnings). Each passed **36 deployment guards + 25 normal-launch verifier tests + 9 starter checks**.
- [Shared CI 39741241651](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39741241651): **success**; **300 Mac Qt tests** (0 failures, 13 skips), **321 editor tests** (0 failures/skips), typecheck, asset build and committed-asset verification passed.
- Final ARM64 VM: the **unmodified standard centre-click normal-launch gate passed**, including native folder/file pickers, exact save, actual Live pixels/OCR, Source return, clean exits and normal restore. Strict offline verification passed. A separate run changing only the disposable writing path to shared storage also passed. Package/report hashes and the retained initial environment-blocked attempt are in the [repair record](M2-REPAIR-EVIDENCE.md).

## Checker parity cycle in verification

Twenty-eight existing Source/Live spelling, grammar and Review cases now run against real bundled Harper in addition to native Mac rows. They exposed and now cover a nested-list prose bug caused by parsing already-masked prose as Markdown again. Candidate code `d145a0f` passes the full local build and Qt suite (340 passed, 0 failed, one Windows-only skip), focused checks and 324 editor tests. Shared CI passed 328 Mac Qt and 324 editor tests. Windows x64 passed 302 Qt tests; ARM64 passed 301 and failed one imported-theme CSS fixture write, with 11 skips on each architecture. Both deployed diagnostics passed 124 scenarios. The theme-write failure is under investigation; new package qualification is pending. [Evidence and remaining gaps](M2-CHECKER-PARITY.md). The shelf candidate above remains the last qualified package.

## Live persistence and typography candidate

The pending repair waits for acknowledged Live text before Save, Save As, close or navigation; it rejects stale document/page replies and preserves newer editor/layout choices. Modern Sans and other built-in styles now have portable font fallbacks without changing stored font settings. Focused integration passed **19 tests, zero failures/skips**, plus typography regressions. The required complete build/test repeat for `bb62989ac053616b870499422b586447e2e9cb27` passed **351 tests, zero failures, one Windows-only skip** in 451.122 seconds. The initial nine failures and their corrections remain recorded. [Repair evidence](M2-LIVE-PERSISTENCE.md).

The maintained actual-input driver reproduces rapid Undo→Save failure on d573 and passes its settled baseline. Final repaired packages require all five timing cases, publishing/export repetitions and independent receipts. Its **74 Python guards, 11 native UIA/readiness checks and three folder-readiness checks** passed. No new package has been promoted. Windows CI `39759214639` stopped before compilation because the primary Qt repository removed the pinned archive revision (HTTP404); its deployment guards passed. The reviewed mirror fallback retains the original checksums; its 12 focused tests plus the existing 74 guards passed. Native builds are being repeated.

Updater checkpoint `2d84cc4` passed all updater tests on both native architectures, including actual engine install/rollback, plus three appearance cases and 124 packaged diagnostic scenarios each. Each complete Windows run was 315 passed/one failure/one skip; Mac was 331/one/13. Their only failure was an obsolete assertion that updater Help commands must be absent; its corrected form passed the focused integration run. The earlier theme-write failure did not recur, and its cause remains unexplained. Final full CI remains required.

## Next stability cycle

- Resolve the ARM64 theme-fixture write failure. Twenty-eight checker skips have been replaced with actual Harper cases; the ten updater cases now have a Windows implementation and focused host coverage, pending native CI. [Updater evidence](M2-UPDATER-PARITY.md). The remaining native Mac menu exclusion is platform-specific.
- Extend actual desktop acceptance beyond the completed ARM64 workflow. The [ordinary publishing supplement](PUBLISHING-ACCEPTANCE.md) passed document identity, Web/PDF and native exports on d573; visual review exposed a Modern Sans fallback defect, now under repair. Repeat on the final candidate. Settled Live typing/save/undo/redo and reopening passed on d573. Rapid Undo→Save reproducibly saved the earlier text while the editor remained dirty; the reviewed persistence barrier passes 14 focused checks and is now in full integration testing. [Live repair evidence](M2-LIVE-PERSISTENCE.md).
- Both architectures now pass the ordinary VM workflow. Broaden the coverage beyond this six-stage gate; physical x64 hardware remains untested.

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

M1's synthetic Qt input does not qualify ordinary startup/restore or hardware input. Wider profiles, storage paths, DPI, desktop parity, installer execution/upgrade/uninstall and distribution/licensing acceptance remain open. The shelf package still uses bundled Harper; the new updater candidate is awaiting Windows qualification. [Skip scope](M1-EVIDENCE.md#windows-skip-scope), [M2 menu limits](M1-EVIDENCE.md#windows-m2-menu-limits), [plan](PLAN.md) and [test matrix](TEST-MATRIX.md).

This is an internal repair candidate, not a qualified beta or release. Its inherited `0.3.0-dev49` label is insufficient identity; use architecture, commit and manifest hashes. See [testing builds](TESTING-BUILDS.md) and [portable starter](PORTABLE-STARTER.md).
