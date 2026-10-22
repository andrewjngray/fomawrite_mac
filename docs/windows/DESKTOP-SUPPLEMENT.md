# Review and Center desktop supplement

This maintained driver covers six physical Windows input workflows: Open by Path cancellation, Command Palette cancellation, Review toggle, Next Issue, Previous Issue, and Window → Center. It composes the current normal-launch driver and helper; it returns to that driver for Source preservation, normal Exit, ordinary relaunch and restored-document checks. It makes no Print claim.

This is a reproducible **future acceptance harness**, not new qualification of the b056 package. No interactive desktop execution, native PowerShell parse or C# compilation has been performed for this maintained composition. The historical failures in [M2-DESKTOP-B056.md](M2-DESKTOP-B056.md) remain unchanged. This covers parts of [TEST-MATRIX.md](TEST-MATRIX.md) W11/W13/W14; the wider matrix remains applicable.

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

## Resume checklist — draft, not an acceptance gate

The overnight VM was released on 22 October 2026; this draft was prepared on the host afterwards without sending any more VM input. The 87 existing Python deployment/normal-launch/Live/installer guards were rerun successfully. They do not validate this new composition. No application source, packaged build or CI wiring changed in this draft.

Before using this as an acceptance gate:

1. Parse the wrapper, steps and generated PowerShell with native Windows PowerShell 5.1 and compile the combined C# helper; retain exact source hashes. Use CI for this preparation without taking over the user desktop.
2. Add an independent Python verifier and negative fixtures. Reconstruct the generated sources from their five recorded inputs, verify every evidence hash and copied staging receipt, and run the existing normal-launch verifier with the additional workflow stage. Never change raw pending flags.
3. Require six ordered successful workflows, exact fixture bytes, owned and focused next/previous selections, successful DWM geometry, preserved monitor/size, and three stable centered samples spanning at least 300 ms. Use the retained R8 outer-border discrepancy as a geometry fixture.
4. Require a separate review receipt bound to the exact raw report and opening/closing captures before claiming Review acceptance. It must show the Harper label and all three fixture spelling cards. Missing or two-card evidence remains pending.
5. Add those rejection fixtures and native parse/compile validation to CI, then request a fresh unattended VM session for actual execution and normal exit/reopen. Do not transfer old package evidence to a newly stamped build.
6. Complete Print cancellation separately after observing the ownership and lifecycle of the second Windows printer UI. Clipboard/search, nested menus, installer and wider display/endurance checks remain separate open work.

This draft is intentionally not wired into release acceptance until the above validation is implemented. The source wrappers fail closed on identity/composition mismatches, but independent verifier coverage is still missing.
