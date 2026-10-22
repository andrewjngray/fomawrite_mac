# Fomawrite Windows — manual testing

This is an internal candidate, not a finished Windows release. Start with disposable Markdown files. Current build identity and unresolved automated failures are in [Windows status](docs/windows/STATUS.md); the full handoff is [WINDOWS-REVIEW-HANDOFF.md](WINDOWS-REVIEW-HANDOFF.md).

## Launch

1. In Windows Explorer open `C:\Mac\Home\repo\projects\fomawrite_windows`.
2. Double-click **TEST-FOMAWRITE.cmd**. It verifies the ARM64 package and runs it from Windows-local storage. The first launch can take longer while staging/verifying.
3. Keep the package folder intact. Do not run a loose EXE directly from the shared folder, and do not use the older `START-WINDOWS-ARM64.cmd` shelf entry.

## First pass — about 15 minutes

| Check | Expected result | Result / notes |
| --- | --- | --- |
| Folder and file controls | Add a sample library folder and open a Markdown file using toolbar and File menu. Controls respond. | |
| Source edit and Save | Type a unique sentence; Ctrl+S; reopen. Exact text remains, including Markdown markers and Unicode. | |
| Live edit and immediate Save | Switch to Live, edit and apply Bold, immediately Ctrl+S. Reopen and inspect Source. Both edit and formatting persist. | |
| Undo / Redo | Undo the last change, save; redo and save. Saved text matches each visible state. | |
| Switch files | Alternate between two obviously different documents. Editor, filename and preview always refer to the same document. | |
| Web / PDF | Switch both formats and select two output themes. Correct content appears without blank, stale or flickering output. | |
| Export | Export public samples to HTML/PDF, open the files independently and compare content. | |
| Themes | Import a sample CSS theme, select it, edit its CSS externally and reload. Record any failure or delayed update. | |
| Review | Open/close Review; inspect spelling findings and Next/Previous navigation. | |
| Find and clipboard | Find text; test Copy/Paste using disposable content. Keep any important clipboard content elsewhere first. | |
| Close and reopen | Save, close normally and reopen. Confirm the correct file and saved text return. Check unsaved-change prompts separately on a sample. | |

Print, complex nested menus, recovery, multiple displays/scales and installation lifecycle still need qualification. Avoid sending a real print job just to test whether Cancel works.

## Report a failure

Record the build/commit from [Windows status](docs/windows/STATUS.md), ARM64 or x64, launch path, exact steps, expected/actual behavior, and a screenshot. If possible include a small public Markdown sample. Distinguish a blank preview, wrong document, input failure and saved-file mismatch. Preserve the original failing file and logs; do not repeatedly overwrite it trying to recover.

Save notes under `dist/windows-testing/results/` or share them with the reviewing developer.
