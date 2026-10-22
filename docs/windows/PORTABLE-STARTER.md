# Windows-local portable starter

The current repair candidate packages **START-FOMAWRITE.cmd** beside
`Fomawrite.exe`. Final ARM64 package identity and actual command-file launch
passed. The promoted shelf entry is
`dist/windows-testing/START-WINDOWS-ARM64.cmd`, pointing to the verified ARM64 package.
See [current status](STATUS.md) and [testing builds](TESTING-BUILDS.md).

Once the package is verified and promoted, the starter is the user entry when
the extracted package is on a Mac shared folder. It verifies the package and
runs a local copy from:

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
manifest paths, separate manifest versions and temporary cleanup. All **9
checks passed** without starting an application. **2 real UNC shared-folder
staging/reuse checks** also passed; provider-qualified UNC paths resolve through
the filesystem provider. The concurrent fixture explicitly passes its cache
root into the child process, records job state/reason/errors on failure and
allows up to 60 seconds for a running job to complete after lock release.

Run staging fixtures in a hidden Windows PowerShell process while desktop
acceptance is being conducted. The fixture results are separate from the actual
packaged ARM64 command-file launch described below.

The final ARM64 `START-FOMAWRITE.cmd` was invoked from the shared package and
**passed** actual ordinary startup with an isolated native profile, verified local
executable identity, cache reuse and File → Exit with exit status zero.
`dist/windows-testing/results/starter-d573-arm64-d85250e2e3d0455e82711a25da1e705e/starter-check.json`
records `passed: true`, `normalStartup: true` and `stageOnly: false`, with the
final commit, executable hash and manifest hash. The unmodified centre-click
[ordinary-launch gate](NORMAL-LAUNCH-GATE.md) also passed with strict offline
verification; a shared-document writing-path-only supplement passed separately.
Exact package/report identities and the retained initial environment-blocked
attempt are in [M2 repair evidence](M2-REPAIR-EVIDENCE.md).

The separate ARM64 guest document-view diagnostic passed all 124 scenarios
with no failures or QML warnings; the verified packages are promoted to the shelf. Final x64 VM normal-launch acceptance has not run.
Shared-source staging into the local cache and shared-document writing do not
qualify direct shared-folder WebEngine execution. Wider profiles, displays,
installer and distribution acceptance remain open.
