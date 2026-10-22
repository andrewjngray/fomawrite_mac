# Cycle 154 — Windows Focus and chrome menu parity candidate

22 October 2026. Bounded Windows-port continuation from `0e9ef4a71d0c43e21440d9efe0c1d1110998d6c5`. Candidate commit and native qualification are pending. Current build and package identities remain in [Windows STATUS](../../docs/windows/STATUS.md); this record does not advance the primary macOS build.

## Scope and behavior

Expose existing shared writing commands through the real Windows desktop menu. The primary Mac checkout and installed/distribution/Dev app bundles are outside this change.

| Finding | Candidate behavior |
| --- | --- |
| Windows had shared focus commands but no Focus menu. | Focus → Enable Focus Mode exposes Sentence, Paragraph and Typewriter (Source). Sentence and Paragraph remain exclusive; Typewriter remains independent. |
| Existing style toggles and Writing Review lacked Windows desktop routes. | Focus → Enable Style Check exposes Fillers and Custom; Writing Review opens the existing dialog. Highlight refresh preserves document text, caret, Undo and authorship. |
| Windows lacked the existing chrome controls. | View → Title Bar offers Fade In/Out and Always Show. View → Toolbar offers Fade In/Out, Always Show, Hide and Default. Stats Only contains all ten existing metric toggles and a heading checked from persisted toolbar mode. |
| Existing tests accepted absent Windows menus or used command-registry state instead of menu state. | Five existing cases now require actual platform menu nodes, checked state, enabled dispatch and persisted state. Mac native menu assertions remain active. No Qt test functions were removed or added. |

`WindowsMenuBar.qml` reuses its existing `WorkspaceMenu` and `Command` components, including nested-menu focus ownership and restoration before command dispatch. No new document, focus-mode, style-check or statistics engine is introduced.

## Regression and native proof requirements

The five tightened cases are:

- `focusMenuKeepsModesExclusiveAndTypewriterIndependent`
- `customStyleCheckPersistsAndRefreshesWithoutEditing`
- `fillerStyleCheckTogglesIndependentlyWithoutEditing`
- `statisticsCountRenderedUnicodeTasksAndAuthorship`
- `chromePresentationModesPersistAndKeepCommandsReachable`

Native Windows red/green has **not run** for this candidate. A local Mac green run does not prove a Windows regression.

The optional `menu_baseline_revision` Windows workflow input accepts only a full 40-character commit SHA and fetches that exact revision from `origin`. With the input empty, baseline fetching and proof execution are skipped. After a successful complete native Qt build/test run, `bin/windows/prove-menu-regressions.ps1` retains the newly built tests, replaces only the local `src/WindowsMenuBar.qml` with the baseline Git blob using a byte stream, and requires all five tests to fail at their specific missing-menu assertions and current C++ source lines. Setup, parse, crash, skip, missing-case and unrelated failures are rejected.

The proof restores the original menu bytes in `finally`, verifies the restored SHA-256, and requires the same five tests plus init/cleanup to pass with no failures or skips. Red/green text and JUnit logs and a JSON report with executable/source hashes and restore result are included in native evidence artifacts. Both ARM64 and x64 runs remain required against the exact candidate. Script execution itself remains unverified on Windows; local validation covered workflow YAML, actual Qt JUnit structure and the source-location pattern in retained genuine Windows logs.

## Local verification so far

- Required `./bin/build`: passed in this Windows checkout on the Mac host; completed log is retained locally as `evidence/build.log`.
- Focused `./bin/test` for the five cases: **7 passed, 0 failed, 0 skipped**, including init/cleanup, **3.668 seconds** on Qt 6.11.2 ARM64/macOS. Log: `evidence/focused-build-and-test.log`.
- Focused executable rerun to capture real JUnit for proof-parser validation: **7/0/0**, **3.469 seconds**. Logs: `evidence/focused-green.xml` and `evidence/focused-green.log`.
- QML parser accepted `WindowsMenuBar.qml`; workflow YAML parsed; `git diff --check` passed. These are syntax/structure checks, not Windows execution evidence.
- Full required local `./bin/test`: **370 passed, 0 failed, 1 skipped**, **556.797 seconds**. The skip is Windows-only appearance on the Mac host. Log: `evidence/full-local-test.log`. Generated reference captures were restored after this run; no baseline was changed to obtain a pass.

The focused logs retain delayed backend/null-object warnings during destroyed-window transitions and compiler unused warnings; these runs are not claimed as warning-free. They verify Mac native command behavior and shared state. Actual new Windows popup pointer activation, overflow and focus restoration remain native follow-through.

## Ordinary-input reliability support and baseline evidence

The accompanying maintained `r7` input-driver correction addresses observing Exit in a long, scrolling File popup. End did not scroll that popup, and UI Automation could omit the lower rows or report coordinates from an intermediate wheel frame. The driver now physically scrolls an owned foreground popup, measures its exact crop, requires three identical crop hashes/Exit coordinates spanning at least 300 ms plus a matching fresh crop, and rejects changed popup geometry or a final crop older than 750 ms before clicking. Failed attempts and measured evidence are retained. This is test-driver reliability work, not a claimed application menu fix.

The existing `0e9ef4a` ARM64 package was exercised independently of these new Focus/chrome routes with maintained harness revision `final-0e9-workflows-exit-ocr-stable-r7`. The reviewed local summary is retained at `evidence/baseline-runtime-summary.json`; its immutable source is `/private/tmp/fomawrite-0e9-arm-runtime-summary.json`.

| Baseline workflow | Recorded result and boundary |
| --- | --- |
| Normal startup/restore | PASS plus strict verifier. |
| Live daily writing | All five scenarios PASS plus strict verifier. |
| Publishing supplement | 9/9 workflows passed; all 95 declared hashes matched; eight format captures were separately reviewed. The wrapper's exit status 255 is retained, so this is a workflow/report result rather than a zero-exit wrapper claim. |
| Selected formatting/Save supplement | Failed strict verification: selected word was not proved/preserved, empty bold markers appeared, and old disk bytes plus dirty state remained. Attribution remains open; this is not an accepted pass. |
| Clipboard menu supplement | Failed Copy Markdown readback; restoration failed with `0x800401D4`, and original clipboard preservation is unknown. Strict verification rejected the run. |
| Nested View supplement | Failed reopening Sort Files By after the first Name selection; strict verification rejected the run. |

These baseline observations do not qualify the new Focus/chrome controls. Typed emoji input remains unqualified, the menu and selected-formatting supplements remain incomplete, and no package promotion or complete Windows/Mac parity is claimed. CI completion and exact-package native follow-through remain pending.
