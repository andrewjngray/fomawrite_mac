# Cycle 146 — keeping Harper current

Andrew's requirement (10 October 2026): pull new Harper releases down periodically, with a menu item that triggers it. Harper's rules and dictionary are compiled into the engine, so an update is a newer harper.js release, verified and loaded in place of the bundled one, with a way back. Plan: [docs/harper-plan.md](../../docs/harper-plan.md) ("Keeping Harper current", now marked built).

## What it does

| Surface | Behaviour |
| --- | --- |
| **Help → Check for Writing Checker Updates…** | Reads the npm registry's `latest` for harper.js. Banner (the existing `navigationNotice`): "Harper is up to date (2.10.0)", or "Harper 2.12.0 available — installing…" and then "Harper 2.12.0 installed (was 2.10.0)", or the reason it failed ("Could not check for Harper updates: …", "Harper update failed: …"). Dimmed while a check or install runs. |
| **Help → Use Built-in Writing Checker** | Rollback: removes `<AppData>/harper/current`; the version folders stay; the engine reloads and the bundled Harper serves again. Enabled only when an installed engine is serving. "Using the built-in Harper 2.10.0". |
| **Help → Check for Updates Automatically** | Ticked by default (`harper/autoCheck`). |
| **Weekly check** | Ten seconds after the window shows (`Main.qml`, `harperUpdateTimer`), only if automatic checks are on and `harper/lastCheck` is more than 7 days old: check, and if newer, install and show "Harper 2.12.0 installed (was 2.10.0)". Never installs the same version automatically twice (`harper/autoAttempted`: set by an automatic install, by an automatic install that failed verification, and by a rollback, so "Use Built-in" is not undone a week later). A network failure is silent; a download that fails verification is not. |
| **Review pane status line** | "Harper 2.10.0 (built-in)", "Harper 2.12.0 (installed)", plus " · 2.12.0 available" after a check that found one and " · updating…" while installing. |
| **Other windows** | Every window has its own `Backend`, updater and engine; an install or rollback in one makes all of them refresh and reload their engine (`HarperUpdater::s_instances`). |

## How

**Versioned serving** ([src/harperscheme.cpp](../../src/harperscheme.cpp)). ES module maps are keyed by URL, so after an update `import("fomawrite://harper/index.js")` would return the old module. The scheme now serves:

