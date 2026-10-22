# Windows Live typing, save and history acceptance

This supplemental gate tests actual Live text input, save, undo, redo, dirty state,
Source switching and normal reopen. Run it in addition to the
[ordinary six-stage gate](NORMAL-LAUNCH-GATE.md) and deployed document-view check.
The standard six-stage workflow and its input sequence are preserved; its
opened-document readiness check requires native picker dismissal and the exact
owned main window title before subsequent menu input.

`bin/windows/run-live-edit-guest.ps1` composes a disposable derivative of that
runner, inserting `live-edit-steps.ps1` immediately after the real Live render
proof and appending `live-edit-input.cs` to the guarded input helper. Composition
requires unique explicit anchors and fails if the base contract changes. The
exact generated scripts and source hashes are retained per case and independently
reconstructed by the offline verifier. No application, backend, bridge or editor
JavaScript calls perform an edit or simulate engine readiness.

## Required cases

Each case uses a fresh ordinary-startup isolated profile and disposable sample:

1. `settled`: type into Live; save; observe completed Undo before Save; observe
   completed Redo before Save.
2. `immediate`: send Ctrl+Z then Ctrl+S with no deliberate gap or completion wait.
3. `50ms`: the same pair with a requested 50 ms gap.
4. `100ms`: the same pair with a requested 100 ms gap.
5. `150ms`: the same pair with a requested 150 ms gap.

Every case requires the exact expected UTF-8 disk bytes, including newline/BOM
behavior, and a clean window title at three consecutive observations spanning
at least 250 ms after each save. It retains saved/undone/redone disk snapshots,
screenshots, UIA observations and actual shortcut timing. The final edited bytes
must survive Source switching, a clean exit and a second argument-free launch.

The pointer targets the OCR word `Saved` in the previously established exclusive
writing-pane crop. It must lie inside the owned application and pane. Read-only
UIA must confirm a visible, focused Edit control in the owned process before
input. All edits and shortcuts use Windows `SendInput`; OCR/UIA never invoke or
set application state. Process and foreground guards remain active.

Rapid cases insert no OCR, UIA readback or document-completion wait between Undo
and Save. The helper records monotonic times around both shortcuts. Immediate
requires an observed dispatch gap of at most 10 ms; requested nonzero gaps must
measure between their requested value and that value plus 40 ms. An overloaded
machine outside these bounds cannot qualify that timing case. These are helper
dispatch intervals, not measurements of physical keyboard delivery.

## Guest command

Use Windows PowerShell 5.1 in an unlocked English Windows desktop, with the
English Windows OCR component available. Close existing Fomawrite normally,
saving work first. Keep the VM's mouse and keyboard idle for the entire run.
Use a new output directory; retain the whole directory afterward.

In a fresh Windows PowerShell process, read the runner as a script block if
execution policy would otherwise block local/shared script files; no policy
change is required:

```powershell
$drivers = 'C:\Mac\Home\repo\projects\fomawrite_windows\bin\windows'
$runner = [ScriptBlock]::Create([IO.File]::ReadAllText((Join-Path $drivers 'run-live-edit-guest.ps1')))
& $runner -DriverDirectory $drivers `
    -Package 'C:\FomawriteQA\candidate\Fomawrite' `
    -Output 'C:\FomawriteQA\live-input-001'
```

When dispatched through Parallels, use a short hidden current-user PowerShell
invocation that reads a saved wrapper. Keep that process attached until it
finishes; do not detach the driver or encode the entire driver into its command
line. The runner creates fresh hidden PowerShell child processes for its cases.
It does not install tools, alter security policy, reset user preferences, or edit
user writing. Failure captures evidence (including the full exception chain in
`failure-exception.txt`) and the standard runner terminates only
its owned isolated application process. Other timing cases still run in their
own new profiles, so the first failure does not hide later results.

For development diagnosis only, `-Case settled` or `-Case immediate` selects one
case (also `50ms`, `100ms`, `150ms`). A single-case pass is explicitly insufficient
for full acceptance. The complete default run remains required on the final
package, with both architectures labelled according to checks actually run.

