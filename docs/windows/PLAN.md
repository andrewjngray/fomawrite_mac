# Fomawrite project plan: Mac primary, Windows secondary

Approved direction: 22 October 2026. Owner: Andrew Gray.

## Outcome and separation

Keep the Mac editor dependable while adding a Windows distribution of the same local-file Markdown product. Source and Live edit the same canonical document; the publishing pane remains read-only Web/PDF output. Plain Markdown, exact save behaviour, undo, recovery and correct preview identity are release requirements on both platforms.

| Workstream | Location and responsibility |
| --- | --- |
| Primary Mac | `master`, `/Users/andrewgray/repo/projects/fomawrite_mac`; current build and evidence remain in the root `STATUS.md`. |
| Secondary Windows | `windows/port`, `/Users/andrewgray/repo/projects/fomawrite_windows`; progress in [Windows status](STATUS.md). |
| Repository | <https://github.com/andrewjngray/fomawrite_mac> — one shared history, separate branch, checkout, chat and build artifacts. |
| Fork point | `6bc58f91878e26cbc916b0355b31a63325e3ac70`, after the completed Mac stability cycle. |

This is an isolated workstream, not a second diverging product. Do not modify the primary Mac checkout, replace its installed bundles, or merge Windows work to master during the port. Pull subsequent Mac fixes into the Windows branch at reviewed checkpoints, without rewriting shared history. Shared fixes can return through focused PRs after Mac and Windows checks pass; merging and public release remain separate decisions.

## Programme sequence

1. **Maintain Mac stability.** Continue daily-writing checks and triage demonstrated defects. Preserve the current Qt and editor CI gates and native bundle acceptance. Do not add speculative features while qualifying Windows.
2. **Prove Windows feasibility.** Resolve compiler/platform blockers; produce an executable whose Source, Live, Web and PDF surfaces work in the actual target environment.
3. **Make Windows usable.** Complete platform menus, shortcuts, file handling, appearance, spelling, recovery and packaging.
4. **Qualify distribution.** Test deployed artifacts on clean Windows and the Parallels ARM guest; produce a versioned portable ZIP and installer EXE with a manifest and checksums.
5. **Review convergence.** Review shared changes for Mac regressions, agree whether to merge support, then consider signing/public distribution and optional performance work. No unrelated Roam/Teams work belongs here.

## Target and toolchain decision

Andrew will test in Parallels on Apple Silicon. Read-only inspection confirmed an existing, running **Windows 11 Enterprise ARM64** guest, version `10.0.26200`; Git is on PATH. Compiler, Qt and Node were not found on PATH; that does not prove they are absent elsewhere.

Target decision: **native ARM64 first for the Parallels guest, followed by a separate x64 build for conventional Windows PCs**. Official Qt 6.11.3 metadata confirms both architectures have the required WebEngine and PDF packages; see [target evidence](TARGET.md). Windows 11 ARM can also run x64 applications through emulation, which is a fallback/test case, not a native ARM64 claim. Package availability is established; build, deployment and actual rendering remain to be proved. ARM64 and ARM64EC are different targets.

Keep Qt 6.11.3 aligned with existing CI unless evidence requires a documented change. Use MSVC 2022 and a compatible Windows SDK; Qt WebEngine rules out MinGW. Keep qmake and the existing C++17 application. Use prebuilt Qt Core/Gui/Widgets/PrintSupport, QML/Quick/Controls/Dialogs, Concurrent/Network, WebChannel, WebEngine and PDF modules. Do not introduce a CMake migration into this port.

