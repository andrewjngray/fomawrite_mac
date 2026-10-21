# Windows workstream status

Updated: 22 October 2026. This file tracks Windows only. The root `STATUS.md` records the Mac baseline.

- Branch: `windows/port`.
- Checkout: `/Users/andrewgray/repo/projects/fomawrite_windows`.
- Mac source baseline: `6bc58f91878e26cbc916b0355b31a63325e3ac70`.
- Plan: [project and Windows plan](PLAN.md); [test matrix](TEST-MATRIX.md).
- Phase: **M0 — feasibility and isolated setup**.
- Windows executable / installer: **not built or verified yet**.
- Windows CI: **planned; not added yet**.
- Host inspection: Parallels Windows 11 guest is running; guest reports ARM64, Windows 11 Enterprise, OS `10.0.26200`. Git is on PATH. Other toolchain locations are not yet inventoried.
- Architecture: **native ARM64 primary, x64 secondary**. Official Qt 6.11.3 packages include WebEngine and PDF for both targets; [verified package metadata](TARGET.md). Native runtime is not yet verified.
- Mac branch and installed applications: unchanged by this setup.
- Orchestration: separate GPT-6 Astra / Ultra chat to own implementation and delegate bounded tasks to less expensive workers.

## Next checkpoint

Select a supported kit, implement the minimum Windows platform/CI changes, and prove Source, Live, Web and PDF in a deployed Windows executable. Report actual tests and artifact identity before advancing readiness.
