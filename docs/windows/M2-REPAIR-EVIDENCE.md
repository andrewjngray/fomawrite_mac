# Windows M2 repair evidence

Updated: 22 October 2026. Current repair candidate: **`d573f1666d68efaee0b60f43f04d3fe1e96781c4`**. **ARM64 normal-launch and packaged-starter acceptance passed.** The separate ARM64 guest document-view diagnostic also passed; verified packages are promoted to the testing shelf. The x64 normal-launch gate also passed under ARM64 VM emulation after correcting a driver timing race, as recorded below. The Mac baseline is unchanged.

## Final candidate checkpoint

The candidate includes the Organizer scrollbar-gutter repair, bounded Live recovery and canonical-document reconnect, isolated ordinary-startup acceptance profiles, and the verified Windows-local portable starter. The final local `./bin/test` passed **312 tests, 0 failures, 1 skipped** in **339.8 seconds**, including the Organizer fix.

[Windows CI 39741241624](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39741241624) and [shared CI 39741241651](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39741241651) are **successful** at this exact commit. Final ARM64/x64 artifact identities and final ARM64 VM results are recorded below. Earlier candidate identities and probes remain historical evidence.

| Final CI check | ARM64 | x64 |
| --- | --- | --- |
| Complete Qt suite | 274 passed, 0 failed, 39 skipped | 274 passed, 0 failed, 39 skipped |
| Native Windows appearance | 3 passed, 0 failed, 0 skipped | 3 passed, 0 failed, 0 skipped |
| Deployed executable diagnostic | 124 scenarios, 0 failures, 0 QML warnings | 124 scenarios, 0 failures, 0 QML warnings |
| Python deployment/normal-launch guards | 36 + 25 passed, 0 failed | 36 + 25 passed, 0 failed |
| Non-UI portable starter fixture | 9 passed; no application started | 9 passed; no application started |

Shared CI passed **300 Mac Qt tests, 0 failures, 13 skips**, and **321 editor tests, 0 failures, 0 skips**. Editor typecheck, asset build and committed-asset verification passed. CI evidence artifacts were hash-verified: ARM64 `12410660428`, SHA-256 `68adc8a24183e21493a9f15c6ea674fc048b09f700ab0d7613e2816deee35096`; x64 `12409827967`, SHA-256 `d57394bf755e737895c58a6bae50a318a084c5272145b76ca4938ea93389fd7d`. Zero QML warnings applies to deployed diagnostics; it does not claim warning-free Qt suite logs.

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

At the diagnostic stage the testing shelf remained `b45cb34` in two architectures; the final repair promotion is recorded below. Separately staging diagnostics does not update `BUILD-CATALOG.json`, `COPY-VERIFICATION.json`, `START-HERE.txt` or a latest-build recommendation. See [testing builds](TESTING-BUILDS.md).

## Confirmed Organizer click interception

The ordinary Windows gate reproduced a second defect even from local NTFS storage: the Add Library Location button centre was covered by a Material scrollbar whose paint had faded out. UI Automation hit testing at the button centre returned `ControlType.ScrollBar`. Clicking the unobstructed left interior of the same button opened the native folder picker. With that diagnostic click only, all remaining normal-launch stages passed, including Live writing-region OCR, byte-exact save, clean exit and normal document restore. Offline verification of the diagnostic driver and package passed. That altered driver remains a diagnostic probe. The final repair later passed the unchanged standard centre-click gate recorded below.

The fix reserves the ScrollView vertical scrollbar gutter so the full header and row actions remain clear, and disables unused horizontal scrolling. Two regressions (short and scrolling Organizer lists) failed on the previous source because actual centre clicks never opened the folder dialog. Both pass with the fix and also verify the Favorites menu and full button clearance: **4 passed, 0 failed, 0 skipped**, including setup/cleanup. The app build passed.

The same fresh-profile diagnostic run from Mac-shared storage still produced repeated Live page-load failures and failed to complete folder navigation. The same executable from local NTFS completed the workflow. This strengthens the storage-dependent renderer finding without claiming the underlying Qt/Parallels failure mechanism is fully explained.

## Bounded Live recovery change

The candidate QML change replaces an unconditional immediate reload after renderer termination with **one delayed automatic retry**. Repeated termination reports do not queue additional reloads or replenish that retry. A later failure stops with a visible error and **Retry Live** control; a nonmodal notice also exposes failure when the Live page was prewarmed behind Source. Source remains available for writing.

Before reload, the page replay is refreshed from the canonical document so incremental unsaved Live edits survive reconnect. The handshake resynchronizes the canonical document again if Source changed during reload. Recovery must preserve the document revision and must not manufacture edits.

