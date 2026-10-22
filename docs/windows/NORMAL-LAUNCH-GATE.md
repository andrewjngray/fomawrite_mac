# Ordinary Windows launch acceptance

`--check-document-views` exercises a bounded in-process diagnostic. Its success
cannot establish ordinary startup, native picker dispatch or Windows input.
`bin/windows/run-normal-launch-guest.ps1` is a separate required desktop gate for
that behavior. Run both gates against the same deployed package.

Run this in Windows PowerShell 5.1 in an unlocked, interactive English Windows
desktop. Do not use the mouse or keyboard during the run. The runner needs no Qt
SDK, Python or PowerShell 7. Use a new output directory on each run:

Copy the current driver and `normal-launch-input.cs` into a local gate
folder. Restricted script execution policy can be retained: load the driver as
a script block and supply its actual directory explicitly:

```powershell
$gateDirectory = 'C:\FomawriteQA\gate'
$driver = [ScriptBlock]::Create([IO.File]::ReadAllText((Join-Path $gateDirectory 'run-normal-launch-guest.ps1')))
& $driver -DriverDirectory $gateDirectory `
    -Package C:\FomawriteQA\candidate\Fomawrite -Output C:\FomawriteQA\normal-launch-001
```

For Parallels automation, execute these commands inside
`powershell.exe -NoProfile -WindowStyle Hidden -Command <commands>`.
`-DriverDirectory` supplies the helper and driver paths used for loading and
report hashes when ScriptBlock execution has no `PSScriptRoot`/`PSCommandPath`.
Direct `-File` invocation can use its default directory when existing execution
policy permits it; the gate requires no execution-policy change.


`-WindowStyle Hidden` is required when invoking PowerShell through Parallels
`prlctl exec --current-user`: a visible PowerShell console can steal foreground
from the application or native picker. Keep that PowerShell process alive for
the complete driver run; a bare detached `Start-Process` is insufficient.

The runner validates every deployed manifest file, strips inherited Qt/QML/app
overrides and developer PATH entries, then launches `Fomawrite.exe` twice with
**no arguments**. It requires an app-written `profile/profile.json` isolation
receipt before sending any input. `FOMAWRITE_PROFILE_ROOT` chooses isolated
native settings, data, cache and IPC namespaces while leaving the normal
startup/session/restoration code active. The same profile is reused on reopen.
A candidate that lacks this isolation contract fails before UI input.

The gate uses read-only Windows UI Automation to discover visible controls and
read their bounds, editor visibility and Source text. Discovery uses bounded
breadth-first child queries with runtime-ID deduplication and avoids browser
subtrees for menu/toolbar/Source searches. Pointer clicks and keyboard
input enter through Windows `SendInput`; there are no QML calls, Qt test events,
UIA Invoke/Value setters or command-dispatch shortcuts. The current PowerShell
thread must enter verified per-monitor-v2 DPI awareness. Pointer placement uses
`SetPhysicalCursorPos` and verifies the result with `GetPhysicalCursorPos`, so
UIA's physical bounds remain physical at 200% desktop scaling. Clicks recheck
foreground after accessibility queries, and Unicode input rechecks it for each
character pair. It requires all of:

1. An ordinary application window with acknowledged isolation.
2. The organizer's **Add library location** button opening a native Windows
   folder picker. Selecting the new writing location must reveal the sample in
   the Library. The driver waits for the confirmation button to become enabled
   after shell navigation, rather than assuming a fixed delay is sufficient.
3. **File → Open File…** opening a native file picker. The chosen document must
   appear in Source with its exact sample text.
4. Windows keyboard input (Shift+Enter for each literal Source newline) replacing that sample and **File → Save** persisting
   the exact expected Markdown to disk.
5. The **Live** footer button hiding the Source editor and changing the actual
   editor-mode appearance label. Built-in English Windows OCR must recognize
   the sample heading and saved sentence in a screenshot cropped to the prior
   Source writing rectangle, excluding Library excerpts and publishing output.
   The **Source** footer button must then reveal Source with exact text.
   Qt's mode buttons expose no Toggle/Selection UIA pattern, so checked-state
   labels cannot stand in for this visible editor and rendered-content proof.
6. **File → Exit** ending the process successfully, then a second argument-free
   ordinary launch restoring the saved document. The second process must also
   exit successfully through the menu.

A missing control, unreadable UIA state, missing native dialog or ineffective
input fails the gate. It has no reduced-check or coordinate fallback. Failure
captures the current screenshot/UIA tree and terminates only the isolated
process spawned by this run. The disposable evidence/profile/writing remain
for inspection; it does not remove user files or settings.

Copy the complete output directory and exact deployed package back to the host.
Verify them independently (the input driver/helper must match this checkout):

```sh
python3 bin/windows/check-normal-launch.py \
  --package /path/to/copied/Fomawrite \
  --verify-report /path/to/copied/normal-launch-001/normal-launch-report.json \
  --expected-executable 'C:\FomawriteQA\candidate\Fomawrite\Fomawrite.exe'
```

The verifier requires every ordered stage, two clean argument-free launches,
timestamps inside the run (including .NET seven-digit fractions on Python 3.9),
exact package/executable/driver identities,
app-written isolation receipt, screenshots, UIA/native-picker observations,
Live writing-region screenshot/OCR proof, clean renderer/page-load logs and
byte-exact expected saved sample (allowing Windows line
endings). All artifacts are hash-bound to the report. Its focused report
rejection tests run with `python3 -m unittest windows.test_normal_launch`.
Those tests protect the verifier; only a real guest run establishes UI behavior.

Run Windows-local NTFS and Mac-shared storage as separate candidates/evidence
runs when qualifying both locations. This small gate does not establish
hardware input, arbitrary existing user profiles, broader dialogs, localization,
DPI, installer or full daily-writing acceptance.

The non-UI `windows/test_normal_launch_wait.ps1` fixture exercises the actual
driver wait/selection logic for delayed readiness, timeout and process exit.
It accepts `-DriverPath` to verify red/green against a saved earlier driver.
Its three checks complement the report verifier and actual desktop runs.