- `fomawrite://harper/VERSION` — unversioned; the version to use (the override's if one is installed and valid, else the bundled one).
- `fomawrite://harper/<version>/<file>` — from `<AppData>/harper/current/` when `<version>` is that folder's VERSION, else from the bundled resources when it is the bundled VERSION, else 404. Exactly two path segments; `<version>` must be `x.y.z`; no `..`, no backslash, no nested paths; a versioned `VERSION` is not served.
- `Cache-Control: no-store` on every reply (and the existing `Access-Control-Allow-Origin: *`).

The override is valid only if `current` holds `index.js`, `binary.js`, `BinaryModule.js`, `harper_wasm_bg.wasm` and a VERSION that is a plain `x.y.z`; otherwise the bundle serves. `HarperSchemeHandler::resolvePath(url, &mime)` is the lookup without a job, for tests.

**Page** ([src/editor/src/harper.ts](../../src/editor/src/harper.ts)). `load()` reads VERSION (no-store), imports `fomawrite://harper/<version>/index.js` and `binary.js`, and remembers the version it loaded. `HarperService.load(dialect)` with an engine already loaded re-reads VERSION: the same version just announces `harperReady` again; a different one drops the shared engine (`resetHarperEngine`) and loads the new one (queued lints wait for it); a `harperLoad` that arrives mid-load is checked when the load finishes. The dev server keeps `./harper/…` (unversioned). 2 new Node tests (321 total).

**Engine** ([src/harperengine.cpp](../../src/harperengine.cpp)). `reload()`: drops cached answers (a newer Harper may say different things), makes `ready` false (so the surfaces use the macOS checker until the page announces the engine, as for a new page), re-queues what was in flight and sends `harperLoad` for the current dialect.

**`HarperUpdater`** ([src/harperupdater.h](../../src/harperupdater.h)). Properties `bundledVersion`, `installedVersion`, `servedVersion`, `latestVersion`, `checking`, `installing`, `lastCheck`, `autoCheck`, `status`, `registryUrl`, `error`, plus `hasUpdate`, `hasOverride`, `paneSuffix`. Methods `checkNow()` (check only: `updateAvailable(v)` or `upToDate()`), `install()`, `checkAndInstall()` (what the Help item runs, with banner notices), `rollback()`, `runScheduledCheck()`, `refresh()`. Signals `installed(version, previous)`, `rolledBack()`, `failed(message)`, `notice(text)` (to the banner), `servedChanged()` (to the engine's `reload()`).

Install: GET the tarball with `QNetworkAccessManager` into a private working folder `<AppData>/harper/.work-XXXXXX/`, hashing as it arrives (capped at 120 MB); compare the SHA-512 with the registry's integrity; list the archive with `/usr/bin/tar -tzf` and check every entry before anything is extracted; extract with `tar -xzf` into the working folder; walk the result (no symlinks, no special files, nothing resolving outside the folder); take `package/dist/{index.js, binary.js, BinaryModule-*.js, harper_wasm_bg.wasm}` and `package/LICENSE*`; rename the glue to `BinaryModule.js` and rewrite its import in `index.js` and `binary.js` exactly as `bin/fetch-harper` does; write VERSION; move the finished folder to `<AppData>/harper/<version>/` with one rename; make `harper/current` a relative symlink created beside it and renamed over it (a reader sees the old target or the new one, never none); keep the new folder and the one that was current, remove the other version folders (16 MB each); delete the working folder (on every path, so a failure leaves nothing behind). A complete folder for the version (kept by a rollback) is re-activated without a download.

## Security: what is and is not verified

Verified:

- **Only https.** Registry and tarball URLs must be https with a host and no user info. The tarball must be on the registry's own host, scheme and port (`registry.npmjs.org` for the default registry). Redirects are not followed.
- **Integrity.** The tarball's SHA-512 must equal the `dist.integrity` the registry publishes for that version (`sha512-<base64 of 64 bytes>`, strictly parsed; a sha1, a short or padded string, or none refuses). That proves the bytes are the ones npm published for that version, over TLS to npm's registry. A mismatch (or a truncated download) deletes the download and installs nothing; the message says so.
- **Version.** Must be a plain `x.y.z` (it becomes a folder name and a URL segment); prereleases, tags and anything with a path in it are refused. Only a version newer than the one served is an update (never a downgrade).
- **Archive.** Every entry is checked before extraction (at most 64 of them; a release has 17): it must be under `package/`, either a file directly in it (`package.json`, `LICENSE`, `README`) or a file directly in `package/dist/` of the kinds a release holds (`.js`, `.d.ts`, `.json`, the two `.wasm` names, `LICENSE*`), nothing nested deeper; absolute paths, `..`, `.` and empty segments, control characters and backslashes are refused. After extraction nothing may be a link or special file or resolve outside the working folder. Files are copied out **by name**; nothing else from the archive is used. The WebAssembly binary must start with the `\0asm` header.
- **Never executed by the app.** The folder is only served to the editor page, which runs in Chromium's sandbox. The unpacking is `/usr/bin/tar` with fixed arguments (no shell), with a 60 s timeout.

Not verified:

- **Authorship.** npm's integrity proves "this is what npm published", not who wrote it. The registry's answer (version, URL, integrity) is trusted as TLS to npm delivers it; npm's package signatures are **not** checked (they are in the registry document, `dist.signatures`, and would be the next step), and neither is Automattic's GitHub release or the package's provenance.
- **Behaviour.** The new engine is not run before it is served, and no lint results are compared with the old engine's. A release that loads but lints worse is for Help → Use Built-in Writing Checker.
- **A compromised registry or npm account** that publishes a malicious version would be installed (the weekly check does so unattended, and is on by default). The loaded code runs in the Live page's renderer (Chromium's sandbox) and can reach the host only through the editor bridge, but the bridge carries the page's whole authority: it can rewrite the open document (`pushChanges`), read local image files the document refers to (`requestImage`) and write image files beside the document (`saveImage`). The page's content policy blocks network access and navigation. So the realistic worst case of a malicious release is tampering with or reading the writer's documents, not code execution on the Mac. The next hardening step is to pin npm's registry public key and verify `dist.signatures`, which also closes the rogue-CA case.
- **`/usr/bin/tar` itself** is trusted (macOS's bsdtar).
- Test-only allowance: `registryUrl` (not user-facing, not stored) may be http on loopback so tests can serve it in-process; with the default registry only https on `registry.npmjs.org` is accepted.

## Memory

An update loads a second copy of the engine in the page (the first stays in Chromium's module map, which is keyed by URL, until the page reloads at the next launch). Harper's share of the Live renderer was about 320 MB (Cycle 145), so an update costs up to that much again until the next launch.

## Verification

Tests in [tests/cycle146-harper-updates.inc](../../tests/cycle146-harper-updates.inc); nothing reaches the real network (a QTcpServer on 127.0.0.1 plays the registry and the tarball host; `FOMAWRITE_NO_HARPER_UPDATE_CHECK` is set in `initTestCase`, so no window starts the weekly check); the packages are built with `/usr/bin/tar` from stand-in files (a fake `BinaryModule-abc.js`, `index.js` importing it, `binary.js`, a 16-byte `.wasm` with a header), except the last test.

- `harperSchemeServesVersionedPathsFromTheOverrideThenTheBundle`: VERSION and `<bundled>/<file>` from the resources with their types; the unversioned old paths, other versions, other hosts, traversal and nested paths are 404; an override (a `current` link) serves its version and VERSION, the bundled version still resolves to the bundle, an unknown version is 404; a VERSION that is not a version, or a missing engine file, falls back to the bundle.
- `harperUpdaterReportsUpToDateAndUpdateAvailable`: defaults; up to date; an older registry version is not an update; a newer one is "available" and is not installed by `checkNow`; a downed registry and a non-registry answer fail with messages.
- `harperUpdaterInstallsAnUpdateServesItAndReloadsTheEngine`: install with no check refuses; install writes `<AppData>/harper/<v>/` with the glue renamed, both imports rewritten, the wasm byte-identical, LICENSE and VERSION; nothing else is left in the data folder; `current` points at it; `servedVersion()` and `resolvePath` serve it; the bundled version still resolves; `harperLoad` goes out again (QSignalSpy on `EditorBridge::harperLoad`) and the engine is not ready until the page announces the new version.
- `harperUpdaterRollbackRestoresTheBundledVersionAndKeepsTheFolders`: the link is gone, the folder stays, the bundled version serves, the rolled-back version 404s, the engine reloads, "Using the built-in Harper …"; a second rollback is harmless; installing again from the kept folder makes no download.
- `harperUpdaterRefusesAWrongIntegrityAndLeavesNothingBehind`: registry vouching for other bytes; a truncated download; the data folder is empty afterwards; a correct package then installs.
- `harperUpdaterRefusesPackagesThatReachOutsideDist`: the entry rules directly (15 refused, 7 allowed); real tarballs with a `package/evil/` folder, a `../escape.txt` (made with `tar -P`), a symlink in `dist`, no glue file, a non-wasm binary: each refused with a reason, nothing installed, nothing left, and the `../` file not written.
- `harperUpdaterAcceptsOnlyHttpsAddressesOnTheRegistrysHost`: integrity parsing; version comparison; http, ftp, file and user-info registry URLs refused before anything is sent; registry answers with an http tarball elsewhere, an https tarball on another host, the registry's host on another port, a file tarball, no or sha1 integrity, a version with a path, a prerelease, a tag: all refused, and `install()` has nothing to install.
- `harperUpdaterWeeklyCheckRespectsLastCheckAndAutoCheckAndInstallsOnce`: checked yesterday → nothing asked; auto off → nothing asked (and persisted); eight days → check, install, one banner line, `lastCheck` recorded; again at once → nothing; a week later, same version → up to date, silent; after a rollback the same version is offered but not installed, and the manual command still installs it; a newer release is picked up once; a network failure is silent in the automatic path and shown in the manual one; a download that fails verification is shown and not retried for that version.
- `helpMenuOffersWritingCheckerUpdatesWithBannerAndPaneStatus` (window): the three Help items and their text; enabled, dimmed and ticked state (Use Built-in dimmed until an install, the toggle ticked and unticked by the command); the banner for up to date, "available — installing…", the result, "Using the built-in Harper …", and an error; the pane's status line through "(built-in)", the macOS checker while the engine reloads, "(installed)", "· available (built-in)". The suite does not run the weekly check from a window.
- `harperUpdateLoadsInTheRealPageUnderItsOwnVersionedUrlAndRollsBack` (real page, real engine): a package made from the bundled engine under 99.0.1 with a marker appended to `index.js`; installed through the updater; the page announces `harperReady("99.0.1")`, the marker is set (the new module ran, imported from `fomawrite://harper/99.0.1/index.js`), a lint is answered; after the rollback the page announces the bundled version again and lints. Also checks `Cache-Control: no-store` on `fomawrite://harper/VERSION` from inside the page.
- Existing test updated: `writingCheckerCommandsMenuAndPaneFollowTheEngine` now expects "Harper 2.10.0-test (built-in)" in the status line.
- Node: `harperLoad after an update: …` and `a harperLoad that arrives while the engine is still loading is checked once it has loaded` (321 tests; typecheck clean).

### Green

`./bin/test` (full, offscreen): **273 passed, 0 failed, 0 skipped** (309 s). The ten new tests above pass alone (`./bin/test <names>`) and in the full run. `npm test`: 321 passed (319 before); `npm run typecheck` clean.

### Red then green

Each break made alone in the code under test, then reverted (`git checkout`):

| Break | Result |
| --- | --- |
| `HarperEngine::reload()` returns at once | `Actual (loads.count()): 0  Expected (1)` (cycle146-harper-updates.inc:319, the install test) and the Help test's "macOS checker while the engine reloads" check returned FALSE; 2 failed |
| `runScheduledCheck()` ignores `lastCheck` | `'!updater->checking()' returned FALSE` (the weekly test, "checked yesterday: nothing is asked"); 1 failed |
| `rollback()` does not remove `current` | `'!QFileInfo(root/current).isSymLink() && !exists' returned FALSE` (rollback test); 1 failed |
| Page: `harperLoad` with an engine loaded only re-announces (no VERSION re-read) | Node: "harperLoad after an update: the version is read again…" failed (320 pass, 1 fail); Qt real page: `Actual (readied.last()): "2.10.0"  Expected: "99.0.1"` after 60 s; 1 failed |

Not demonstrated red: the **security checks** (integrity compare, entry rules, https/host rules, version matching in the scheme). The automation that built this refused to let a build temporarily disable them, even for a break-and-revert, so these tests have only been seen green. They test the refusal directly (a wrong hash, a `package/evil/` entry, a `../` entry made with `tar -P`, a symlink, http and foreign hosts, a `99.0.1/../../x` version) and assert the data folder is empty afterwards; run the same break by hand if you want the red.


## Not done

- npm signature (`dist.signatures`) verification and provenance.
- A progress indicator for the 20 MB download (the banner says "installing…" and the pane says "updating…").
- Pruning the old module from the page's memory without reloading the page.
- The Rules page (the rest of the plan's Cycle 146 line) is not part of this change.

## Review and fixes (build 146)

A security-first reviewer broke each check in turn and confirmed the matching test went red (integrity compare, archive entry rules, https-only rule, the scheme's version match), built crafted archives with symlinks and hard links and confirmed `tar` and the post-extraction walk refuse them, and read the real registry document and the pinned tarball against the parser. Verdict: ship with fixes. Taken: the bundled engine wins over a stale download unless the download is strictly newer (an app update could otherwise ship a newer Harper that an earlier download kept hiding); the check repeats every six hours while the app stays open, the weekly rule deciding; archive entries limited to the kinds of file a release holds, nothing nested under `dist`, at most 64 entries; version strings anchored (`\A…\z`: PCRE's `$` accepted a trailing newline); a future `lastCheck` no longer silences the check; an empty data root guards rollback; one install at a time across windows. The security paragraph above now says plainly what a malicious release could do through the bridge. Not taken: npm signature verification (next step), extraction size caps beyond the entry rules, a slow-drip download guard, the second engine's memory in the page until relaunch.
