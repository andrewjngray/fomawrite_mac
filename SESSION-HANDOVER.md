# Fomawrite Windows — session handover

**Closed for Andrew's manual testing and independent LLM review, 23 October 2026 (Brisbane).**

The Windows build is ready for an internal manual pass. It is **not a fully qualified release or installer**. Automated desktop testing has stopped. The last VM check, at 06:49 Brisbane, found **zero Fomawrite processes**. Further unattended cycles are paused at Andrew's request. The Mac checkout and installed Mac apps remain unchanged.

## Open the app

In the Windows VM, open:

```text
C:\Mac\Home\repo\projects\fomawrite_windows
```

Double-click **[TEST-FOMAWRITE.cmd](TEST-FOMAWRITE.cmd)** in that root folder. This selects the reviewed ARM64 package, verifies it and stages/runs it on Windows-local NTFS. Keep its entire package intact. Do not run the EXE directly from the Mac share or copy just the EXE.

Follow **[MANUAL-TEST-CHECKLIST.md](MANUAL-TEST-CHECKLIST.md)**. Use disposable copies for the initial pass. This is a portable app, not a validated installer.

**Avoid the historical `dist/windows-testing/START-WINDOWS-ARM64.cmd`:** it still selects older d573f16, which has a subsequently discovered rapid-Save defect. The root launcher above selects b056 instead.

## Exact checkpoint and locations

| Item | Identity / location |
| --- | --- |
| Repository | https://github.com/andrewjngray/fomawrite_mac |
| Windows branch | `windows/port` |
| Windows checkout on Mac | `/Users/andrewgray/repo/projects/fomawrite_windows` |
| Primary Mac checkout / branch | `/Users/andrewgray/repo/projects/fomawrite_mac` / `master` |
| Tested app version | `0.3.0-dev49` — version text alone is not a unique build identity |
| Tested product commit | `b056ab5e6a73078b69c99916a4c5bce52772dfa6` |
| Latest full Windows CI commit | `78b9d9c2540d44fc3fa73ae4a6e0a4ccc284d08b` |
| Closing documentation checkpoint | The commit containing this file; use `git log -1 --oneline` |
| ARM64 package | `dist/windows-testing/diagnostic-candidates/b056ab5-arm64/Fomawrite/` |
| x64 package | `dist/windows-testing/diagnostic-candidates/b056ab5-x64/Fomawrite/` |
| Actual ARM64 EXE in VM | `C:\Users\andrewgray\AppData\Local\Fomawrite\Portable\arm64-b056ab5e6a73-da4ae383dbba8c26\Fomawrite.exe` |
| ARM64 manifest SHA-256 | `da4ae383dbba8c2634b39d669abc93a3ecdc41e9cc7016b6de0fbb4643b01033` |
| ARM64 EXE SHA-256 | `1e113b3fb7c15cf72307dd1d9f6ac19842e4225b38c89056a73a59c97032345a` |

All 191 product source/resource hashes at the latest CI checkpoint match b056. Subsequent test, driver, diagnostic and documentation changes do **not** constitute a new app implementation. Newly stamped CI executables have different binary identities; their green CI does not automatically qualify them for ordinary desktop use.

`dist/` and raw evidence are local, ignored files: **a Git clone does not include the packages or screenshots**. They are available in this existing checkout; GitHub Actions artifacts also have finite retention. The reviewed summaries, identities, handovers and proposed patches are committed. Windows work has not been merged into the Mac branch.

## Work completed

The Mac baseline supplies Source and Live editing of canonical UTF-8 Markdown, Web/PDF publishing, CSS themes, editor styles and focus/chrome behavior. The Windows port adds ARM64/x64 Qt/MSVC builds, app-local dependencies, Windows menus/shortcuts and appearance, bundled Harper checking/updating, isolated acceptance profiles and verified portable launchers.

Substantial fixes covered:

- An invisible Organizer scrollbar intercepting button-centre clicks.
- WebEngine instability when the executable runs from shared storage; local verified staging is the workaround, not a claim that shared execution itself is fixed.
- Live reconnect, stale page updates, delayed selection/formatting and immediate Save ordering, with exact-byte/Undo/Redo checks.
- Windows Paginated Preview using the existing PDF renderer rather than printer-backed preview.
- Missing portable menu routes and shortcuts, plus native/offscreen regression coverage.
- Pinned Qt mirror fallback retaining original checksums, test readiness and evidence validation.

The longer [developer handoff](WINDOWS-REVIEW-HANDOFF.md) contains the architectural review map and earlier repair history. [Windows status](docs/windows/STATUS.md) is the current status record; root `STATUS.md` describes the Mac baseline.

## What passed

| Evidence | Result and limit |
| --- | --- |
| Full Windows CI at 78b9d9c | ARM64 **390/0/0** Qt and x64 **390/0/0**; each also passed **3 appearance checks, 124 deployed scenarios and 210 evidence/driver guards**. [Run 39845554167](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39845554167). |
| Shared Mac/editor CI at 6c12569 | **414 Mac Qt passes, 0 failures, 13 capability skips**; **324 editor passes, 0 failures/skips**, plus typecheck/assets. [Run 39835545936](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39835545936). |
| Ordinary b056 ARM64 desktop | Native startup/pickers/edit/save/Source/Live/Exit/reopen **6/6**; full Live timing series **5/5**; selected Bold → immediate Save/Undo/Redo/reopen; publishing **9/9** with actual Web/PDF captures and exports. |
| Final Review/Center desktop supplement | **6/6 actions**, three visible Harper spelling cards, closed Review, exact next/previous selections, native DWM centering, exact document preservation, return to Live/Source and **two normal exit-zero launches including reopen**. Independent verifier passed; **255 evidence hashes** verified. |
| Root launcher | Actual isolated VM launch of the exact b056 EXE, visible app and normal close passed. |
| x64 ordinary launch | Passed in the ARM64 VM's emulation; **not physical x64 hardware qualification**. |

