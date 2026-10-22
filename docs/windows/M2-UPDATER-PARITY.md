# Windows M2 — checker update parity candidate

22 October 2026. Windows branch only. Implementation and focused host tests are complete; native Windows CI and final packaged VM qualification are pending. The testing shelf remains the previously qualified d573f16 package.

## Behavior

Windows now exposes the existing Help commands to check/install a writing-checker update, select the bundled checker, and enable automatic checks. It uses the trusted system tar executable, a verified npm SHA-512 download, a bounded USTAR conversion and an application validator before writing engine files. It does not require administrator rights, developer mode or directory symlinks.

The complete archive is checked before materialization: entry count and expanded size, checksums, padding, paths, Windows aliases/case collisions, and links/special files. System tar only emits a stream; it does not extract files. Empty names are rejected because an actual Windows ARM64 conversion of an overlong PAX name returned exit zero with an empty converted name.

A process-wide file lock serializes installation/activation. The verified version directory is selected through an atomic `current-version` text marker; `bundled` selects rollback. Readers validate the selected directory and required files. Busy locks and local I/O failures remain retryable. Existing Mac activation retains its atomic `current` symlink.

## Verification so far

- Focused host updater batch: 18 passed, zero failed/skipped. It includes the ten existing updater cases, four safety cases, platform/menu policy and setup/cleanup. A subsequent file-handle cleanup correction passed six affected install/rejection/rollback/real-WASM checks.
- Real shipped JavaScript/WASM was compressed, downloaded from the isolated loopback fixture, verified, installed and used by the real QRC editor page. The tests require actual spelling/repetition findings and UTF-16 offsets before and after rollback.
- Archive cases cover invalid names, links, aliases, case collisions, bad headers/padding and the expanded-output limit. A PATH-shadow tool is ignored. Lock contention remains retryable.
- Red/green: the old inventory validator accepted `package/dist/AUX.js`; the new regression fails there on the prior implementation and passes with the Windows-safe inventory checks.
- A Windows-only sharing-denied rollback case verifies no success notification and no activation change; it awaits native CI.

Host logs: `/tmp/fomawrite-updater-final-focused.log, /tmp/fomawrite-updater-cleanup-final.log` and `/tmp/fomawrite-updater-alias-red.log`. Native ARM64 tar conversion evidence is retained locally under `dist/windows-testing/results/diagnosis/tar-compat-ad927b55/`.

## Other open qualification findings

The previous ARM64 CI run failed a single imported-theme fixture replacement without reporting the I/O phase/error. The fixture now records the first failing open/write/commit operation, Qt error and native Windows error. It still makes exactly one attempt; there is no retry or skipped assertion. Four focused host cases passed (six including setup/cleanup). This instrumentation is not a claimed fix for that unexplained Windows failure.

Ordinary VM testing separately found a rapid Live Undo→Save ordering defect and a Modern Sans fallback defect. They remain release blockers until their own fixes, regressions and packaged repetitions pass. Passing this updater checkpoint alone does not qualify the app or promote a package.
