# Windows M1 implementation evidence

22 October 2026. Work in progress; native ARM64 and x64 compile, but neither executable is qualified yet.

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
  together: 5 passed, 0 failed, 0 skipped. Complete local Mac suite at `324d5da`:
  **306 passed, 0 failed, 1 skipped** (Windows appearance only), 330.7 seconds.
- First Windows [CI run](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39670164799)
  at `848d17e` installed both official Qt 6.11.3 kits. Visual Studio environment
  command quoting failed before application compilation. The corrected temporary
  command-file wrapper passed a real Windows guest red/green check using a path
  containing spaces: old exit 1, new exit 0, compiler and architecture both matched.
- Deployment guards: 29 adversarial Python tests passed, including distinct Web/PDF
  evidence, source/executable provenance and copied guest report identity; PowerShell
  syntax checks passed. A PowerShell 5.1 guest runner requires no Python or Qt SDK.
- Both native app builds in [second Windows CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39671294627)
  compiled at `324d5da`. ARM64 used MSVC v143 `14.44.35207` and Windows SDK
  `10.0.26100.0`. The deployed x64 executable passed all eight strict Web/PDF
  document-identity and paint scenarios. Overall acceptance failed on inherited
  native-Mac-menu assumptions, constrained-display submenu direction, and a fixed
  compact-row height. The Windows Qt suite failed to compile because its `far`
  variable collided with a Windows macro; renamed to `farBlock`. ARM64 deployment
  failed because the runner supplied a newer runtime directory to the v143
  compiler environment. Reviewed runtime discovery/selection now validates
  actual DLL versions, native architecture and hashes; native rerun remains needed.
- The same deployed x64 ZIP was copied to disposable NTFS storage in the ARM64
  Parallels guest and run under x64 emulation, without a Qt SDK or developer PATH.
  All eight Web/PDF paint/identity scenarios passed, with no QML warnings. Overall
  acceptance remained red on menu and compact-row checks; this is diagnostic
  evidence, not native ARM64 qualification. ZIP SHA-256:
  `42a7d985fa244bc5a475e44f5ccdf8fbed7e782596cf663a8020d4c90b21a14b`.
  Executable SHA-256:
  `5ab85d34c4ecf361cf129285385e509c2b46c8b1e55b4f0c989b01431268256b`.
  The copied report and process receipt were checked against the manifest.
- Test-portability focused run: 25 passed, 0 failed, 0 skipped on Mac. The renamed
  spelling test separately passed (3/0/0 including setup/cleanup). Compact-row and
  chrome regressions passed together (6/0/0). Compact rows retain explicit font,
  line-count, padding, alignment and containment bounds using actual QML metrics;
  the former fixed total height was not portable across native fonts. Header hover
  tests now target the actual header below the Windows menu bar. Cascading menus
  must remain inside the window and may open on either non-overlapping side.
  The full Mac deployed-app checker is still running.
- Editor job of [integrated shared CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39671295502)
  passed. The Mac CI job is still running.

## Windows M2 menu limits

The initial Windows menu exposes basic File/Edit/Format/View/Help commands. It
omits the Output Style submenu, additional structure/format insertion items,
Find Next/Previous/Selection/Delete, copy/paste variants, transformations,
completion/smart punctuation/focus/review toggles and document-bar mode controls.
Shared behavior remains tested through existing common commands or the footer;
those checks do not claim the omitted desktop affordances exist. macOS Versions,
Share and Speech are intentionally absent as platform capabilities.

## Independent review and open qualification

Independent reviews covered menu focus/clipboard parity, preview-check isolation,
file I/O and packaging. Findings included the menu focus defect (fixed and
regressed), a folder containment guard using the Windows separator against Qt's
slash-normalized paths (awaiting native red/green), and acceptance verification
that needed exact distinct Web/PDF scenarios (fixed with adversarial tests).

Deployed Windows rendering, normal startup/restore, NTFS and
UNC paths, installer execution/upgrade/uninstall, DPI, and physical input remain
unqualified. Additional W09/W10 follow-up includes Windows filename/alternate
stream rejection, path case identity, and hostname-bearing file URLs. The source
still inherits Mac `0.3.0-dev49`; an internal Windows artifact is identified by
architecture, exact source commit and manifest, not by Mac release readiness.

Raw logs/screenshots remain local or in CI artifacts. Root `STATUS.md` and the
primary Mac checkout/installed bundles are unchanged.
