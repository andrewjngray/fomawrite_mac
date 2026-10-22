# Ordinary Windows launch acceptance

`--check-document-views` exercises a bounded in-process diagnostic. Its success
cannot establish ordinary startup, native picker dispatch or Windows input.
`bin/windows/run-normal-launch-guest.ps1` is a separate required desktop gate for
that behavior. Run both gates against the same deployed package.

Run this in Windows PowerShell 5.1 in an unlocked, interactive English Windows
desktop. Do not use the mouse or keyboard during the run. The runner needs no Qt
SDK, Python or PowerShell 7. Use a new output directory on each run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\bin\windows\run-normal-launch-guest.ps1 `
    -Package C:\FomawriteQA\candidate\Fomawrite -Output C:\FomawriteQA\normal-launch-001
```

The runner validates every deployed manifest file, strips inherited Qt/QML/app
overrides and developer PATH entries, then launches `Fomawrite.exe` twice with
**no arguments**. It requires an app-written `profile/profile.json` isolation
receipt before sending any input. `FOMAWRITE_PROFILE_ROOT` chooses isolated
native settings, data, cache and IPC namespaces while leaving the normal
startup/session/restoration code active. The same profile is reused on reopen.
A candidate that lacks this isolation contract fails before UI input.

The gate uses read-only Windows UI Automation to discover visible controls and
read their bounds, checked states and editor text. Pointer clicks and keyboard
input enter through Windows `SendInput`; there are no QML calls, Qt test events,
UIA Invoke/Value setters or command-dispatch shortcuts. It requires all of:

1. An ordinary application window with acknowledged isolation.
2. The organizer's **Add library location** button opening a native Windows
   folder picker. Selecting the new writing location must reveal the sample in
   the Library.
3. **File → Open File…** opening a native file picker. The chosen document must
   appear in Source with its exact sample text.
4. Windows keyboard input replacing that sample and **File → Save** persisting
   the exact expected Markdown to disk.
5. The **Live** and **Source** footer buttons changing their exposed checked
   states and preserving the exact document source.
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
timestamps inside the run, exact package/executable/driver identities,
app-written isolation receipt, screenshots, UIA/native-picker observations,
launch logs and byte-exact expected saved sample (allowing Windows line
endings). All artifacts are hash-bound to the report. Its focused report
rejection tests run with `python3 -m unittest windows.test_normal_launch`.
Those tests protect the verifier; only a real guest run establishes UI behavior.

Run Windows-local NTFS and Mac-shared storage as separate candidates/evidence
runs when qualifying both locations. This small gate does not establish
hardware input, arbitrary existing user profiles, broader dialogs, localization,
DPI, installer or full daily-writing acceptance.
