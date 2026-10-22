# Windows M2 — checker update parity candidate

22 October 2026. Windows branch only. Implementation and focused host tests are complete; native Windows CI checkpoint results are below. Corrected CI and final packaged VM qualification remain pending. The testing shelf remains the previously qualified d573f16 package.

## Behavior

Windows now exposes the existing Help commands to check/install a writing-checker update, select the bundled checker, and enable automatic checks. It uses the trusted system tar executable, a verified npm SHA-512 download, a bounded USTAR conversion and an application validator before writing engine files. It does not require administrator rights, developer mode or directory symlinks.

The complete archive is checked before materialization: entry count and expanded size, checksums, padding, paths, Windows aliases/case collisions, and links/special files. System tar only emits a stream; it does not extract files. Empty names are rejected because an actual Windows ARM64 conversion of an overlong PAX name returned exit zero with an empty converted name.

A process-wide file lock serializes installation/activation. The verified version directory is selected through an atomic `current-version` text marker; `bundled` selects rollback. Readers validate the selected directory and required files. Busy locks and local I/O failures remain retryable. Existing Mac activation retains its atomic `current` symlink.

## Verification so far

- Focused host updater batch: 18 passed, zero failed/skipped. It includes the ten existing updater cases, four safety cases, platform/menu policy and setup/cleanup. A subsequent file-handle cleanup correction passed six affected install/rejection/rollback/real-WASM checks.
- Real shipped JavaScript/WASM was compressed, downloaded from the isolated loopback fixture, verified, installed and used by the real QRC editor page. The tests require actual spelling/repetition findings and UTF-16 offsets before and after rollback.
- Archive cases cover invalid names, links, aliases, case collisions, bad headers/padding and the expanded-output limit. A PATH-shadow tool is ignored. Lock contention remains retryable.
- Red/green: the old inventory validator accepted `package/dist/AUX.js`; the new regression fails there on the prior implementation and passes with the Windows-safe inventory checks.
- A Windows-only sharing-denied rollback case verifies no success notification and no activation change; it passed native x64 CI.

Host logs: `/tmp/fomawrite-updater-final-focused.log, /tmp/fomawrite-updater-cleanup-final.log` and `/tmp/fomawrite-updater-alias-red.log`. Native ARM64 tar conversion evidence is retained locally under `dist/windows-testing/results/diagnosis/tar-compat-ad927b55/`.

## Native CI checkpoint

[Run 39755453624, native x64 job](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39755453624/job/119746190121), commit `2d84cc40`, compiled with Qt 6.11.3/MSVC2022 on Windows Server 2022. All ten updater fixtures and four new archive/marker/lock/output-bound regressions passed, including trusted native tar, sharing-denied rollback, and real QRC/WASM install and rollback with expected findings. The full offscreen suite reported 315 passed, one failed and one Mac-specific skip. Its sole failure was the obsolete `windowsMenuRoutesPortableCommands` assertion requiring the newly supported updater command to be absent. The fixture now requires all three Help controls and their enabled/check states; the failing CI assertion is the red evidence, and verification of that correction is separate from these checkpoint results.

The separate native Windows appearance run passed three cases (including setup/cleanup), verifying Qt Dark→Light→Dark transitions. The offscreen unknown-scheme palette fallback also passed. The deployed executable checker passed all 124 scenarios. These results do not qualify ordinary VM input or promote a package; the corrected full suite remains pending.

The [native ARM64 job](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39755453624/job/119746190412) also passed all updater fixtures, the native appearance run (three passed), and all 124 deployed scenarios. Its full suite reported 315 passed, one failed and one Mac-specific skip, with the same obsolete menu assertion as its only failure. The [Mac Qt job](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39755453774/job/119746191085) reported 331 passed, one failed and 13 skipped; its only failure was also that assertion. The separate editor tests/committed-bundle job passed.

Imported-theme refresh and atomic-resource replacement fixtures passed on both Windows architectures. The new open/write/commit/native-error diagnostics did not fire, so the earlier intermittent fixture failure did not recur; this run does not explain its cause.

## Other open qualification findings

The previous ARM64 CI run failed a single imported-theme fixture replacement without reporting the I/O phase/error. The fixture now records the first failing open/write/commit operation, Qt error and native Windows error. It still makes exactly one attempt; there is no retry or skipped assertion. Four focused host cases passed (six including setup/cleanup). This instrumentation is not a claimed fix for that unexplained Windows failure.

Ordinary VM testing separately found a rapid Live Undo→Save ordering defect and a Modern Sans fallback defect. They remain release blockers until their own fixes, regressions and packaged repetitions pass. Passing this updater checkpoint alone does not qualify the app or promote a package.