The final Review/Center pass used an **unmerged test-driver proposal** that acknowledges each physical filename character. It changes no app bytes. The two native file paths each acknowledged all 194 Unicode scalars, with no clipboard, SetValue or input replay. The maintained drivers have not been silently replaced.

The Mac/Windows case-identity audit found no missing applicable named case: 390 identities are shared. Mac's 37 additional rows are native-provider variants or duplicate component fixtures with Windows counterparts. Platform-specific bodies (notably NSFileVersion) remain exclusions. This is **not proof of every real desktop behavior**. See [parity ledger](docs/windows/PARITY-LEDGER.md).

## Remaining issues and untested scope

1. **Intermittent ARM64 atomic theme replacement failure.** Earlier run 39836302810 at 6c12569 failed one QSaveFile replacement with native error 5, Access denied. Later full CI and ten targeted attempts passed, but the cause remains unknown. Keep the failed record; do not hide it with retries. Failure-only owner diagnostics compiled but were not exercised in the passing attempts.
2. **Clipboard/search and nested-menu desktop completion.** Earlier attempts failed or stopped in accessibility/input observations. A proposed observer repair exists but has not had native compilation/execution. A fresh matching PublicControl receipt is required before clipboard tests. The original user's clipboard restoration from an earlier session remains **unknown**; later public-baseline restoration cannot prove it.
3. **Preview cleanup and Print.** Actual paginated pages/zoom/close worked, but earlier complete preview reports failed at subsequent Exit observation. Native Print cancellation stopped after a printer-connection prompt at a second Windows UI. No print or printer-setting change was made. Do not transfer the separate normal-launch/Review cleanup pass to these workflows.
4. **Wider acceptance:** final-package updater, unsaved recovery and multiple windows/profiles, broader storage/DPI/monitors/endurance, physical x64, signing, installer/upgrade/uninstall preservation and distribution review remain incomplete. The current portable manual checkpoint does not require pretending these are complete.

The failure email for **7edb501** was a separate diagnostic setup failure: the standalone build omitted Harper resources and stopped before testing. It was corrected in be99f89; the targeted diagnostic passed. It is distinct from the real intermittent theme-write failure above. No email notification settings were changed.

## Final-session evidence and proposals

- Full CI: `research/cycle-162/evidence/normal-78b9d9c/summary.json`.
- Reviewed records and hashes: [research/cycle-162/verified-builds.json](research/cycle-162/verified-builds.json) and [cycle record](research/cycle-162/README.md).
- Final scoped desktop run: `dist/windows-testing/results/maintained-b056-supplement-ada6a49d4add497d86407ff2b94dc93b-run-ack/evidence/`.
- Its raw report SHA-256: `b310ccbdc4d3c192e8cb394a365397d9b3de7e760e4aa6baadea4ae2dd6e453b`. The independent `desktop-supplement-verification.json` passes only `review-center-supplement`; `fullM2Acceptance` is false. `independent-visual-review.json` binds actual reviewed captures.
- Earlier final-session attempts remain in sibling `-run/` (shared-path alias rejected before editing) and `-run-unc/` (six actions passed, subsequent native filename input incomplete). Neither failed report was rewritten as a pass.
- [Unmerged driver patches](docs/windows/review-candidates/README.md) preserve filename acknowledgment and clipboard/search observer proposals for the reviewer. Their full frozen source copies and invocation notes remain under `dist/windows-testing/results/diagnosis/`.

## Suggested independent review

Read `AGENTS.md`, `MISSION.md`, this handover and `docs/windows/STATUS.md`. Start with an ordinary manual smoke pass using the root launcher and disposable documents. Verify Save/Undo/Redo, file switching, Source/Live and correct-document Web/PDF publishing before expanding test infrastructure.

Then inspect the async document/publishing boundaries and reproduce the retained theme-write failure if possible. Review proposed harness changes separately from application code. Assess whether the desktop automation can be simplified: substantial time went into asynchronous Windows dialogs and accessibility observers, not new product features. This has produced useful evidence but also delayed handoff. Preserve real failures and exact build identities; recommend the smallest changes needed for reliable daily use.

Do not infer complete parity from matching menu labels, green synthetic checks or passing a verifier alone. Do not replace the primary Mac app or discard user writing. Further unattended VM use should be coordinated with Andrew now that this session is closed.

For a fixed review baseline:

```sh
git status --short --branch
git log -1 --oneline
git remote -v
git diff --stat 6bc58f91878e26cbc916b0355b31a63325e3ac70...HEAD
```

The final documentation-only commit uses `[skip ci]`; no unnecessary rebuild is needed. The green executable/test run remains explicitly bound to 78b9d9c.
