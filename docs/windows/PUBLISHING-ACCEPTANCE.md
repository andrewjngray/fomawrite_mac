# Ordinary Windows publishing acceptance

22 October 2026. This supplements the six-stage normal-launch gate. It exercises the packaged application on an unlocked Windows desktop using physical `SendInput` clicks/keys and read-only UI Automation discovery. It never invokes Backend/QML test methods, runs a diagnostic mode or edits a user document.

## Running it

Use a verified Windows-local portable package, a fresh output directory and a fresh hidden PowerShell 5.1 process. Keep that parent process alive until the run exits. The helper is `bin/windows/normal-launch-input.cs`; the driver is `bin/windows/run-publishing-guest.ps1`. Pass `-Package`, `-Output`, `-HelperDirectory` and the actual `-DriverPath`. A ScriptBlock invocation can read the saved driver without changing execution policy. `-PrepareOnly` only writes fixtures; it cannot establish a pass.

The driver hashes all manifest entries and the executable, starts the app without arguments, and validates its isolated native settings/workspace receipt before sending input. Two new Markdown fixtures contain different title/body sentinels. All tested writing and exported files live in this disposable profile/run. Foreground and process identity guards prevent input or capture against another app. Failure cleanup affects only the process started by the driver.

## Nine workflow stages

1. Ordinary startup and profile/package verification.
2. Native Library folder picker, document A, Split layout and Web preview.
3. Document A in PDF preview.
4. Library switch to B; both PDF and Web must show B and exclude A.
5. Select Modern (Sans), then Classic (Serif), reload themes and preview; B must remain visible and the rendered style must change.
6. File → Export HTML, export hub and native Save dialog. Inspect the actual output for B, absence of A and selected Georgia CSS.
7. File → Export PDF and native Save dialog. Decode the saved one-page PDF with Windows PDF APIs and OCR its rendered page for B and absence of A.
8. Switch back to A in Web and PDF. Both original Markdown files must retain their exact SHA-256.
9. File → Exit must terminate normally with status zero and no detected fatal renderer/page-load failure.

Publishing OCR is cropped strictly to the right of the observed Source editor bounds. Eight observations require both the expected heading and body and reject the other document. The driver also records footer button bounds, images, timestamps, UIA state availability and file hashes. It requires exact native filename-field readback before pressing Save.

## Workflow success is not visual qualification

On the tested Qt Windows build, Web/PDF buttons expose Invoke and empty Value patterns, without a readable checked-state pattern. PDF page objects also are not exposed reliably. Therefore `workflowPassed: true` leaves `passed: false`, `qualified: false`, `qualificationPending: true` and `manualFormatReview: required`.

A reviewer must inspect all eight hashed `*-format-review.png` captures, confirm the selected Web/PDF button and continuous-Web versus bounded-paper layout, and verify document identity. Record that review separately, keyed to the original report hash; never overwrite the driver's pending flags or pretend OCR alone proves the output format. Typography must be inspected too: the first successful workflow exposed Modern's incorrect serif fallback on Windows, despite passing text/content checks.

This short-fixture workflow does not establish rapid-switch race coverage, rich Markdown/theme asset fidelity, flicker endurance, resize/zoom, every export failure/cancel case, or full W04–W08 acceptance. Those remain separate tests. The 124-scenario deployed diagnostic is also separate and uses synthetic input.

## First completed run and retained failures

Package `d573f1666d68efaee0b60f43f04d3fe1e96781c4`, ARM64, Windows 11 ARM64 Parallels guest at 200% scaling. The first completed run is under:

`dist/windows-testing/results/publishing-d573-825793c5e1db41ffaa5cc143be7fa1e9/evidence/`

Report SHA-256: `641407b1b7ecd4c3c40d16b2a695a8db7dfdc4dd0923f760c949165415eb171e`. All nine workflow stages passed; all eight publishing OCR observations contained the correct document. HTML and PDF exports succeeded and the app exited normally. Root reviewed all eight format captures: output format, selected controls and document identity were correct. Modern's visible serif fallback is an open product finding, so this is not blanket visual acceptance.

Earlier trials are retained under `results/publishing-d573-*`: missing UIA view/checked-state assumptions, an unavailable managed accessibility pattern and a native Save dialog input/readback mistake were harness failures. None is presented as an application regression. The successful report retains one `Compositor returned null texture` warning alongside successful output checks; it does not prove absence of transient flicker. Its Parallels wrapper returned 255 after copying the completed evidence, whereas the owned app's exit status was zero. The report and copied hashes, rather than the wrapper status alone, establish the workflow result.

Raw desktop evidence remains ignored and local. Repeat this driver and visual review against each final candidate; a pass on d573 does not qualify later binaries.