Sources: [Qt Windows support](https://doc.qt.io/qt-6.11/windows.html), [WebEngine platform requirements](https://doc.qt.io/qt-6.11/qtwebengine-platform-notes.html), [Microsoft ARM support and emulation](https://learn.microsoft.com/en-us/windows/arm/add-arm-support), [Parallels limitations](https://kb.parallels.com/en/129497).

## Milestones and acceptance gates

| Milestone | Implementation | Exit evidence |
| --- | --- | --- |
| **M0 — target proof** | Confirm Qt kits/modules for x64 and ARM64, MSVC/SDK requirements, runner availability and guest access. Inventory Windows blockers and select architecture. | Reproducible dependency list, exact versions and source URLs; compile/render spike plan; no unsupported-target claim. |
| **M1 — executable and CI** | Split macOS/Linux/Windows build scopes; remove Windows D-Bus assumptions; implement Windows system appearance; handle POSIX-only Harper updater code. Add reproducible PowerShell build/test scripts and Windows CI. | Native Windows compile; a minimal deployed directory or portable ZIP smoke launches (installer follows in M3); Source, Live, Web and PDF each paint the correct fixture; shared regression and editor jobs pass with justified platform-specific skips. |
| **M2 — desktop behaviour** | Reuse the command registry for Windows menus and Ctrl shortcuts; native open/save/export dialogs, Explorer reveal, multiwindow routing, fonts and DPI. Hide unsupported Mac-only commands. Make Harper state truthful without a macOS checker. | Actual keyboard/mouse workflows, exact save/reopen, undo, recovery, theme/font import, correct preview after rapid switching. Windows capability matrix completed. |
| **M3 — distributable artifacts** | Deploy DLLs, QML plugins, WebEngine helper/resources/locales, PDF support, fonts, licences and pinned Harper. Produce portable ZIP plus per-user installer EXE; version resources, icon, install/uninstall and optional file associations. | Both artifacts start on a machine without Qt/developer PATH; no missing assets; manifest binds architecture, commit and resource hashes; upgrade/uninstall preserve user documents and data. |
| **M4 — Parallels and regression qualification** | Run [test matrix](TEST-MATRIX.md) on the actual installed executable, guest NTFS and shared folders. Inspect rendering and performance under the VM graphics stack. | Evidence per artifact/architecture; cold/restored startup and current-document Web/PDF pixels pass; no unexplained blank pane, stale document, save loss or crash. Physical-input follow-through is recorded separately. |
| **M5 — review and delivery** | Independent code/test review, resolve material findings, publish internal artifacts and developer instructions, sync branch and status. | Windows CI and Mac regression gates green; reproducible download/install steps and explicit remaining limits. Public signing/release is a later approval, not implied by an internal test build. |

A rough planning allowance is several development days to a first usable candidate and one to two working weeks for a dependable internal distribution. This is an estimate, not an unattended-agent completion promise. Re-estimate after M0/M1; WebEngine deployment, ARM packages and preview correctness are the largest uncertainties.

## Concrete code work

- `fomawrite.pro`, `tests/tests.pro`, `src/systemtheme.*`: make platform scopes explicit. The existing non-Mac path assumes Linux D-Bus; preserve Linux code while adding Windows support.
- `src/harperupdater.cpp`, `src/harperscheme.*`: replace or isolate unguarded `unistd.h`, `/usr/bin/tar`, POSIX symlink/rename activation. Preserve archive traversal checks, integrity verification, atomic activation and rollback. M1 may explicitly disable updater commands while using bundled Harper; require W12 before enabling updates, and record the omission rather than claiming beta parity. Avoid requiring administrator symlink privileges.
- `src/spellcheck.cpp`, `ReviewPane.qml`, `WorkspaceCommands.qml`: expose only available spelling engines and correct labels. Windows must not report the macOS engine as a fallback.
- `src/Main.qml`, `NativeMenuBar.qml`, `WorkspaceCommands.qml`: Windows menu access and Ctrl-key commands must reach the existing document operations. Mac-only sharing, versions, speech and window functions need explicit capabilities or honest omission.
- `src/backend.cpp`, `workspace.cpp`, `filelibrary.cpp`: qualify Windows drive/UNC paths, Unicode, case semantics, locks, external changes, application data locations and single-instance routing. Do not normalize document bytes as a side effect of portability.
- `src/documentviewcheck.cpp` and acceptance includes: make supported checks run against the deployed Windows executable. Keep paint/current-document checks, not only backend success signals.
- Add `windows/` packaging metadata and `bin/windows/` PowerShell entry points. Add a dedicated Windows workflow, retaining Mac and editor validation for shared code. Keep output under ignored build/dist paths.

## Testing and evidence policy

Use the [test matrix](TEST-MATRIX.md) as an executable acceptance backlog. Every defect fix needs a regression that fails before and passes after the fix. A compile-only change needs a before/after compile result. Existing tests passing are useful but do not by themselves prove a new fix.

Check the packaged app, not only the development executable. Record commit, Qt/compiler versions, architecture, guest OS, artifact SHA-256, test counts/skips, logs and representative screenshots using disposable writing. Preview readiness must include matching document identity and rendered content/pixels. Asynchronous switching tests must reject stale results and catch blank or flickering output.

Keep Mac CI and editor tests as gates on Windows branch PRs. Windows-only jobs must run for Windows branch pushes and manual dispatch; branch configuration must not accidentally leave them dormant. A hosted runner cannot certify Parallels graphics or physical interaction. No broad skip lists, disabled sandbox, mocked rendering or weakened assertions to obtain a green badge.

Each implementation milestone gets a concise evidence record and changelog entry. Root `STATUS.md` continues to identify the Mac baseline; `docs/windows/STATUS.md` is authoritative for Windows readiness. Do not overwrite historical handoffs or claim unsatisfied gates are complete.

## Distribution and safety boundaries

Use Qt's [Windows deployment](https://doc.qt.io/qt-6.11/windows-deployment.html) and [WebEngine deployment](https://doc.qt.io/qt-6.11/qtwebengine-deploying.html) instructions, including the correct runtime redistributable and subprocess assets. Inspect runtime dependencies and exercise a clean installation. Package the existing MIT/OFL/third-party notices and applicable Qt notices/licensing obligations.

Use disposable profiles/documents for automated tests. The harness must isolate settings, recovery directories, instance identity and IPC endpoints as well as document files, so it cannot restore or forward into another normal instance. Keep the normal user workspace and Mac recovery/settings intact. Inspect VM configuration read-only first; no snapshot deletion, VM reset, host protection changes or unrelated software upgrades. Prefer CI builds and test artifacts in the guest before installing a full development toolchain there. Never write tests over Andrew's real writing. Public release, paid signing services and changes to the default Mac product remain outside the initial port.

## Orchestration and cost control

The separate Windows chat is to use **GPT-6 Astra with Ultra reasoning** as requested. Astra owns design, integration, difficult diagnosis and final evidence review. Delegate scoped implementation and test work to **GPT-6.1 Sol**, and simple inventory/docs/tool checks to **GPT-6 Luna**. Escalate a worker's task when findings or failed tests justify it; saving credits must not lower acceptance standards.

Give workers only the files, contracts and context needed. Assign disjoint file ownership, reuse agents, batch reads and avoid repetitive full-suite runs without a changed risk. Run cheap focused checks first, full suites at integration gates, and actual-app checks before delivery. Require an independent review of document I/O, preview lifecycle and packaging changes. Keep one integration owner; no simultaneous agents editing the same file or publishing conflicting artifacts.

## First execution cycle

1. Complete M0 and record the architecture decision.
2. In parallel, delegate platform build/updater work and Windows CI/test-harness work with disjoint ownership.
3. Integrate a first executable and use the Parallels guest to prove Source, Live and publishing output with disposable fixtures.
4. Fix failures with regression evidence, update Windows status and push the Windows branch. Do not stop at a successful compile or describe an untested installer as a release.
