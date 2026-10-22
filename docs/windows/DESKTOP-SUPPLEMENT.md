# Review and Center desktop supplement

This maintained driver covers six physical Windows input workflows: Open by Path cancellation, Command Palette cancellation, Review toggle, Next Issue, Previous Issue, and Window → Center. It composes the current normal-launch driver and helper; it returns to that driver for Source preservation, normal Exit, ordinary relaunch and restored-document checks. It makes no Print claim.

This is a reproducible **future acceptance harness**, not new qualification of the b056 package. Native Windows PowerShell 5.1 parsing, generated-driver preparation and combined C# helper compilation passed on ARM64 and x64 in commit `385340d` ([CI run](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39824399841)). The ten preparation/identity checks send no input and launch no application. Interactive desktop execution of this maintained composition remains pending. The historical failures in [M2-DESKTOP-B056.md](M2-DESKTOP-B056.md) remain unchanged. This covers parts of [TEST-MATRIX.md](TEST-MATRIX.md) W11/W13/W14; the wider matrix remains applicable.

## Sources and preparation

The sources are `bin/windows/run-desktop-supplement-guest.ps1`, `desktop-supplement-steps.ps1`, `desktop-supplement-input.cs`, and the existing `run-normal-launch-guest.ps1` / `normal-launch-input.cs`. The steps file has exactly one `# DESKTOP-SUPPLEMENT-WORKFLOW` boundary: its prefix is the early physical Help-menu prerequisite; its suffix runs after the original Live capture. The supplementary C# file contains the read-only owned-window/selection/geometry witnesses and bounded owned-menu hover helper. After Center, the driver reopens the original sample, reads its fresh Source bounds, returns to Live, and retains the final OCR observation as `$render`. It refreshes `live-mode-rendered.png` and its UIA snapshot at that location, so the base report region, Live content crop and capture describe the same restored state. All inputs use the established Win32 SendInput driver.

Use an unlocked English Windows desktop and a fresh hidden Windows PowerShell 5.1 process. No existing Fomawrite process may be attached or displaced. The exact verified StageOnly package must be on local NTFS. Supply these mandatory bindings from independently verified package/staging records, never from an inherited application-version label:

- `Package`, `LocalStageReceipt`, `Architecture`, `ExpectedCommit` (full SHA).
- `ExpectedManifestSha256`, `ExpectedExecutableSha256`, `ExpectedBackendSourceSha256`, `ExpectedMenuSourceSha256`.
- `DriverDirectory` pointing at the maintained sources and a fresh `Output` outside the package.

First run the saved wrapper with `-PrepareOnly`. This checks exact clean package identity and StageOnly binding, verifies unique composition anchors, parses the composed PowerShell, and writes `driver/run-normal-launch-guest.ps1`, `driver/normal-launch-input.cs` and `driver-composition.json`. It neither launches the app nor sends input. Native ParseFile the wrapper/steps and compile-only Add-Type the generated helper with UIAutomationClient, UIAutomationTypes and WindowsBase references in a separate fresh process. Record that validation separately; `helperCompilePending=true` in the composition receipt is intentionally unchanged by preparation.

After reviewing the generated files and native validation, execute the same saved wrapper with the same binding arguments, a different fresh output and no `-PrepareOnly`, using the established short hidden PowerShell 5.1 ScriptBlock launcher. The generated driver also compiles the helper before launching. Do not replay uncertain input following a transport failure: inspect the exact owned process and retained report first. The wrapper does not control or acquire a VM.

The base driver verifies all manifest files and the application's isolated-profile receipt before document input. An early physical Help toggle proves automatic checking OFF in the isolated profile before its scheduled check; failure to observe that within nine seconds stops the run. No user profile, printer configuration or real writing is modified. Source-composition hashes bind all five maintained sources and both generated files. The unchanged StageOnly receipt is copied into `driver/local-stage-receipt.json` during preparation and into `evidence/local-stage-receipt.json` at workflow entry; composition fields `localStageReceipt` and `localStageReceiptSha256` bind its name and exact bytes. The external original is preserved. Old frozen evidence stays intact.

## Observations and acceptance

`evidence/desktop-supplement-report.json` uses gate `windows-desktop-supplement`. It always retains `passed=false`, `qualified=false` and `qualificationPending=true`; `workflowPassed` requires the supplement and the complete normal-launch follow-through. `desktop-supplement-observations.json` contains the six ordered check IDs, source-composition binding, failure, capture references and explicit visual-review pending state. `center-native-observations.json` retains initial/before geometry, every post-Center sample, assertion status and failure.

The public fixture is `writing-normal-launch/desktop-public-review.md`, UTF-8 without BOM:

```markdown
<!-- Unicode witness: café 😀 -->

A flomptoid waits.

We recieve the mail.

A quorbleflat waits.
```

It ends with one newline. Both public files are checked byte-exactly after every supplemental workflow. US punctuation checks require the existing `04090409` keyboard layout and never change it. Next/Previous must select exactly `flomptoid` / `quorbleflat` through the owned focused Source TextPattern.

