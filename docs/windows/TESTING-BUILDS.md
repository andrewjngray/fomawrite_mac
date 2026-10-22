# Windows testing builds

## Current location

Mac/shared collection folder:

`/Users/andrewgray/repo/projects/fomawrite_windows/dist/windows-testing`

In Windows, navigate to that same shared checkout and open `dist`, then `windows-testing`. Start with `START-HERE.txt`.

The shelf contains the original verified ZIPs **and already unpacked application folders**:

- `builds/b45cb34-arm64/Fomawrite/Fomawrite.exe` — native Apple Silicon Parallels target.
- `builds/b45cb34-x64/Fomawrite/Fomawrite.exe` — x64 comparison under emulation in the ARM VM.

**These are the original `b45cb34` diagnostic packages. No fixed or qualified package is promoted here.** On the Mac share, ordinary launch repeatedly failed to load the Live page and a QtWebEngine renderer crash was observed. The executable hash matched the original ARM64 package. A same-hash Windows-local copy completed real File Open, View Source and File Save actions. The full failure mechanism remains unresolved; those successful actions do not establish complete normal-startup, Live, folder or restore acceptance.

The newer `d8b8580` packages belong only in `diagnostic-candidates/<commit>-<architecture>/`. [Their CI run](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39736778817) is green on both architectures, but they add normal-startup isolation only. They contain neither the current bounded Live recovery changes nor the new local portable starter. Do not label them fixed, accepted or latest recommended builds. See [M2 repair evidence](M2-REPAIR-EVIDENCE.md).

## Windows-local starter under development

The working portable packaging change adds `START-FOMAWRITE.cmd` and a manifest-verifying local starter. It stages the whole manifest-listed application in a versioned `%LOCALAPPDATA%\Fomawrite\Portable` cache, verifies the copy and launches it through normal startup. Its 9 synthetic fixture checks passed without launching the app. Real UNC/shared-folder starter acceptance and the full [ordinary launch gate](NORMAL-LAUNCH-GATE.md) remain pending. The starter is not present in the original shelf packages or the `d8b8580` diagnostic packages. See [starter behavior and limits](PORTABLE-STARTER.md).

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
- Update `BUILD-CATALOG.json`, `COPY-VERIFICATION.json` and `START-HERE.txt` only when intentionally adding a candidate to the shelf after identity verification. Diagnostic-only staging under `diagnostic-candidates/` does not promote a build or update those public shelf pointers. Keep diagnostic, failed and accepted states explicit.
- `dist/` is ignored by Git. Packages and local result notes stay local; publish only reviewed status/evidence in `docs/windows/` and CI artifacts.
- The shelf has **one code version in two architectures** (`b45cb34`). Separately staged diagnostic revisions are not accepted shelf versions.

## Next Windows work

Reproduce and fix the reported failure through normal startup, Windows input and native file/folder dialogs; add an acceptance gate for those paths before recommending a new candidate. Normal startup/restore, the installer and broader desktop qualification remain open. See [Windows status](STATUS.md).
