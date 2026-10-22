# Windows testing builds

## Choose a build from the current status

For this checkpoint, the explicit internal-review entry is **`TEST-B056-ARM64.cmd`**, in the collection folder below. On Windows use:

`C:\Mac\Home\repo\projects\fomawrite_windows\dist\windows-testing\TEST-B056-ARM64.cmd`

This launches the exact ordinary-tested b056 ARM64 package through its manifest-verifying local starter. It is an internal test candidate, with [bounded passes and remaining qualification](M2-DESKTOP-B056.md), not a release. `TEST-B056-X64.cmd` selects the separately tested x64-emulated package. Neither entry overwrites a historical package or changes ordinary user preferences.

Read [Windows status](STATUS.md) before choosing a package. It records the exact candidate, CI results, ordinary VM acceptance and any promotion. The top-level starter and catalog can still point to an older shelf build; they are not a reliable indication of the newest tested code.

The historical `d573f16` shelf passed its original checks, but a later selected-Live-formatting test exposed a rapid Save defect. **Do not treat that shelf as the current qualified build.** New repair packages stay in `diagnostic-candidates/` until their exact identities and required acceptance checks are reviewed. A successful build alone does not promote one.

The Mac/shared collection folder is:

`/Users/andrewgray/repo/projects/fomawrite_windows/dist/windows-testing`

The historical shelf packages are already unpacked in `builds/d573f16-arm64/Fomawrite/` and `builds/d573f16-x64/Fomawrite/`, each with its original portable ZIP and copy verification. The older `b45cb34` shelf packages and `diagnostic-candidates/` are retained for comparison. Use the exact package identified in current Windows status; do not assume the top-level starter points to it.

## Run on the Parallels ARM64 VM

After a candidate is explicitly promoted, the shared-folder ARM64 entry is **`START-WINDOWS-ARM64.cmd`**. Until then it points to the historical shelf described above. Its Windows path is:

`C:\Mac\Home\repo\projects\fomawrite_windows\dist\windows-testing\START-WINDOWS-ARM64.cmd`

For an explicitly identified unpacked candidate, use its own **`Fomawrite\START-FOMAWRITE.cmd`**. It verifies and stages that exact package locally. No ZIP extraction or manual DLL copying is needed.

1. Close other Windows Fomawrite instances normally, saving work first.
2. Open the exact ARM64 candidate folder given in current Windows status and use its packaged starter; use the top-level starter only after explicit promotion.
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

The current candidate’s completed and pending checks are recorded in Windows status; broader Windows-versus-Mac parity work continues. Physical x64 hardware, installer and wider desktop qualification remain open. [M2 repair evidence](M2-REPAIR-EVIDENCE.md#remaining-qualification) records the exact completed checks and limits.
