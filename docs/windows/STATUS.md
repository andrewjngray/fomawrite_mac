# Windows workstream status

Updated: 22 October 2026. This file tracks Windows only. The root `STATUS.md` records the Mac baseline.

- Branch: `windows/port`.
- Checkout: `/Users/andrewgray/repo/projects/fomawrite_windows`.
- Mac source baseline: `6bc58f91878e26cbc916b0355b31a63325e3ac70`.
- Plan: [project and Windows plan](PLAN.md); [test matrix](TEST-MATRIX.md).
- Phase: **M1 — implementation and native CI proof in progress**.
- Windows executable: **ARM64 and x64 compiled; runtime qualification pending**. Installer: source prepared, not built or verified.
- Windows CI: both native apps compiled at `6eeca40`; x64 packaged and its complete offscreen Qt suite ran: **242 passed, 26 failed, 39 skipped**. ARM64 packaging rejected an x64 sidecar in the native runtime folder. The refreshed x64 guest checker passed all eight Web/PDF paint/identity scenarios and compact rows, but failed document-header hover checks. Bounded runtime, I/O, layout and test-portability fixes await native rerun. No qualified Windows artifact yet.
- Host inspection: Parallels Windows 11 guest is running; guest reports ARM64, Windows 11 Enterprise, OS `10.0.26200`. Git is on PATH. No compiler/Qt kit was found in standard locations. CI supplies builds; no guest toolchain was installed.
- Architecture: **native ARM64 primary, x64 secondary**. Official Qt 6.11.3 packages include WebEngine and PDF for both targets; [verified package metadata](TARGET.md). Native runtime is not yet verified.
- Mac branch and installed applications: unchanged by this setup.
- Orchestration: separate GPT-6 Astra / Ultra chat to own implementation and delegate bounded tasks to less expensive workers.

Implementation/evidence: [M1 record](M1-EVIDENCE.md). Prior full local Mac Qt suite: 306 passed, 0 failed, 1 Windows-only skip. Shared CI at `6eeca40` is green: Mac 294 passed, 0 failed, 13 justified skips; editor tests/typecheck/bundle checks passed. Latest focused fixes are still being verified. Windows testing now requires both native Windows appearance transitions and the full offscreen suite; the offscreen appearance case checks the real unknown-scheme palette fallback. Neither substitutes for deployed acceptance.

## Next checkpoint

Rerun the integrated native builds, both Windows Qt gates and the deployed checker after the bounded fixes. Confirm ARM64 runtime deployment, exact-source artifacts and Source/Live/Web/PDF behavior before advancing readiness. The M2 desktop-menu omissions are enumerated in the [M1 record](M1-EVIDENCE.md); shared command/dialog tests do not establish those missing affordances.
