# Cycle 163 — Windows development launch and updates

31 October 2026, Pacific/Auckland. Scoped launcher maintenance; no app source,
embedded editor or package bytes changed, and primary Mac master/apps untouched.
Current Windows package identity and remaining qualification limits are in
[Windows status](../../docs/windows/STATUS.md).

## Changes

- Root **Launch.cmd** selects the current downloaded native architecture, verifies
  files, stages on local NTFS and uses normal app startup/IPC. The old root
  TEST-FOMAWRITE.cmd is an alias. Explicit b056 launchers remain rollback options.
- Mac **Launch.command** checks the source branch and latest successful complete
  Windows CI packages, refreshes both architectures, starts the selected Parallels
  VM if needed and opens the app. **Update.command** refreshes without opening.
  Dirty source trees are preserved, and clean source sync permits only fast-forward.
- Windows Launch deliberately has no GitHub dependency: each click checks the
  downloaded selection. Mac downloads use existing gh authentication without
  copying credentials to the VM. PR/fork and diagnostic-only runs are excluded.
- GitHub ZIP digests, nested archive paths, package file hashes, architecture and
  commit identities are checked before atomic publication. Failed updates retain
  the previous selected packages. Versioned executable caches are never overwritten.
- Workspace identity includes the executable path in the existing app. The launcher
  copies missing saved workspace files once, byte-for-byte, from its prior launch
  record. Existing destination layouts and all recovery/documents remain untouched.
  An older running managed build blocks the switch until the user closes it normally.

## Verification

- Python updater and Mac orchestration fixture tests: **22 passed**. The updater
  trust regression failed on the previous selector for eight PR/fork/missing or
  malformed repository-metadata cases, then passed with strict origin/event checks.
- Windows PowerShell 5.1: **19 checks passed**, using synthetic packages and mocked
  process starts. Tests cover cache switching, hash/path rejection, offline launch,
  process/version guards, quoting and workspace preservation.
- The first native run exposed PowerShell 5.1 converting a null File.Replace backup
  argument to an empty path. The failing record is retained; explicit NullString
  fixed it and the replacement regression passed. A separate fixture expectation
  was corrected to compare resolved FullName rather than Windows short TEMP spelling.
- Both architecture packages from full CI run 39845554167 downloaded and verified.
  Real ARM64 staging passed; Mac launch started the exact new local executable.
  Root Launch.cmd opened START-HERE.md through IPC in the same responsive process.
  Prior b056 workspace identity was checked against an existing real Qt workspace;
  the initial transfer copied that existing empty window list successfully.
- The previous full Windows CI for these application bytes passed 390 Qt tests per
  architecture, 3 appearance checks, 124 deployed scenarios and 210 guards. Those
  historical results are not represented as a fresh full desktop run here.
- **Final focused CI passed:** [run 41093374244](https://github.com/andrewjngray/fomawrite_mac/actions/runs/41093374244) at `f4d00bb`: 22 Python tests and 19 native Windows fixture checks. The first CI run exposed Windows ZIP-name normalization; original-name validation and a portable regression corrected it. Both records are retained. Root Update.command also completed against the final history, retaining the latest complete application build rather than selecting a launcher-only run.
- Focused launcher CI is invoked with windows.yml input `launcher_only=true`, so
  application builds and artifact promotion are skipped. This prevents launcher-only
  verification runs from being selected as downloadable application builds.

Raw local logs (including failed attempts) are under `evidence/`; only this reviewed
summary and verified-builds.json are committed. See START-HERE.md for user instructions.

## Limits

ARM64 launch and document-open were exercised in Andrew's Windows 11 Parallels VM.
The x64 package was downloaded/hash-verified here, not launched afresh. General app
acceptance limitations from Cycle 162 remain. This does not add an installer or a
signed public app updater; macOS file associations from build 154 remain Mac-only.
