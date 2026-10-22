# Windows testing builds

## Current repair build

Current repair code is **`d573f1666d68efaee0b60f43f04d3fe1e96781c4`**. ARM64/x64 CI and strict package identities passed. The final ARM64 standard centre-click VM gate, shared-document supplement and packaged starter passed with offline verification. The separate ARM64 guest document-view diagnostic passed 124 scenarios with no failures or QML warnings. Verified packages are now promoted to the testing shelf. x64 has no final VM normal-launch run. See [Windows status](STATUS.md) and [repair evidence](M2-REPAIR-EVIDENCE.md).

The Mac/shared collection folder is:

`/Users/andrewgray/repo/projects/fomawrite_windows/dist/windows-testing`

The current packages are already unpacked in `builds/d573f16-arm64/Fomawrite/` and `builds/d573f16-x64/Fomawrite/`, each with its original portable ZIP and copy verification. The older `b45cb34` shelf packages and `diagnostic-candidates/` are retained for comparison; use the current starter below.

## Run on the Parallels ARM64 VM

Double-click **`START-WINDOWS-ARM64.cmd`** in the shared testing folder. In Windows its path is:

`C:\Mac\Home\repo\projects\fomawrite_windows\dist\windows-testing\START-WINDOWS-ARM64.cmd`

No ZIP extraction or manual DLL copying is needed.

1. Close other Windows Fomawrite instances normally, saving work first.
2. Open the shared `dist/windows-testing` folder in Windows File Explorer and use the promoted ARM64 starter.
3. Allow the first preparation to finish. The manifest-verifying starter copies the selected package into a versioned Windows-local cache, checks it and launches ordinary Fomawrite. First staging from a share can take several minutes; later launches verify and reuse that cache.
4. Use sample writing and record the exact build, launch path, architecture and any failed action in a fresh copy of `TEST-RESULTS-TEMPLATE.md` under `results`.

Each newly packaged application also includes **START-FOMAWRITE.cmd** beside `Fomawrite.exe`. The final ARM64 packaged command file passed an actual ordinary startup with an isolated native profile from the share into the verified local cache and normal File → Exit. Its behavior and evidence are documented in [portable starter](PORTABLE-STARTER.md). The original `b45cb34` and isolation-only `d8b8580` packages predate those packaged starters.

## Evidence boundaries

The original `b45cb34` ARM64 executable repeatedly failed to load Live from Mac-shared storage; a QtWebEngine renderer crash was observed. The same executable from local NTFS completed native file actions. An unobstructed-edge diagnostic later completed the local workflow and Live OCR, while the ordinary Add button centre was intercepted by an invisible scrollbar. These observations motivated the repairs. The final ARM64 standard-centre run is separate and passed; its retained first attempt stopped before workflow input because an older dialog blocked foreground activation. After authorized normal closure and confirmation that the old process was absent, the unchanged gate passed.

A local starter runs verified binaries from `%LOCALAPPDATA%\Fomawrite\Portable`. Shared-source staging and direct shared-executable launch are different tests and need separate labels. The successful starter workflow does not establish that direct shared-folder WebEngine execution is fixed.

Different application directories may share ordinary preferences and themes. Preserve settings, recovery and unsaved work. The automated normal-launch gate uses an explicit isolated profile; a manual launch does not reset user data.

## Maintaining the shelf

- Add each build under a new `<commit>-<architecture>` directory; never overwrite a running or previously reviewed version.
- Identify builds by commit, architecture and hashes, not only the inherited application version label.
- Keep each original ZIP beside its extracted application folder and verify ZIP, executable and every package-manifest file.
- Update `BUILD-CATALOG.json`, `COPY-VERIFICATION.json`, `START-HERE.txt` and user starter pointers only as an intentional promotion after identity and acceptance checks. Separate diagnostic staging does not update those pointers.
- Keep diagnostic, failed and accepted states explicit. `dist/` remains local/ignored; commit only reviewed summaries and evidence records.

## Remaining qualification

The ARM64 repair checkpoint is available for testing; broader Windows-versus-Mac parity work continues. Final x64 VM normal-launch acceptance, installer and wider desktop qualification remain open. [M2 repair evidence](M2-REPAIR-EVIDENCE.md#remaining-qualification) records the exact completed checks and limits.
