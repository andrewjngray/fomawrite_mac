# Fomawrite Windows — independent review handoff

Snapshot: 23 October 2026 Brisbane / 22 October UTC. Owner: Andrew Gray.

**Disposition: internal manual-test candidate, not a fully qualified Windows release.** Andrew has narrowed this checkpoint to a usable manual-test handoff. Preserve the unresolved results below; do not turn sampled passes into an overall green qualification.

The session has now closed for Andrew's independent review. Start with **[SESSION-HANDOVER.md](SESSION-HANDOVER.md)** for the final scoped desktop result and frozen outstanding work.

## Start here

- Double-click **[TEST-FOMAWRITE.cmd](TEST-FOMAWRITE.cmd)** in this folder from Windows. It selects the reviewed ARM64 candidate and runs its existing verified Windows-local starter.
- The root launcher was actually tested on the ARM64 Parallels VM: exact local executable identity, isolated ordinary startup, visible window, normal Alt+F4 close and exit zero passed. Five evidence hashes and the screenshot were independently reviewed.
- Follow **[MANUAL-TEST-CHECKLIST.md](MANUAL-TEST-CHECKLIST.md)**.
- Authoritative current state: **[docs/windows/STATUS.md](docs/windows/STATUS.md)**.
- This handoff describes the product candidate `b056ab5e6a73078b69c99916a4c5bce52772dfa6`, version **0.3.0-dev49**. Later commits through `78b9d9c` concern tests, drivers, diagnostics, SDK acquisition and records, not a new product implementation; all 191 application source hashes still match b056. The final handoff commit is available with `git log -1`.

## Repository and separation

