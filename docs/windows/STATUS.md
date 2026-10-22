# Windows workstream status

Updated: 22 October 2026. This file tracks Windows only. The root `STATUS.md` records the Mac baseline.

- Branch: `windows/port`; checkout: `/Users/andrewgray/repo/projects/fomawrite_windows`.
- Mac source baseline: `6bc58f91878e26cbc916b0355b31a63325e3ac70`.
- Plan: [Windows plan](PLAN.md); [test matrix](TEST-MATRIX.md); [candidate evidence and hashes](M1-EVIDENCE.md).
- Phase: **M1 — executable and CI complete** at the code commit below. M2 desktop behaviour and later distribution qualification remain open.
- Candidate code: **`b45cb345a9c4ff880f1f132dba936d2bb4dce9e0`**. A later documentation-only commit does not change the tested code or package identity.
- Architecture/toolchain: native ARM64 primary, separate native x64 build secondary; Qt 6.11.3, MSVC 2022 v143 `14.44.35207`, Windows SDK `10.0.26100.0`, app-local v143 CRT `14.44.35211.0`. [Target evidence](TARGET.md).
- [Windows CI 39681549835](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39681549835) is green for **both native ARM64 and x64**. Each full offscreen Qt suite passed **269 tests, 0 failures, 39 planned skips**; each native Windows appearance gate passed **3 tests, 0 failures, 0 skips**. Each deployed CI checker passed **124 scenarios, 0 failures, 0 QML warnings**, with strict exact-package verification. Actual ARM64 MSVC compilation with `/bigobj` passed after the earlier C1128 failure. Zero QML warnings refers to deployed checkers; the offscreen Qt logs contain warning messages.
- Parallels Windows 11 Enterprise ARM64 guest, OS `10.0.26200.0`, desktop session 1: **both candidate packages passed 124 scenarios, 0 failures, 0 QML warnings**. ARM64 ran natively; x64 ran under emulation. Each used a fresh disposable NTFS directory without a Qt SDK/developer PATH; copied report, fresh exit-zero receipt, exact executable path and package hashes passed independent offline verification.
- [Shared CI 39681549825](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39681549825) is green: Mac Qt 6.11.3 **295 passed, 0 failed, 13 expected skips**; editor **321 passed, 0 failed, 0 skipped**, with typecheck, asset build and committed-bundle checks passed.
- Local affected regressions: **10 passed, 0 failed, 0 skipped** (eight slots plus setup/cleanup), following a reproduced narrow-footer failure. The rebuilt isolated Mac app passed **124 scenarios, 0 failures, 0 QML warnings**.
- Mac checkout, root status and installed applications remain unchanged. No compiler or Qt SDK was installed in the Windows guest.

## Normal-launch failure and repair work

**M2 remains open; no fixed or qualified build is available.** The original `b45cb34` ARM64 package repeatedly failed to load the Live editor page from Mac-shared storage, with a QtWebEngine renderer crash also observed. The executable hash matched the M1 package. A Windows-local copy with the same executable hash completed real **File → Open File…**, **View → Source** and **File → Save** actions. This isolates an observed storage/location difference; the complete causal mechanism remains unresolved, and those local actions do not establish the full normal-launch gate.

[Diagnostic CI 39736778817](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39736778817) is green for native ARM64 and x64 at `d8b8580384821f0e3d2f3cfd573d22ccec83369e`. That candidate adds normal-startup profile isolation for acceptance testing. It is **not a repair build** and does not contain the current bounded recovery or local starter changes. It is staged separately under `dist/windows-testing/diagnostic-candidates`; it is not promoted to the testing shelf.

A second bug is now confirmed: an invisible Material scrollbar intercepted the Organizer Add button centre. Reserving its gutter passes two actual centre-click regressions (short and scrolling lists), **4 tests passed** including setup/cleanup. A diagnostic click at the old unobstructed edge passed the complete local Windows workflow and Live OCR, but the standard centre-click gate still requires the repaired package.

The current working changes bound automatic Live renderer recovery to one delayed retry, preserve canonical unsaved edits across reload, and show a visible error with an explicit retry. The focused local regression run passed **6 tests, 0 failures** with red/green evidence, and the local build passed. A manifest-verifying portable starter stages the application on Windows-local storage; its **9 non-UI fixture checks passed**. Two additional real UNC shared-folder staging/reuse checks passed. The complete local shared Qt suite passed **310 tests, 0 failures, 1 skipped**. The repaired package and full normal-launch acceptance remain pending. These local results do not qualify a packaged Windows repair.

See [M2 repair evidence](M2-REPAIR-EVIDENCE.md), [testing builds](TESTING-BUILDS.md), [portable starter](PORTABLE-STARTER.md) and [normal-launch gate](NORMAL-LAUNCH-GATE.md) for the evidence boundaries and remaining checks.

## Scope and remaining qualification

M1 covers an executable, reproducible CI and isolated deployed Source/Live/Web/PDF proof. Checker input is synthetic Qt key/pointer events; it does not qualify normal startup/restore or physical input. Windows updater commands remain disabled until W12; only bundled Harper is supported. Legacy native-checker fixture skips do not establish full Windows Review UI parity. The [skip scope](M1-EVIDENCE.md#windows-skip-scope) and [M2 menu omissions](M1-EVIDENCE.md#windows-m2-menu-limits) remain explicit.

Normal startup/restore, wider NTFS/UNC and shared-folder coverage, DPI, physical input, installer execution/upgrade/uninstall and full licensing/distribution acceptance remain open. Installer source is prepared, not built or verified. This candidate is an internal diagnostic portable build, not an internal beta or release. Its inherited `0.3.0-dev49` label is insufficient identity; use the architecture, code commit and manifest hashes.

## Next checkpoint

M2 covers the documented desktop affordances, real user workflows, file/dialog behaviour, fonts and DPI. M3/M4 cover installer, clean distribution, normal startup/restore and the broader platform matrix. Historical failures, their regressions and exact final package evidence are preserved in the [M1 record](M1-EVIDENCE.md). The M1 result does not mark these later milestones complete.
