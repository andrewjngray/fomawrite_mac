# Windows M2 repair evidence

Updated: 22 October 2026. **Repair work in progress; M2 and the full normal-launch gate remain open.** This record does not promote a package or change the Mac baseline.

## Observed failure and local comparison

Andrew reported that the ARM64 app opened menus but actions, buttons and folder navigation did not work from the Mac-shared Desktop. The running `Fomawrite.exe` hash matched the original M1 `b45cb345a9c4ff880f1f132dba936d2bb4dce9e0` package.

Subsequent ordinary shared-folder launches repeatedly produced Live editor page-load failures; a QtWebEngine renderer crash was also observed. A Windows-local copy with the **same executable hash** completed real **File → Open File…**, **View → Source** and **File → Save**, including persistence of the edited sample. This is evidence of an observed location-dependent difference. It does not fully isolate the failure mechanism or establish that every original symptom has the same cause. Successful local file actions do not establish Live, library folder selection, restore or the complete normal-launch gate.

The M1 document-view checker used isolated settings and synthetic Qt input. Its earlier passes remain evidence for that bounded diagnostic; they do not resolve this ordinary-launch failure. Original package identities and their earlier checks remain in [M1 evidence](M1-EVIDENCE.md).

## Diagnostic candidate, separate from the repair

[Windows CI 39736778817](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39736778817) is green for native ARM64 and native x64 at commit `d8b8580384821f0e3d2f3cfd573d22ccec83369e`. This candidate adds normal-startup profile isolation so native-input tests can run without reusing Andrew's settings, recovery or IPC namespace. It is **not a fix** and contains neither the bounded renderer recovery nor the local starter described below.

The ARM64 artifact was downloaded into `dist/windows-testing/diagnostic-candidates/d8b8580-arm64/`. Its GitHub artifact digest matched; the inner ZIP checksum and all **1,606 manifest-listed file hashes** matched, with no unlisted extracted files except the manifest itself. The executable matched both the manifest and native-build record and had ARM64 PE machine `0xaa64`. Tracked source was clean in the packaged manifest. Local `staging-verification.json` records the full paths and hashes. These are identity and copying checks, not ordinary-launch acceptance.

| ARM64 diagnostic identity | SHA-256 |
| --- | --- |
| GitHub artifact `12407687345` archive | `6e29c8392f9fedebab4b0716fde4704585434a493fa773c273ca7c3db3bf7fbd` |
| `Fomawrite-0.3.0-dev49-windows-arm64-d8b858038482.zip` | `12f2e86bc6a5080d2639f3e77a224b4d1c5b987575457df0c5cffd900ba4612f` |
| `Fomawrite.exe` | `22b90cab01f9092376f99a4ae406d4d81b4e870c9f709a532634acf8473cd1f1` |
| `manifest.json` | `c478446915ac49acb047be903e9ab5f57caa5286b65ece149d855a407a842526` |

The original testing shelf remains `b45cb34` in two architectures. Separately staging diagnostics does not update `BUILD-CATALOG.json`, `COPY-VERIFICATION.json`, `START-HERE.txt` or a latest-build recommendation. See [testing builds](TESTING-BUILDS.md).

## Confirmed Organizer click interception

The ordinary Windows gate reproduced a second defect even from local NTFS storage: the Add Library Location button centre was covered by a Material scrollbar whose paint had faded out. UI Automation hit testing at the button centre returned `ControlType.ScrollBar`. Clicking the unobstructed left interior of the same button opened the native folder picker. With that diagnostic click only, all remaining normal-launch stages passed, including Live writing-region OCR, byte-exact save, clean exit and normal document restore. Offline verification of the diagnostic driver and package passed. This does not qualify the unchanged standard centre-click gate.

The fix reserves the ScrollView vertical scrollbar gutter so the full header and row actions remain clear, and disables unused horizontal scrolling. Two regressions (short and scrolling Organizer lists) failed on the previous source because actual centre clicks never opened the folder dialog. Both pass with the fix and also verify the Favorites menu and full button clearance: **4 passed, 0 failed, 0 skipped**, including setup/cleanup. The app build passed.

The same fresh-profile diagnostic run from Mac-shared storage still produced repeated Live page-load failures and failed to complete folder navigation. The same executable from local NTFS completed the workflow. This strengthens the storage-dependent renderer finding without claiming the underlying Qt/Parallels failure mechanism is fully explained.

## Bounded Live recovery change

The working QML change replaces an unconditional immediate reload after renderer termination with **one delayed automatic retry**. Repeated termination reports do not queue additional reloads or replenish that retry. A later failure stops with a visible error and **Retry Live** control; a nonmodal notice also exposes failure when the Live page was prewarmed behind Source. Source remains available for writing.

Before reload, the page replay is refreshed from the canonical document so incremental unsaved Live edits survive reconnect. The handshake resynchronizes the canonical document again if Source changed during reload. Recovery must preserve the document revision and must not manufacture edits.

The focused local Qt run passed **6 tests, 0 failures**, including the renderer recovery regression, with red/green evidence against the previous behavior. The local app build passed. The complete local shared Qt suite then passed **310 tests, 0 failures, 1 skipped** (`./bin/test`; 344.7 seconds). The recovery fixture delivers the real WebEngine termination signal to the QML handler; it does not inject an actual operating-system renderer crash. These results establish the bounded recovery behavior under the fixture, not resolution of the shared-folder failure or acceptance of a packaged Windows repair.

## Windows-local portable starter

The working packaging change adds `START-FOMAWRITE.cmd` and `launch-local.ps1` before manifest creation. The starter verifies every listed source file, copies only manifest-listed files plus the manifest into a unique temporary Windows-local cache directory, verifies that copy and publishes it by a same-parent rename. A file lock serializes simultaneous starters. An existing verified cache is reused; a corrupt cache is rejected rather than overwritten. Separate manifest versions have separate cache directories.

The local starter passed **9 synthetic fixture checks**: exact manifest-only staging, unchanged cache reuse, concurrent lock waiting, source corruption rejection, cache corruption rejection without overwrite, unexpected cache-file rejection, unsafe path rejection, separate manifest versions, and temporary-directory cleanup. No application was launched by those fixtures. The Windows CI deployment-evidence guard now invokes the starter fixture in Windows PowerShell, with an explicit exit-code check after each Python or PowerShell guard process. CI execution of this new wiring remains pending.

Two additional real `\\Mac\Home` UNC staging/reuse checks passed. Staging the complete 1,606-file diagnostic ARM64 package from `C:\Mac\Home` to Windows-local storage also passed every hash check. Ordinary launch of the final repaired package remains pending. See [portable starter](PORTABLE-STARTER.md) for behavior and limits.

## Remaining acceptance and final package identity

The full [normal-launch gate](NORMAL-LAUNCH-GATE.md) remains pending. It must verify the exact deployed package using normal startup, native picker discovery and Windows input, byte-exact save, Live/Source switching, clean exit and restored document on reopen. Windows-local NTFS and Mac-shared storage need separate labelled evidence. The separate document-view diagnostic remains required against the same final package.

Final repair commit, package ZIP/manifests/executable hashes, copied guest reports and independent verifier results are **pending**. They must be recorded before any repaired-package recommendation. The `d8b8580` diagnostic hashes above must not be reused as identities for the working repair. Broader profiles, hardware input, DPI, installer and distribution/licensing qualification also remain open.