The focused local Qt run passed **6 tests, 0 failures**, including the renderer recovery regression, with red/green evidence against the previous behavior. The local app build passed. An earlier complete local shared Qt run passed **310 tests, 0 failures, 1 skipped** in 344.7 seconds, before the Organizer fix. The final candidate run passed **312 tests, 0 failures, 1 skipped** in 339.8 seconds. The recovery fixture delivers the real WebEngine termination signal to the QML handler; it does not inject an actual operating-system renderer crash. These fixture results establish bounded recovery behavior. Final ARM64 normal-launch acceptance is recorded below; the direct shared-folder renderer mechanism remains unresolved.

## Windows-local portable starter

The candidate packaging adds `START-FOMAWRITE.cmd` and `launch-local.ps1` before manifest creation. The starter verifies every listed source file, copies only manifest-listed files plus the manifest into a unique temporary Windows-local cache directory, verifies that copy and publishes it by a same-parent rename. A file lock serializes simultaneous starters. An existing verified cache is reused; a corrupt cache is rejected rather than overwritten. Separate manifest versions have separate cache directories.

The local starter passed **9 synthetic fixture checks**: exact manifest-only staging, unchanged cache reuse, concurrent lock waiting, source corruption rejection, cache corruption rejection without overwrite, unexpected cache-file rejection, unsafe path rejection, separate manifest versions, and temporary-directory cleanup. No application was launched by those fixtures. The Windows CI deployment-evidence guard now invokes the starter fixture in Windows PowerShell, with an explicit exit-code check after each Python or PowerShell guard process. The final-candidate starter fixture passed on both architectures in run 39741241624. The concurrent fixture now passes its cache root explicitly to the child job and records job state/reason/errors on failure, with a bounded 60-second completion wait.

Two additional real `\\Mac\Home` UNC staging/reuse checks passed. Staging the complete 1,606-file diagnostic ARM64 package from `C:\Mac\Home` to Windows-local storage also passed every hash check. The final packaged ARM64 starter subsequently passed ordinary launch, local-executable identity, isolated-native-profile and File → Exit checks; exact evidence is below. See [portable starter](PORTABLE-STARTER.md) for behavior and limits.

## Final package identities

Both packages are staged under `dist/windows-testing/diagnostic-candidates/d573f16-{arm64,x64}/`. Each `staging-verification.json` records absolute paths, clean tracked source, verified starter hashes, matching native-build executable and no unlisted extracted files except the manifest. ARM64 has **1,608** manifest-listed files and PE machine `0xaa64`; x64 has **1,611** and `0x8664`. All listed hashes matched. The staging receipt's earlier pending-qualification label predates the VM results below.

| SHA-256 identity | ARM64 | x64 |
| --- | --- | --- |
| GitHub package artifact archive | `23b4fdd16f19a85ee8d4f26841a86b08b22c2355f26d42e74ef8df3acc1edcfb` (artifact `12408959992`) | `ec0173da7c4f7fd40a9821845cb52c00d6cb47e2117afb7fae07da2aafb83df8` (artifact `12409930537`) |
| Inner portable ZIP | `ffc8f77a6c0d1213145caea23111aa0919a448d143350c8a4b58d9540e2b1513` | `17ec8958c5eada6b86067e4b8b648c5683a34fe52f090ff5dcbe01cce6a1f8ec` |
| `Fomawrite.exe` | `6f60ca2b82c31ba0571a7506f8865373b928e9a8a837e9556289358071ccc6b6` | `d6c512120283859405678aca03d705e1742ac77665e9bc35655e05b2e3531809` |
| `manifest.json` | `105bcb8f90518198bffec9c38b0ba42f84efa7808a0f1cce2d483657f4a28639` | `93e3e24959f1cba1ae85ac627ec3a747b96ed35e357b48362af770435dfa889f` |

## Final ARM64 VM normal-launch evidence

The unchanged standard [normal-launch gate](NORMAL-LAUNCH-GATE.md) **passed all six stages with standard button-centre clicks** against the ARM64 executable above. It used ordinary argument-free startup, an isolated native profile, Windows input and native folder/file pickers. Exact save, Live writing-region pixels/OCR, Source return, two exit-zero launches and normal document restore passed. Strict offline verification returned no failures; renderer/page-load logs passed its checks.

The local executable was `C:\Users\andrewgray\AppData\Local\Fomawrite\Portable\arm64-d573f1666d68-105bcb8f90518198\Fomawrite.exe`. The local evidence is `dist/windows-testing/results/final-d573-arm64-8d4cf73d3c684efcb11e197f86958f78/evidence/normal-launch-report.json`, SHA-256 **`c5e38f2a4036fdacaccd74a8b0e87f8ec247569ed509f23fb270781c690debe8`**. `normal-launch-verification.json` beside it records the strict pass and same report digest.

