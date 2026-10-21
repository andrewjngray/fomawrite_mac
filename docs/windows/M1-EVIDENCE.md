# Windows M1 implementation evidence

22 October 2026. **M1 (executable and CI) is complete** for code commit `b45cb345a9c4ff880f1f132dba936d2bb4dce9e0`. Both native Windows CI jobs, shared Mac/editor CI and both Parallels guest checks passed. This is a bounded M1 result; desktop parity, normal startup/restore, installer and distribution qualification remain in later milestones. Earlier failures and fixes below are historical evidence.

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

## M1 exit evidence: `b45cb34`

Tested code commit: **`b45cb345a9c4ff880f1f132dba936d2bb4dce9e0`**. Later
changes to these evidence documents do not rebuild or reidentify the artifacts.
This is the bounded M1 executable/CI checkpoint, not installer, beta or public
release qualification.

| Gate | Result for this exact code candidate |
| --- | --- |
| Native ARM64 Windows CI | Appearance 3 passed, 0 failed, 0 skipped (27 ms); full offscreen Qt 269 passed, 0 failed, 39 skipped (454.751 seconds); deployed checker 124 scenarios, 0 failures, 0 QML warnings and strict verification passed. |
| Native x64 Windows CI | Appearance 3 passed, 0 failed, 0 skipped (40 ms); full offscreen Qt 269 passed, 0 failed, 39 skipped (445.915 seconds); deployed checker 124 scenarios, 0 failures, 0 QML warnings and strict verification passed. |
| Native ARM64 in Parallels | 124 scenarios, 0 failures, 0 QML warnings; fresh exit 0 and copied-report identity verification passed. |
| x64 emulation in Parallels ARM64 | 124 scenarios, 0 failures, 0 QML warnings; fresh exit 0 and copied-report identity verification passed. |
| Shared Mac CI | 295 passed, 0 failed, 13 expected skips; Qt 6.11.3 ARM64 on macOS 15.7.9, 596.040 seconds. |
| Shared editor CI | 321 passed, 0 failed, 0 skipped; typecheck, asset build and committed-bundle checks passed. |
| Local focused regression | 10 passed, 0 failed, 0 skipped; eight affected slots plus setup/cleanup, 41.534 seconds. |
| Isolated local Mac deployed app | 124 scenarios, 0 failures, 0 QML warnings; Cocoa, Qt 6.11.2. |

Runs: [native Windows 39681549835](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39681549835)
and [shared Mac/editor 39681549825](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39681549825).
Both native packages use Qt **6.11.3**, MSVC 2022 v143 **14.44.35207**, Windows
SDK **10.0.26100.0** and app-local CRT **14.44.35211.0**. ARM64 was built on
Windows 11 build 26200; x64 on Windows Server 2022 build 20348. Their manifests
contain 1,606/1,609 hashed files respectively. Native PE and runtime-version
checks passed; no browser sandbox override was used. Actual native ARM64 MSVC
`cl /bigobj` compilation succeeded after the earlier C1128 failure; no test or
optimization setting was removed to pass it.

The guest is Windows 11 Enterprise ARM64, OS **10.0.26200.0**, desktop session 1;
PowerShell **5.1.26100.9711**. Separate fresh NTFS package directories were used
without a Qt SDK or developer PATH. The launch receipt, manifest, executable path,
report hash and copied report were verified independently. ARM64 is native;
x64 is explicitly emulation on ARM64. Source and Live screenshots show the correct
fixture; all eight required Web/PDF current-document identity and paint observations
pass in both guest reports.

| Identity | ARM64 | x64 |
| --- | --- | --- |
| ZIP SHA-256 | `f2ed158fcebcc245c12943f660ece9cab084de3ab95cc120b28f2040065e3c52` | `aa228bdf1e658e610fdf046d06e5e713a33dab25e032211163dcf48a52cbfbf6` |
| Executable SHA-256 | `6356bcd7c749b702f5ca4c57ef5c3e4f03d4464c252388130ae799dd492a73c2` | `9597f91fa810a929af571a3e8286677c08768d1ee853fb40ce8b0e5e847222c1` |
| Manifest SHA-256 | `a04e34d336fd6fe972a8fd49b8bbd1b2ed6fc908d926e9d9da2bc174ddebb1b3` | `1e319079afdcd041b1ea60e51b5b51b98fc71dc6d9630bdead9398f78ac9e217` |

