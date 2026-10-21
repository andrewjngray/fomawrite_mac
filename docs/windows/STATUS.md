# Windows workstream status

Updated: 22 October 2026. This file tracks Windows only. The root `STATUS.md` records the Mac baseline.

- Branch: `windows/port`.
- Checkout: `/Users/andrewgray/repo/projects/fomawrite_windows`.
- Mac source baseline: `6bc58f91878e26cbc916b0355b31a63325e3ac70`.
- Plan: [project and Windows plan](PLAN.md); [test matrix](TEST-MATRIX.md).
- Phase: **M1 — implementation and native CI proof in progress**.
- Windows executable: **native ARM64 packaging and isolated deployed checks pass at `3095601`; full Qt qualification pending**. ARM64 and x64 compiled. Installer: source prepared, not built or verified.
- Windows CI: [candidate `3095601`](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39678109253) compiled and strictly packaged ARM64 (119 native PE files; v143 CRT `14.44.35211.0` with compiler `14.44.35207`). Native CI deployed acceptance passed. The same package passed **124 scenarios, 0 failures, 0 QML warnings** in the native ARM64 Windows 11 guest; copied report, fresh exit-0 receipt, exact executable path and hashes verified offline. The ARM64 job is still red: MSVC C1128 stopped Qt test compilation before either Qt gate ran. The bounded Windows/MSVC `/bigobj` fix awaits native rerun. Native x64 appearance passed (3/0/0 including setup/cleanup), and deployed CI acceptance passed 124 scenarios. Its full offscreen Qt suite reported **261 passed, 8 failed, 39 skipped** in 517.536 seconds; bounded corrections passed the eight-slot local focused run (10/0/0 including setup/cleanup), but native confirmation remains pending. Shared Mac/editor CI is green. No qualified Windows artifact or M1 completion claim.
- Host inspection: Parallels Windows 11 guest is running; guest reports ARM64, Windows 11 Enterprise, OS `10.0.26200`. Git is on PATH. No compiler/Qt kit was found in standard locations. CI supplies builds; no guest toolchain was installed.
- Architecture: **native ARM64 primary, x64 secondary**. Official Qt 6.11.3 packages include WebEngine and PDF for both targets; [verified package metadata](TARGET.md). Native ARM64 app-local runtime deployment is verified for the isolated checker at `3095601`; broader qualification remains open.
- Mac branch and installed applications: unchanged by this setup.
- Orchestration: separate GPT-6 Astra / Ultra chat to own implementation and delegate bounded tasks to less expensive workers.

Implementation/evidence: [M1 record](M1-EVIDENCE.md). Prior full local Mac Qt suite: 306 passed, 0 failed, 1 Windows-only skip. [Shared CI at `3095601`](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39678109413) is green: Mac 295 passed, 0 failed, 13 expected skips (12 unavailable runner-native grammar checks and Windows appearance); editor 321 passed, 0 failed, 0 skipped, with typecheck/asset-build/committed-bundle checks passed. Windows testing now requires both native Windows appearance transitions and the full offscreen suite; the offscreen appearance case checks the real unknown-scheme palette fallback. Neither substitutes for deployed acceptance.

Latest corrections: footer controls preserve full labels and minimum hit widths under font pressure; per-instance Omarchy input isolates the fixture while preserving default production loading/watching; export bounds use the actual parent; the Live fixture verifies real selection and request/reply behavior. Focused local run: **10 passed, 0 failed, 0 skipped** (eight slots plus setup/cleanup), following a reproduced footer failure. The app rebuilt successfully; its complete checker is still running. These corrections and `/bigobj` await native CI.

## Next checkpoint

Rerun native ARM64 Qt compilation with the bounded `/bigobj` correction, then require both the native Windows appearance gate and complete offscreen suite. Confirm the bounded x64 offscreen-suite corrections in a complete green native rerun, then refresh package checks and shared CI. Preserve the passing exact-source ARM64 package and CI/guest Source/Live/Web/PDF evidence; complete the remaining qualification before advancing readiness. The M2 desktop-menu omissions are enumerated in the [M1 record](M1-EVIDENCE.md); shared command/dialog tests do not establish those missing affordances.
