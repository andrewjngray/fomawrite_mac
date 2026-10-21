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
- Deployment guards: 36 adversarial Python tests passed, including distinct Web/PDF
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
- Third native CI at `6eeca40` again compiled both apps and packaged x64. ARM64
  runtime inventory showed native v143 DLLs at version `14.44.35211.0`, plus an
  x64 `vcruntime140_1.dll` inside the ARM64 redistributable folder. The strict
  architecture guard rejected that mixed folder. The corrected selector inspects
  normal and delay imports from every packaged PE and every native runtime DLL,
  excludes an unneeded foreign DLL only when no deployed/native runtime imports it,
  and rejects missing or foreign required dependencies. Selected DLL versions,
  hashes and architecture guards remain mandatory. The actual-shape regression
  failed before and passed after this correction; native ARM64 deployment is pending.
  Qt regression jobs now run after a successful app build even if packaging fails.
- The x64 Qt suite at `6eeca40` compiled and ran on the native Windows CI
  runner with Qt's offscreen platform: **242 passed, 26 failed, 39 skipped**
  in 517.4 seconds. Failures exposed the folder-descendant guard, autosave's
  self-held read lock, preserved-timestamp theme refresh, desktop-menu/header
  overlap, footer label clipping and Omawrite glyph-metric assumptions, plus
  inherited Mac-only menu/appearance test assumptions and a Copy revision
  baseline captured before Find highlighting. The 39 reported skips cover
  macOS-only system-checker/native-menu cases and explicitly disabled Harper
  update paths; they do not convert the failing suite into a pass.
- Reviewed bounded fixes now await native rerun. Folder containment uses Qt's
  canonical `/` boundary without doubling a root's trailing slash; its existing
  duplicate-first Windows regression was red, and a sibling `Note` versus `Notes`
  check protects the non-descendant case. Autosave closes its baseline read
  handle before atomic replacement; the second pre-commit baseline check stays
  mandatory. Directory notifications invalidate only cached CSS in that immediate
  directory, allowing same-size, restored-mtime edits to refresh every window
  without changing debounce, watch registration or no-op output suppression.
  Footer capsule widths reserve the actual font advance plus padding, and
  Omawrite measures a monospaced glyph advance. Symlink tests retain real native
  links and verify the missing target cannot be opened; Unix target-existence
  assertions remain. Auxiliary Copy measures its revision after the separate
  Find/highlight operation and still preserves source, modified state and history.
  The affected existing tests remain mandatory; the integrated focused rerun is
  in progress, so no new pass result is claimed here.
- Windows testing now adds a mandatory `QT_QPA_PLATFORM=windows` appearance
  gate for actual Dark/Light/Dark transitions before the full offscreen suite.
  Both processes run and either nonzero result fails the job; separate native
  appearance logs/XML are retained. Offscreen Qt does not implement color-scheme
  override requests, so its appearance case instead verifies real palette-based
  light/dark transitions while the scheme is Unknown. This fallback check does
  not replace the native gate. These updated gates await Windows execution.
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
  The full Mac deployed-app checker then passed all 124 scenarios with no
  failures or QML warnings (local Qt 6.11.2, isolated application/profile).
- Editor job of [integrated shared CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39671295502)
  passed. Its Mac test process completed with 294 passed, 0 failed and 13 skipped
  (12 unavailable native grammar checks plus Windows appearance); the workflow was
  cancelled by the next branch push at completion, so this is test-log evidence,
  not a green workflow result. [Shared CI at `6eeca40`](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39673900779)
  is green: Mac 294 passed, 0 failed, 13 skipped with the same reasons; editor
  tests/typecheck/committed-bundle checks passed.

- Refreshed x64 guest run at `6eeca40` passed all eight Web/PDF identity/paint
  checks, the portable menu/theme routes and compact rows, with no QML warnings.
  All remaining failures were the top document bar's hover reveal/latching.
  A local regression using the real desktop menu reproduced the product defect:
  menu bounds started at y=0 with height 40, while the document header also
  started at y=0. Explicit zero ApplicationWindow padding defeated the menu's
  safe-area reservation. Padding now follows the active menu height; ordinary
  Mac padding stays zero. Before/after geometry is header y=0/y=40, and the new
  regression plus existing chrome/menu/F6/focus tests passed (8/0/0). The native
  checker also requires exact menu/header adjacency and records their geometry.
  Native verification of the fix is pending; the current guest package remains
  diagnostic and fails overall acceptance.

- Integrated local regression run recorded 31 passed and two failures: the new
  glyph-width method did not notify QML when the font changed, and the older
  Omarchy fixture depended on an earlier test selecting the System preset.
  The width now uses `TextMetrics.advanceWidth` bound to the actual editor font;
  the exact 65-character/zoom/centering assertions are unchanged. The theme fixture
  explicitly selects its settings and isolates its home-directory input (including
  Windows USERPROFILE). Both failures and adjacent appearance tests then passed:
  **6 passed, 0 failed, 0 skipped**. The other affected file, footer, header, menu,
  search and authorship checks passed in the integrated run. Native Windows
  confirmation remains pending.

- Rebuilt the isolated Mac app after all integrated corrections. Its complete
  deployed-executable checker passed **124 scenarios**, with no failures and no
  QML warnings. Source selection and footer labels were also checked in a fresh
  rendered frame. This is shared-code regression evidence; the next native
  ARM64/x64 packages still require their own CI and guest verification.

## Windows M2 menu limits

The initial Windows menu exposes basic File/Edit/Format/View/Help commands.
The following desktop affordances remain M2 omissions:

- Open Recent; the Go menu, including saved searches, hashtags and navigation;
  the Authors menu and Set Up Authorship; Center window; and What's New.
- The Output Style submenu and additional structure/format insertion items.
- Find Next/Previous/Selection/Delete, extra copy/paste formats and variants,
  transformations, completion, smart punctuation, focus/review toggles and
  document-bar mode controls.

Shared command, dialog, backend and footer behavior remains tested, including
unavailable recent-file guards and saved-search/authorship persistence. Those
checks do not claim the omitted desktop affordances exist. macOS Versions,
Share and Speech are intentionally absent as platform capabilities.

## Independent review and open qualification

Independent reviews covered menu focus/clipboard parity, preview-check isolation,
file I/O and packaging. Findings included the menu focus defect (fixed and
regressed), a folder containment guard using the Windows separator against Qt's
slash-normalized paths (native red observed; minimal fix awaiting native green),
autosave's replacement-blocking read handle and the Windows preserved-mtime CSS
watcher case (both fixed in source, awaiting native rerun), and acceptance
verification that needed exact distinct Web/PDF scenarios (fixed with adversarial
tests). Latest source and fixture corrections received independent review.

Deployed Windows rendering, normal startup/restore, NTFS and
UNC paths, installer execution/upgrade/uninstall, DPI, and physical input remain
unqualified. Additional W09/W10 follow-up includes Windows filename/alternate
stream rejection, path case identity, and hostname-bearing file URLs. The source
still inherits Mac `0.3.0-dev49`; an internal Windows artifact is identified by
architecture, exact source commit and manifest, not by Mac release readiness.

Raw logs/screenshots remain local or in CI artifacts. Root `STATUS.md` and the
primary Mac checkout/installed bundles are unchanged.