**Retained first attempt:** `dist/windows-testing/results/final-d573-arm64-074caf2c86bd4d239b26866729767711/` stopped at ordinary startup before workflow input: foreground activation was blocked by an older instance's dialog. Andrew explicitly authorized cancel and normal close. After the old process was confirmed absent, a fresh run of the unchanged gate passed. The unsuccessful attempt is retained as environment-blocked evidence; no workflow assertion was skipped or weakened in the successful run.

**Shared-document supplement:** the same local executable passed the same six stages with the sole driver substitution of the disposable writing directory onto shared storage. Report: `dist/windows-testing/results/shared-document-d573-arm64-6395e60daf9740239640df380159b902/evidence/normal-launch-report.json`, SHA-256 **`c7e9cc83087952083bdbb35bdb7a002ef9d342d4d2e98263dcfb85448921b23a`**. Its `offline-verification.json` records pass, no failures and the writing-path-only scope. This proves the tested shared-document workflow with local binaries; it does not qualify direct shared-folder WebEngine execution.

**Packaged starter:** `START-FOMAWRITE.cmd` from the final shared ARM64 package launched the verified local executable through ordinary startup with an isolated native profile, reused its verified cache and exited with status zero through File → Exit. Receipt: `dist/windows-testing/results/starter-d573-arm64-d85250e2e3d0455e82711a25da1e705e/starter-check.json`, `passed: true`, `normalStartup: true`, `stageOnly: false`, commit and executable/manifest hashes matching the table above. This was an actual packaged command-file launch, separate from the full six-stage gate and non-UI staging fixtures.

## Final deployed diagnostic and testing-shelf promotion

The same Windows-local ARM64 package passed the separate deployed Source/Live/Web/PDF checker: **124 scenarios, 0 failures, 0 QML warnings**, fresh process exit zero. Report: `dist/windows-testing/results/deployed-d573-arm64-90586882612d4f15bb1005e1cb68d252/report.json`, SHA-256 **`8e0047073fd557a4599faed7f49d3e882da3a1292ec1b33de8a04fc24029e38e`**. `guest-run.json` records the run; `package-verification.json` reports `passed: true`, no failures, and the exact ARM64 commit/executable/manifest above. This synthetic Qt diagnostic supplements the ordinary Windows-input gate; it does not replace it.

Verified originals and extracted packages were copied into new `dist/windows-testing/builds/d573f16-arm64/` and `builds/d573f16-x64/` folders. ZIP and every manifest file hash were rechecked, with no extra extracted application files except the manifest. Per-build and shelf `COPY-VERIFICATION.json`, `BUILD-CATALOG.json` and `START-HERE.txt` record the promotion. `START-WINDOWS-ARM64.cmd` points to the ARM64 packaged starter. Older builds remain unchanged. x64 also passed the ordinary VM workflow under emulation in the follow-up below.

## Remaining qualification

Windows test parity continues: 39 Qt skips require individual capability/fixture review; genuine platform exclusions must remain explicit. A physical x64 machine, wider user profiles, the DPI/display matrix, installer and distribution/licensing qualification remain open. The earlier `d8b8580` unobstructed-edge probe remains separate from the final standard centre-click pass. Direct shared-executable rendering is not claimed repaired by running verified local binaries.

## Follow-up: picker readiness and x64 ordinary input

The first final x64 run failed at the driver's one-shot Select Folder lookup after a fixed 500 ms navigation delay. The native picker and Select Folder control are visible in its retained screenshot/UIA evidence: `results/final-d573-x64-0aa354819b8347e19bdf6d23956af7e7/`. The driver now polls the real enabled/visible button through its existing bounded wait and aborts if the application exits; no input or result assertion was removed.

`windows/test_normal_launch_wait.ps1` runs the actual parsed driver wait/selection logic with a non-UI delayed-control fixture. It failed against the prior driver at the original missing-button error; the corrected driver passed **3 checks** (delayed readiness, bounded timeout and app exit). CI invokes that fixture in a fresh Windows PowerShell process. This regression proves the driver behavior, not application UI behavior.

Fresh unchanged-package ordinary desktop runs then passed all six stages on both architectures, with strict offline verification and two exit-zero launches each:

| Architecture in Windows ARM64 VM | Report under `dist/windows-testing/results/` | Report SHA-256 |
| --- | --- | --- |
| ARM64 native | `final-d573-arm64-1d9f21c479ee49a79a16df1881779957/evidence/normal-launch-report.json` | `4a83afee4eee40d0e103b0bda1652298823bd28179904212e486c644fb939979` |
| x64 emulated | `final-d573-x64-c5d3d25621a74a2b80b930fa7240e037/evidence/normal-launch-report.json` | `8d63882eef02ffc00bdaa35beee995ffef094cf22315bfa5c64b3504590f9047` |

Each run retains its exact driver/helper and `normal-launch-verification.json`. The updated driver hashes differ from the earlier retained ARM64 run. Product/package identity remains `d573f16`; this follow-up changed the test driver only. No physical x64 machine or broader Windows parity is claimed.
