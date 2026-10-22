# Windows-local portable starter

New portable packages contain **START-FOMAWRITE.cmd** beside `Fomawrite.exe`.
Double-click that starter when the extracted package is on a Mac shared folder.
It verifies the package and runs a local copy from:

```text
%LOCALAPPDATA%\Fomawrite\Portable\<architecture>-<commit-prefix>-<manifest-hash-prefix>\Fomawrite.exe
```

The starter checks every manifest-listed file before staging. It copies only
those files and `manifest.json` into a unique temporary directory, verifies the
copy, then publishes it with a same-parent directory rename. A file lock
serializes simultaneous starters. On later launches it verifies and reuses the
same cache. Different manifests receive different directories. Existing cache
versions and writing files are never overwritten; a corrupt existing cache is
rejected with an error rather than replaced beneath a running app.

Both starter files are copied into the package before manifest creation, so the
package manifest binds their hashes too. The command file reads its adjacent
PowerShell script as a script block using a process-local directory variable.
It does not change execution policy, registry settings, persistent environment
variables or Chromium sandbox settings. Script errors produce a Fomawrite error
message. This changes the executable location; normal app startup and the
existing local-file workspace behavior remain active.

For staging verification without starting any application:

```powershell
$starter = [ScriptBlock]::Create([IO.File]::ReadAllText('C:\path\to\launch-local.ps1'))
& $starter -Package 'C:\path\to\Fomawrite' -StageOnly
```

`-StageOnly` returns JSON containing `localExecutable`, `localPackage`, source
package, architecture, commit, manifest/executable hashes and `reusedCache`.
The source script may be outside the package, which permits staging an older
package whose manifest predates the starter. The production command file uses
the adjacent package automatically.

`windows/test_launch_local.ps1` exercises synthetic packages with no UI launch:
new staging, exact file selection, unchanged cache reuse, concurrent lock
waiting, source/cache corruption rejection, unexpected cache files, unsafe
manifest paths, separate manifest versions and temporary cleanup. Run it in a
hidden Windows PowerShell process while desktop acceptance is being conducted.

The starter's staging tests do not establish that the app's menu, dialogs,
editing, rendering or restore behavior works. Final qualification requires the
[ordinary launch gate](NORMAL-LAUNCH-GATE.md) against the verified local copy,
plus the separate document-view diagnostic. Until those finish, the final
normal-launch outcome remains unverified.