| Item | Location |
| --- | --- |
| Git repository | https://github.com/andrewjngray/fomawrite_mac |
| Windows branch | `windows/port` |
| Windows checkout | `/Users/andrewgray/repo/projects/fomawrite_windows` |
| Windows view of checkout | `C:\Mac\Home\repo\projects\fomawrite_windows` |
| Primary Mac checkout / branch | `/Users/andrewgray/repo/projects/fomawrite_mac` / `master` |
| Windows fork point | `6bc58f91878e26cbc916b0355b31a63325e3ac70` |
| Local build collection | `dist/windows-testing/` beneath the Windows checkout |
| ARM64 candidate | `dist/windows-testing/diagnostic-candidates/b056ab5-arm64/Fomawrite/` |
| x64 candidate | `dist/windows-testing/diagnostic-candidates/b056ab5-x64/Fomawrite/` |
| Local Windows execution cache | `%LOCALAPPDATA%\Fomawrite\Portable\` |
| Raw desktop runs | `dist/windows-testing/results/` |
| Latest cycle evidence | `research/cycle-162/evidence/` |

`dist/` and raw evidence are ignored and **not included in a Git clone**. Reviewed evidence summaries and `research/cycle-162/verified-builds.json` are committed. GitHub Actions artifacts have finite retention; exact downloaded logs/artifacts are retained locally. The old `START-WINDOWS-ARM64.cmd` points to historical d573f16, which has a later-discovered rapid-Save defect. Use the new root launcher, not that historical entry. The Mac checkout and installed apps were not replaced by this Windows work. Windows changes are not merged to master.

These are portable builds. A validated installation/upgrade/uninstallation EXE is still outstanding. Keep the whole package intact; copying only Fomawrite.exe is insufficient.

## What was implemented and repaired

The inherited Mac product has Source and Live editing of one canonical Markdown document, read-only Web/PDF publishing, editor styles, CSS publishing themes and auto-hiding chrome. Root `STATUS.md`, `ARCHITECTURE.md` and the older cycle records describe that baseline; this checkpoint primarily qualifies the Windows port.

Windows work includes platform-specific build scopes and appearance, pinned ARM64/x64 Qt/MSVC deployment, Windows menus and shortcuts, bundled Harper checking/updating, isolated acceptance profiles, and manifest-verifying portable launchers.

Important observed failures and responses:

- **Buttons appeared inert:** an invisible Organizer scrollbar intercepted ordinary button-centre clicks. The gutter was reserved; subsequent ordinary centre-click workflows passed.
- **Shared executable location:** original WebEngine failures occurred when running directly from Mac-shared storage. Identical binaries performed better from local NTFS. The starter now verifies and stages locally. The underlying shared-executable failure is not claimed fixed.
- **Live reconnect and rapid Save:** bounded renderer recovery, stale-message rejection and persistence/selection/formatting barriers were repaired. Exact saved bytes, Undo/Redo and delayed-save cases have regression and ordinary desktop evidence.
- **Paginated Preview:** Windows now uses the existing PDF renderer in its own read-only modal viewer rather than opening printer-backed preview. Actual pages and zoom worked, but an earlier whole-workflow report failed during later cleanup and remains a failure.
- **Test reliability and evidence:** real window/page readiness, owned physical-input observations, package identity, stale-page sensitivity and strict report verifiers were added. Changes to a test harness are recorded separately from product fixes.
- **SDK availability:** original September Qt archives disappeared from existing origins. Two exact-path mirrors were added while retaining the original checksums. Mirror availability is not guaranteed indefinitely.

See [repair history](docs/windows/M2-REPAIR-EVIDENCE.md), [Live persistence](docs/windows/M2-LIVE-PERSISTENCE.md), [menu coverage](docs/windows/PARITY-LEDGER.md) and [exact b056 desktop evidence](docs/windows/M2-DESKTOP-B056.md).

## Test results and limits

Latest normal Windows CI at `78b9d9c2540d44fc3fa73ae4a6e0a4ccc284d08b` is **successful on both architectures**: [run 39845554167](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39845554167). Shared Mac/editor results below remain bound to `6c1256909f4af5a9c422808ee1e3e270ddcaf786`.

| Check | Result |
| --- | --- |
| Mac Qt, `6c12569` | 414 passed, 0 failed, 13 capability skips |
| Editor, `6c12569` | 324 passed, 0 failed, 0 skipped; typecheck/assets also passed |
| Windows x64 Qt, `78b9d9c` | 390 passed, 0 failed, 0 skipped |
| Windows ARM64 Qt, `78b9d9c` | 390 passed, 0 failed, 0 skipped |
| Both Windows architectures, `78b9d9c` | 3 native appearance checks, 124 deployed scenarios and 210 deployment/driver guards passed; no deployed failures or QML warnings |

Exact artifact digests, executable/source identities and pinned SDK records were independently verified. All 191 application source hashes match b056; the later CI packages have separate identities and are not promoted or ordinary desktop qualified. [Retained normal CI summary](research/cycle-162/evidence/normal-78b9d9c/summary.json). [Shared CI 39835545936](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39835545936) records the Mac/editor results; Mac skips are twelve unavailable native grammar-checker rows and one Windows-only appearance row.

The earlier [Windows CI 39836302810](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39836302810) at `6c12569` remains a failed run: ARM64 **389 passed, 1 failed, 0 skipped**, x64 **390/0/0**. Its ARM64 failure is `publishingFolderThemesReloadAndAtomicResourcesSettle`: the second atomic replacement of imported CSS failed with `Access is denied`, QFileDevice error 10 and immediate Windows error 5. The intermittent failure's owner/cause remains unknown despite the later full-suite pass. Test-only native diagnostics record metadata and query Restart Manager after failure without retrying the write. The bounded diagnostic at `be99f89c253faf20db21fd2b88eccdc03f6ff2a3` compiled natively and passed **10/10 fresh-process attempts** (30 Qt rows including setup/cleanup): [run 39843298436](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39843298436). Neither passing run exercised the failure-only owner diagnostics or established a fix. The first diagnostic attempt failed before testing because its standalone build omitted the Harper download; that setup failure is retained and corrected.

Ordinary b056 ARM64 VM evidence separately covers normal startup/pickers/edit/save/Source/Live/Exit/restore **6/6**, five Live timing cases **5/5**, selected Bold with immediate Save/Undo/Redo/reopen, and publishing **9/9** with actual Web/PDF captures and exports. x64 ordinary checks ran **under ARM64 emulation**, not physical x64 hardware. These bounded passes do not establish all menu, recovery or distribution behavior.

## Open work, in priority order

1. Independently investigate the retained ARM64 atomic-theme failure. Do not weaken the assertion, silently retry until green, or assume a post-failure process snapshot identifies the culprit.
2. Manually test the current candidate with the checklist. Focus on Save/Undo, switching files and Source/Live, and matching current-document Web/PDF output.
3. Complete clipboard/search and nested-menu workflows, normal preview cleanup and native Print cancellation. The final b056 Review/Center supplement now passes independent verification, including three visible cards, centering, exact files and normal close/reopen. It used an unmerged physical filename acknowledgment driver proposal; 255 evidence hashes and both exit-zero launches are verified. See the [session handover](SESSION-HANDOVER.md) and [contract](docs/windows/DESKTOP-SUPPLEMENT.md).
4. Complete installer/upgrade/uninstall preservation, broader profiles/storage/recovery, DPI/monitors/endurance and physical x64 qualification. Final-package updater evidence also remains incomplete; older successful package evidence does not automatically transfer.

The original user's clipboard restoration from an earlier test remains **unknown**. Later public-baseline restoration does not establish restoration of that original content. Subsequent root-launcher testing does not use the clipboard.

## Review map and reproducible entry points

Read `AGENTS.md` and `MISSION.md` first. Review canonical-document and persistence boundaries in `src/editorbridge.cpp`, `src/LiveEditorPane.qml`, `src/backend.cpp` and editor sources; Windows commands in `src/WindowsMenuBar.qml` / `src/WorkspaceCommands.qml`; publishing in `src/publisher.cpp`, `src/publishingthemes.cpp`, `src/PublishingPreview.qml`, `src/WindowsPaginatedPreview.qml`; deployment and evidence in `bin/windows/`, `windows/` and `.github/workflows/`.

```sh
git switch windows/port
git status --short
git log -12 --oneline
git diff --stat 6bc58f91878e26cbc916b0355b31a63325e3ac70...HEAD
gh workflow run ci.yml --ref windows/port --repo andrewjngray/fomawrite_mac
gh workflow run windows.yml --ref windows/port --repo andrewjngray/fomawrite_mac
```

Normal Windows workflow dispatch runs both complete native suites. The separate `theme_atomic_diagnostics=true` input runs only bounded diagnostics and is not release qualification. Native developer build/test entry points are `bin/windows/build.ps1`, `test.ps1`, `package.ps1` with the pinned Qt kit and MSVC2022 v143; these tools were not installed into Andrew's guest for this session.

Suggested independent-review brief: validate document-byte preservation and asynchronous preview/Live identity first; reproduce the ARM64 failure; inspect whether acceptance witnesses really prove the user action; then recommend the smallest remaining fixes for a dependable manual-test build. Report product defects, harness defects and untested scope separately. Keep the Mac baseline and user's writing intact.