## Independent verification

Copy the entire output folder, including every `driver/` and `evidence/`
directory, and the exact package to the host. Use the matching reviewed checkout:

```sh
python3 bin/windows/check-live-edit.py \
  --package /path/to/copied/Fomawrite \
  --verify-series /path/to/copied/live-input-001/live-edit-series.json \
  --expected-executable 'C:\FomawriteQA\candidate\Fomawrite\Fomawrite.exe'
```

The verifier requires all five ordered cases, independent profiles, package and
source identities, precise target/focus, seven completed workflow stages, actual
rapid timings, stable clean saved observations, exact snapshots, normal
Source/reopen evidence and clean renderer logs. It writes
`live-edit-verification.json`. Failed or omitted rapid cases cannot qualify by
passing the settled baseline.

For a labelled diagnostic case only:

```sh
python3 bin/windows/check-live-edit.py \
  --package /path/to/copied/Fomawrite \
  --verify-case /path/to/copied/live-input-001/settled/evidence/live-edit-report.json \
  --case settled \
  --expected-executable 'C:\FomawriteQA\candidate\Fomawrite\Fomawrite.exe'
```

That receipt records `fullAcceptance: false` even when the single case passes.

Offline verifier rejection tests:

```sh
python3 -m unittest windows.test_live_edit windows.test_normal_launch windows.test_ci
```

Their fixture reports are unit-test inputs, not desktop acceptance evidence.
Windows CI runs the new rejection tests alongside the existing guards, with
explicit process exit checks. Actual desktop input remains a separate run.

## Known evidence boundary

Disposable earlier trials on `d573f16` ARM64 established settled typing/save,
undo/redo, exact Source and normal reopen. They reproduced rapid Ctrl+Z then
Ctrl+S leaving the restored text unsaved while disk retained the prior edited
text. A measured repeat had a 0.021 ms helper dispatch gap. That is a confirmed
product ordering defect; completion waits are not a repair or rapid acceptance.
The maintained gate must retain this failure until the product is fixed.

A whitespace-center click previously failed to focus Live, and one separate
attempt stopped on transient UIA discovery. Those failures remain preserved;
neither substitutes for an editing result. Final repaired-package execution of
all five cases remains necessary. This gate does not establish arbitrary existing
profiles, hardware keyboard behavior, localization, broader documents or full
daily-writing acceptance.

The first two maintained settled validation attempts on d573 stopped before
Live input: the existing bounded UIA search for the opened document threw while
the picker was already closed and Source visibly held the sample. Both failures
are preserved. No click or absence check was weakened, and retries do not count
as Live acceptance. The supplemental composition now retains full exception
chains for diagnosis. The captured inner exception was `RPC_E_DISCONNECTED`
(`0x80010108`) during a UIA `FindAll` call after the picker closed. The input
helper now restarts that entire search at most twice, within the original shared
4.5 second search budget, after revalidating both the native HWND/PID and UIA root
identity. Other exceptions, incomplete traversal and failed reacquisition remain
fatal. `windows/test_uia_retry.ps1` exercises the actual helper methods against a
provider stub, including unchanged-owner and deadline requirements; the previous
helper fails on the disconnected-provider case.

The next maintained attempt stopped at View Source while the native Open dialog
was still visible. Its document-name search matched the Library row behind the
modal dialog. This is preserved as a separate input-harness failure, not Live
acceptance. The opened-document readiness check now rejects this state and waits
for both native picker dismissal and the exact owned main window document title.
It retains the original Alt+N, typed path and Enter, with no additional click,
input retry or fixed sleep. The actual prior wait fails the sidebar/modal negative
regression. Maintained validation on the same d573 ARM64 executable then passed the settled
case and its independent verifier (seven stages, 39 evidence hashes, two ordinary
launches). The immediate case failed at Undo then Save with an observed 0.000125 ms
helper dispatch gap: Live visibly showed the undone text, while the title stayed
dirty and disk retained the edited suffix through the 20 second deadline. Its
independent verifier correctly rejected it. The three delayed rapid cases remain
unrun on this old candidate; all five require a final repaired-package run.