Guest roots are under `C:\Users\andrewgray\AppData\Local\FomawritePortQA\`:
`b45cb34-arm64-f16a95acca78` and `b45cb34-x64-6281f8809886`, each with the
executable at `Fomawrite\Fomawrite.exe`. Local evidence is under the Windows task
workspace's `work/ci-b45cb34/arm64-package/b45cb34-arm64-f16a95acca78/guest-evidence`
and `work/ci-b45cb34/x64-package/b45cb34-x64-6281f8809886/guest-evidence`.
Each contains `report.json`, `guest-run.json`, `guest-context.json`,
`package-verification.json` and screenshots; adjacent `guest-run-location.json`
records the ZIP hash and exact paths. Shared logs are
`/tmp/fomawrite-shared-mac-b45cb34.log` and
`/tmp/fomawrite-shared-editor-b45cb34.log`; local Mac checker evidence is
`work/mac-acceptance-m1-final/report.json`. Native CI evidence is in
`work/ci-b45cb34/arm64-evidence` and `work/ci-b45cb34/x64-evidence`; complete job
logs are `/tmp/fomawrite-arm64-b45cb34.log` and
`/tmp/fomawrite-x64-b45cb34.log`. Both CI executable and manifest hashes match
the exact packages independently verified in the guest.

### Windows skip scope

Each native Windows full suite's 39 skips divide into **10** disabled updater/override
fixtures (W12), **28** legacy macOS native-checker fixtures and **one** native Mac
menu topology fixture. The updater skips retain portable bundle/path/URL validation;
Windows update activation remains unavailable. Harper capability, bridge and real-page
Source/Live/Review coverage passed in both current native suites. Some of the 28
legacy fixtures also contain shared Review navigation/resize/render assertions;
their omission is a coverage limitation, not proof that Windows Review is unavailable
or that full W11/UI parity is qualified. The actual Windows menu regressions run.
Neither native run had a symlink-privilege skip.
Zero QML warnings above refers to deployed checkers; the offscreen Qt logs contain
warning messages and are not claimed warning-free.

The shared Mac suite's 13 skips are 12 unavailable runner-native grammar checks
and one Windows-only appearance test. Synthetic checker input is confined to the
isolated app; native physical input is rejected. Normal startup/restore, broader
filesystem/shared-folder tests, physical input, DPI, installer and licensing gates
remain in their later milestones.

## Historical development evidence

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
  actual DLL versions, native architecture and hashes; a native rerun was still needed at that checkpoint.
- Third native CI at `6eeca40` again compiled both apps and packaged x64. ARM64
  runtime inventory showed native v143 DLLs at version `14.44.35211.0`, plus an
  x64 `vcruntime140_1.dll` inside the ARM64 redistributable folder. The strict
  architecture guard rejected that mixed folder. The corrected selector inspects
  normal and delay imports from every packaged PE and every native runtime DLL,
  excludes an unneeded foreign DLL only when no deployed/native runtime imports it,
  and rejects missing or foreign required dependencies. Selected DLL versions,
  hashes and architecture guards remain mandatory. The actual-shape regression
  failed before and passed after this correction; native ARM64 deployment later
  passed at `3095601`, recorded below.
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
- Reviewed bounded fixes were integrated for native rerun. Folder containment uses Qt's
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
  The affected existing tests remain mandatory; the integrated local results and
  native qualification limits are recorded below.
- Windows testing now adds a mandatory `QT_QPA_PLATFORM=windows` appearance
  gate for actual Dark/Light/Dark transitions before the full offscreen suite.
  Both processes run and either nonzero result fails the job; separate native
  appearance logs/XML are retained. Offscreen Qt does not implement color-scheme
  override requests, so its appearance case instead verifies real palette-based
  light/dark transitions while the scheme is Unknown. This fallback check does
  not replace the native gate. At `3095601`, the native x64 appearance gate passed;
  ARM64 test compilation stopped before the gate executed.
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
  That `6eeca40` guest package remained diagnostic and failed overall acceptance.
  Native deployed checkers and guest runs at `3095601` later passed, recorded below.

- Integrated local regression run recorded 31 passed and two failures: the new
  glyph-width method did not notify QML when the font changed, and the older
  Omarchy fixture depended on an earlier test selecting the System preset.
  The width now uses `TextMetrics.advanceWidth` bound to the actual editor font;
  the exact 65-character/zoom/centering assertions are unchanged. The theme fixture
  explicitly selects its settings and isolates its home-directory input for the
  local Mac run. The attempted Windows USERPROFILE override did not redirect
  Qt's OS-profile lookup; the native x64 fixture still failed and needs correction.
  Both local failures and adjacent appearance tests then passed:
  **6 passed, 0 failed, 0 skipped**. The other affected file, footer, header, menu,
  search and authorship checks passed in the integrated run. Complete native Windows Qt qualification remained pending at that checkpoint; deployed-checker
  confirmation at `3095601` is recorded below.

- Rebuilt the isolated Mac app after all integrated corrections. Its complete
  deployed-executable checker passed **124 scenarios**, with no failures and no
  QML warnings. Source selection and footer labels were also checked in a fresh
  rendered frame. This is shared-code regression evidence; the next native
  ARM64/x64 packages received their own CI and guest checker verification at
  `3095601`, recorded below; complete native Qt qualification was still pending then.

## Native ARM64 candidate `3095601`

[Windows CI run 39678109253](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39678109253),
commit `309560170492a4c0b7d3b53c71ce4bd9d55a1206`, compiled and strictly
packaged the native ARM64 app. The manifest records 1,606 hashed files, including
119 native PE files (2 executables and 117 DLLs). Import inspection covered the
110 deployed PEs before adding the nine native CRT DLLs. The selected v143
runtime DLLs are `14.44.35211.0`, newer than compiler `14.44.35207`; the foreign
x64 `vcruntime140_1.dll` is explicitly recorded as excluded because no deployed
PE or selected native CRT DLL imports it. Native architecture and version checks
remain mandatory. Diagnostic ZIP artifact: `12382992713`; CI evidence artifact:
`12383053256`. The artifact remains explicitly internal and unqualified.

The deployed ARM64 executable passed the native Windows CI checker, including
all eight required Web/PDF identity and paint observations, with no failures or
QML warnings. The same package then ran from disposable NTFS storage in the
Parallels Windows 11 guest without a Qt SDK or developer PATH. The guest context
records **native ARM64**, OS `10.0.26200.0`, desktop session 1, and PowerShell
`5.1.26100.9711`. Its complete deployed checker passed **124 scenarios, 0 failures,
0 QML warnings**. The fresh process receipt records exit 0; offline verification
of the copied report passed against the exact Windows executable path, package
manifest and hashes:

- ZIP SHA-256: `a35856f94c9061a1067fc8247e4aa63e7c93f4a64acdf5c4f3a5182c5593cfd2`.
- Executable SHA-256: `13306b145dae2e79ce7b0b312f56a8469a0c89bc08a1d5c074bc2fe0167ad4a7`.
- Manifest SHA-256: `43ebfa36c5399cdb98babb23c17e2f3f1f4abf8ed0b91bf51fbc4053a63bc2a1`.

Local guest evidence is under
`work/ci-3095601/arm64-package/3095601-arm64-ac8f758c32cc/guest-evidence`
in the Windows task workspace (`report.json`, `guest-run.json`,
`guest-context.json`, `package-verification.json` and screenshots). CI evidence
is under `work/ci-3095601/arm64-evidence`; complete ARM64 job log is
`/tmp/fomawrite-arm64-3095601.log`.

The ARM64 job still failed: MSVC reported **C1128**, “number of sections exceeded
object file format limit: compile with /bigobj,” while compiling the monolithic
`tst_fomawrite.cpp`. Neither the native Windows appearance gate nor the full Qt
suite executed. A bounded `tests/tests.pro` correction now adds `/bigobj` only
for Windows MSVC, including the `win32-arm64-msvc` kit; no optimization, assertion
or test cases changed. That correction awaited native rerun at this checkpoint; its successful native compilation is recorded above. Native x64 appearance passed (3/0/0 including setup/cleanup), and deployed CI
acceptance passed 124 scenarios. Its full offscreen Qt suite reported **261 passed,
8 failed, 39 skipped** in 517.536 seconds; bounded corrections and focused evidence are recorded below. Shared Mac/editor results are green, recorded below.
These deployed-checker passes do not qualify normal startup/restore or complete M1.

- The separate x64 package at `3095601` also passed **124 scenarios, 0 failures,
  0 QML warnings** under x64 emulation in the same ARM64 Parallels guest, using a
  separate fresh NTFS directory and isolated profile. The fresh exit-zero receipt
  and copied report passed offline identity verification. ZIP SHA-256:
  `b5fd6faffa1126b0110e1199b232237949dc50beb6d6b1baf5e07052c51d73c4`;
  executable SHA-256:
  `5ac323500123357f1c0f1f7bdcb74389d168b2eaa98abc5316062dab558f4670`;
  manifest SHA-256:
  `363ec00872070a3d03ef5ab57183367e0f568b5f52de789700adf6f908416ea5`.
  The native x64 Windows appearance gate passed (3/0/0 including setup/cleanup),
  and its deployed CI checker passed 124 scenarios. The full offscreen Qt suite
  reported **261 passed, 8 failed, 39 skipped** in 517.536 seconds. Remaining
  failures cover Live formatting selection, five footer checks, export-dialog
  height and the Omarchy home fixture; bounded corrections and focused evidence are recorded below. Guest x64 emulation is not labelled native ARM64.
- Mac qmake generation accepted the `/bigobj` correction and confirmed the flag
  is absent from Mac compiler flags. The native ARM64 before/after compile result
  was still required then; it is recorded in the final candidate above. No tests or optimisation settings were removed.

- [Shared CI at `3095601`](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39678109413)
  completed green. Mac Qt 6.11.3 on macOS 15.7.9 ARM64: **295 passed, 0 failed,
  13 expected skips**, 582.6 seconds. The skips are 12 unavailable runner-native
  grammar checks and the Windows-only appearance case. Editor: **321 passed,
  0 failed, 0 skipped**; typecheck, asset build and committed-bundle checks passed.

## Corrections between `3095601` and `b45cb34`

The footer defect reproduced locally with the bundled mono font at size 13 in a
321px pane: the appearance selector was hidden (2 passed, 1 failed including
setup/cleanup). The product fix preserves full labels and original minimum hit
widths, allocating spare width to padding instead of enlarging every capsule.
The regression retains default-font assertions and adds font pressure plus real
Source and Output Style clicks.

The optional Backend constructor directory now isolates only that instance's
Omarchy palette input. Its default preserves the existing production directory;
loading and watching share the same stored root. The all-platform fixture checks
empty-directory fallback colors and actual palette-file loading without changing
home variables or touching the real profile, and restores appearance preferences.
The export-dialog test measures its actual parent with the same exact clamped
sizes and 16px containment during resizing. The Live fixture confirms JavaScript
selection before invoking a command, then forces a stale cache and requires the
actual request/reply token and formatting result; no Live product code changed.

The eight focused slots (all five native-red footer cases, export, Omarchy and
Live) passed locally: **10 passed, 0 failed, 0 skipped** including setup/cleanup,
41.534 seconds. Red and green logs are
`/tmp/fomawrite-footer-font-pressure-red.log` and
`/tmp/fomawrite-footer-font-pressure-green.log`. The rebuilt Mac app succeeded;
its complete isolated Cocoa checker subsequently passed 124 scenarios with no
failures or QML warnings. The Windows MSVC `/bigobj` correction and remaining
corrections progressed to the final candidate above. The focused result did not
replace either native Windows Qt gate, full shared CI or fresh deployed checks.

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
file I/O and packaging. Findings included the menu focus defect, Qt-normalized
folder containment, autosave's replacement-blocking read handle, preserved-mtime
CSS watching and distinct current-document Web/PDF verification. Their bounded
corrections and regressions passed the final native suites/checkers above. Latest
source and fixture corrections received independent review.

Native ARM64 and x64 compilation, native appearance, full portable Qt suites and
isolated deployed rendering now satisfy the bounded M1 exit. Normal startup/restore,
broader NTFS and UNC behaviour, installer execution/upgrade/uninstall, DPI and
physical input remain unqualified. Additional W09/W10 follow-up includes Windows
filename/alternate-stream rejection, path case identity and hostname-bearing file
URLs. The source still inherits Mac `0.3.0-dev49`; an internal Windows artifact
is identified by architecture, exact code commit and manifest. This checkpoint
does not qualify an internal beta, installer or public distribution.

Raw logs/screenshots remain local or in CI artifacts. Root `STATUS.md` and the
primary Mac checkout/installed bundles are unchanged.