Review captures are bounded observations: initial, intermediate after 750 ms, and final after another 750 ms. These waits do not establish checker readiness or three findings. The report deliberately leaves `visibleCardCount` and `engineObserved` null. Independent visual review must establish the actual Harper 2.10.0 built-in label, all three fixture spelling cards in the final opening capture, and Review closed after the same toggle chord. Two visible cards cannot pass this criterion. A separate hash-bound review receipt must identify the exact report and opening/closing captures; pending or absent review cannot become full qualification.

Center first moves the owned ordinary window through a physically verified titlebar drag, then physically clicks Window → Center. Successful nonempty DWM visible-frame bounds must center within three physical pixels of the same monitor workarea on both axes. At least three identical native/DWM/monitor/DPI samples must remain centered for at least 300 ms. Native outer bounds remain diagnostics; they do not replace visible-frame geometry. The check also requires positive DPI, ordinary window state, movement, preserved outer and visible sizes, same monitor, exact Source selection and unchanged files. Failed observations are serialized before throwing.

Required captures include `desktop-open-path.png`, `desktop-palette.png`, `desktop-review-open.png`, `desktop-review-toggle.png`, `desktop-next-issue.png`, `desktop-previous-issue.png`, `desktop-center.png`, and `desktop-supplement-workflow.png`, with the matching UIA snapshots. Opening-dialog captures, updater prerequisite, ordinary launch/restore captures and launch logs are retained too. All report-listed evidence hashes must verify against the exact package and reconstructed generated sources. Do not rewrite raw report flags or substitute a capture from another run.

Print cancellation remains open: the observed modern printer dialog needs trustworthy lifecycle and ownership observations before a maintained continuation can safely choose Cancel. Clipboard/search follow-through, nested-menu completeness, multiple displays/scales and the remaining distribution and endurance matrix are separate qualifications. This supplement never touches the clipboard and does not resolve the earlier unknown restoration.

## Independent verification

`bin/windows/check-desktop-supplement.py` checks the exact package and StageOnly receipt, reconstructs both generated files from five maintained sources, validates evidence hashes, six ordered workflows, per-action witness times, focused issue selections, DWM geometry and the ordinary normal-launch Exit/reopen follow-through. It does not run the app or modify raw reports.

```sh
python3 bin/windows/check-desktop-supplement.py \
  --package /absolute/path/to/exact/package \
  --verify-report /absolute/path/to/run/evidence/desktop-supplement-report.json \
  --expected-executable 'C:\exact\local\package\Fomawrite.exe' \
  --source-root /absolute/path/to/exact/driver-source-snapshot
```

Omit `--source-root` only when the current maintained sources match the run byte for byte. Windows CI checks out these scripts as CRLF; the verifier preserves those bytes rather than treating an LF checkout as identical. Independent reconstruction from an exact CRLF snapshot matched both native preparation receipts and both generated file hashes.

A complete workflow without independent visual review exits **2**, with `workflowPassed=true`, `passed=false`, and `qualificationPending=true`. Structural failures exit **1**. A scoped pass exits **0** only when `--visual-review` supplies an independently reviewed JSON receipt containing:

- `schemaVersion: 1`, `gate: "windows-desktop-supplement-visual-review"`, `reviewedBy` and `reviewedUtc` after the run.
- `reportSha256` of the exact unchanged raw report.
- `captures` mapping `desktop-review-open.png` and `desktop-review-toggle.png` to their exact SHA-256 values.
- `engine: "Harper 2.10.0 (built-in)"`, `cards: ["flomptoid", "recieve", "quorbleflat"]`, and `reviewClosed: true`, all established by inspecting those exact captures.

Never create that receipt by copying the expected values without reviewing the images. The verifier cannot itself establish what the screenshots show. Its result always declares `scope: "review-center-supplement"` and `fullM2Acceptance: false`.

Forty-one supplemental fixture tests cover rejected/corrupted evidence and the valid scoped/pending paths, including invisible resize borders and exact CRLF composition. These are verifier tests, not app acceptance. They run alongside the existing deployment guards in Windows CI. The ten native preparation guards run separately and retain their own receipts.

## Remaining acceptance work

The VM was released after the overnight session on 22 October 2026; all work described above after release was host/CI work without VM input. Application source and the tested b056 packages remain unchanged.

1. After another unattended VM session is authorized, run the maintained composition against the exact verified local package with a fresh profile, then verify the full normal Exit/reopen follow-through.
2. Inspect the exact Review captures and record the separate visual receipt only if the label, three cards and closing behavior are visible. Two-card evidence stays pending.
3. Complete Print cancellation separately after observing ownership and lifecycle of the second Windows printer UI. Clipboard/search, nested menus, installer and wider display/endurance checks remain separate open work.

Full M2 qualification remains open. Current complete CI status is recorded in [STATUS.md](STATUS.md); a green preparation step does not establish that the entire run passed.
