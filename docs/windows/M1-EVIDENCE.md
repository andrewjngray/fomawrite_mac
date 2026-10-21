# Windows M1 implementation evidence

22 October 2026. Work in progress; no Windows executable is qualified yet.

## Changes

- Explicit macOS/Windows/Linux qmake scopes, Windows Qt appearance, bundled-only
  Harper and truthful engine availability. Windows updates remain disabled until W12.
- Windows desktop menus use the existing commands and retain the editing focus
  across actual Quick popups, including Live, Source and auxiliary fields.
- The deployed-executable checker uses a new UUID application/data namespace on
  Windows without requiring directory symlinks or administrator privileges. It
  still bypasses normal preferences migration and IPC before making any backend.
- Native ARM64 and x64 CI, pinned Qt archives, app-local runtime deployment,
  architecture/hash checks and strict Web/PDF evidence verification.
- Filesystem regressions use native temporary paths and Windows delete-sharing
  locks. Symlink fixtures are separate tests with a narrow privilege skip.

## Evidence so far

- Shared Mac application built successfully with local Qt 6.11.2 from this checkout.
- Windows menu regression failed with an actual open Format popup before the
  focus fix (Bold disabled after Source lost focus). It passed afterward.
  Expanded checks cover Source formatting/Undo, clipboard authorship MIME,
  auxiliary-field Cut/Paste without document mutation, Live Undo bridge routing,
  and Source/Live/Web/PDF menu state. Focused run: 3 passed, 0 failed, 0 skipped.
- Initial focused capability/menu run: 5 passed, 0 failed, 0 skipped on Mac.
- The first full local suite exposed two existing focus regressions after the menu
  change; it was stopped after recording both failures. A synchronous focus lookup
  corrected them. Both existing regressions and the expanded menu test then passed
  together: 5 passed, 0 failed, 0 skipped. The complete suite is being rerun.
- First Windows [CI run](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39670164799)
  at `848d17e` installed both official Qt 6.11.3 kits. Visual Studio environment
  command quoting failed before application compilation. The corrected temporary
  command-file wrapper passed a real Windows guest red/green check using a path
  containing spaces: old exit 1, new exit 0, compiler and architecture both matched.
- Deployment guards: 18 adversarial Python tests passed, including distinct Web/PDF
  evidence and source/executable provenance; PowerShell syntax checks passed.
- Editor job of [shared CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39670164700)
  passed. The Mac CI job is not yet recorded as passed.

## Independent review and open qualification

Independent reviews covered menu focus/clipboard parity, preview-check isolation,
file I/O and packaging. Findings included the menu focus defect (fixed and
regressed), a folder containment guard using the Windows separator against Qt's
slash-normalized paths (awaiting native red/green), and acceptance verification
that needed exact distinct Web/PDF scenarios (fixed with adversarial tests).

Native compilation, deployed Windows rendering, normal startup/restore, NTFS and
UNC paths, installer execution/upgrade/uninstall, DPI, and physical input remain
unqualified. Additional W09/W10 follow-up includes Windows filename/alternate
stream rejection, path case identity, and hostname-bearing file URLs. The source
still inherits Mac `0.3.0-dev49`; an internal Windows artifact is identified by
architecture, exact source commit and manifest, not by Mac release readiness.

Raw logs/screenshots remain local or in CI artifacts. Root `STATUS.md` and the
primary Mac checkout/installed bundles are unchanged.
