# Windows workstream status

Updated: 22 October 2026. This file tracks Windows only. The root `STATUS.md` records the Mac baseline.

- Branch: `windows/port`.
- Checkout: `/Users/andrewgray/repo/projects/fomawrite_windows`.
- Mac source baseline: `6bc58f91878e26cbc916b0355b31a63325e3ac70`.
- Plan: [project and Windows plan](PLAN.md); [test matrix](TEST-MATRIX.md).
- Phase: **M1 — implementation and native CI proof in progress**.
- Windows executable / installer: **not built or verified yet**.
- Windows CI: added for `windows/port` pushes and manual runs; native ARM64 and x64 jobs both installed the pinned Qt SDK. First run failed before app compilation at compiler-environment command quoting; the correction has passed a guest red/green check and awaits native CI.
- Host inspection: Parallels Windows 11 guest is running; guest reports ARM64, Windows 11 Enterprise, OS `10.0.26200`. Git is on PATH. No compiler/Qt kit was found in standard locations. CI supplies builds; no guest toolchain was installed.
- Architecture: **native ARM64 primary, x64 secondary**. Official Qt 6.11.3 packages include WebEngine and PDF for both targets; [verified package metadata](TARGET.md). Native runtime is not yet verified.
- Mac branch and installed applications: unchanged by this setup.
- Orchestration: separate GPT-6 Astra / Ultra chat to own implementation and delegate bounded tasks to less expensive workers.

Implementation/evidence: [M1 record](M1-EVIDENCE.md). Shared Mac application builds; focused menu regressions passed after an observed red result. Full integration and native Windows qualification remain in progress.

## Next checkpoint

Run the integrated native builds and prove Source, Live, Web and PDF in a deployed Windows executable. Report actual tests and artifact identity before advancing readiness.
