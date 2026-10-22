# Windows testing builds

## Current location

Mac/shared collection folder:

`/Users/andrewgray/repo/projects/fomawrite_windows/dist/windows-testing`

In Windows, navigate to that same shared checkout and open `dist`, then `windows-testing`. Start with `START-HERE.txt`.

The shelf contains the original verified ZIPs **and already unpacked application folders**:

- `builds/b45cb34-arm64/Fomawrite/Fomawrite.exe` — native Apple Silicon Parallels target.
- `builds/b45cb34-x64/Fomawrite/Fomawrite.exe` — x64 comparison under emulation in the ARM VM.

**Both are the existing diagnostic candidate. There is no newer fix here.** Andrew reported that ordinary launch opens menus but menu actions, buttons and folder navigation do not work. The running executable matched the tested ARM64 hash. It was launched from `C:\Mac\Home\Desktop\Fomawrite_windows\Fomawrite\Fomawrite.exe`. The cause is not confirmed. The prior checker used fresh isolated settings and synthetic events, bypassing normal startup and IPC; its passes do not resolve this report.

## Run a version

1. Close any other Fomawrite Windows instance normally, saving work first.
2. In Windows File Explorer, enter `%LOCALAPPDATA%` in the address bar. Create `FomawriteTests`, then a folder named `b45cb34-arm64` (or the version/architecture being tested).
3. Copy the **whole `Fomawrite` folder** from the selected shelf version into that new local folder. Keep all DLLs, resources and subfolders.
4. Open the local `Fomawrite` folder and double-click `Fomawrite.exe`. Do not add checker flags: this test must use ordinary startup and real interaction.
5. Copy `TEST-RESULTS-TEMPLATE.md` from the shelf into its `results` folder with a unique date/build name. Record path, architecture, storage type and exactly which action fails.

The initial destination is `%LOCALAPPDATA%\FomawriteTests\b45cb34-arm64\Fomawrite\Fomawrite.exe`. Windows-local storage gives a controlled comparison with the earlier automated runs. It is not a promised fix. A later shared-folder launch should be a separately labelled comparison.

Different app directories may still share preferences and themes; copying an app does not isolate or reset user data. Preserve existing settings and recovery. Use sample documents. A separate Windows test account is suitable for a truly separate normal profile if needed later.

## Maintaining the shelf

- Keep builds in new `<commit>-<architecture>` folders. Never overwrite a running or previously reviewed version.
- Identify builds by commit, architecture and hashes, not only the inherited application version string.
- Place each original ZIP beside its extracted `Fomawrite` folder; verify ZIP, executable and every package-manifest file hash.
- Update `BUILD-CATALOG.json`, `COPY-VERIFICATION.json` and `START-HERE.txt` when a new candidate is added. Keep diagnostic, failed and accepted states explicit.
- `dist/` is ignored by Git. Packages and local result notes stay local; publish only reviewed status/evidence in `docs/windows/` and CI artifacts.
- The shelf currently has **one code version in two architectures**, not two different revisions. Older failed candidates are not promoted into it.

## Next Windows work

Reproduce and fix the reported failure through normal startup, Windows input and native file/folder dialogs; add an acceptance gate for those paths before recommending a new candidate. Normal startup/restore, the installer and broader desktop qualification remain open. See [Windows status](STATUS.md).
