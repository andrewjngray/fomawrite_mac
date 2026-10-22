# Cycle 160 — Remaining Windows File, Authorship and View menu routes

22 October 2026. Bounded continuation from Cycle158 checkpoint `2f0f272770f031019a44fa2a7b0ebc99fecd8026`. This record covers eleven existing portable menu routes. Current package/build identity remains in [Windows STATUS](../../docs/windows/STATUS.md); checkpoint158 diagnostic packages do not contain this later slice.

## Change

File adds New in Library in Window, New in Library, New Folder, Show in Library, Autosave Saved Files Every Minute and Print Markdown Source. Edit adds Authorship Annotations and Export Authorship Metadata; Authors adds the existing Set Up Authorship dialog. View adds Synchronized Scrolling and Show Completions. All route through the existing handlers/settings and Windows focus dispatcher. The two toggles restore checked bindings after activation. Native Mac menu declarations, Backend semantics, Save/persistence and conservative dirty-until-Save behavior remain unchanged. Format159 is maintained separately and was preserved byte-for-byte when these hunks were applied.

## Regression

The existing Completions and Authors Setup cases retain native production rows and now require actual Windows menu nodes in production/explicit-component rows. The direct Windows Authors dialog fallback and Completions absence assertion are removed. Existing Unicode completion/Undo/protected-Markdown/stale-mode checks and profile validation/Save/Cancel/reopen/no-label/recovery preservation remain.

The new `windowsRemainingPortableMenuRoutes` has nine named rows, requiring real Windows nodes after first verifying correct production platform loading. It checks root/untitled guards; New/Folder dialog Cancel and accepted creation; the real unsaved Cancel/Discard path for same-window creation; separate-window signals with retained current draft; out-of-root Show in Library/filter clear; both preference toggles and external check synchronization; actual annotation snapshot and export picker cancellation. Source text, dirty/history/selection, original file bytes and relevant metadata remain protected.

Print Markdown Source's row is deliberately node/label/enabled-state coverage only. It never calls print/spool code. The separate existing `window-routing-smoke.inc` source-print path opens and cancels its print dialog, but does not qualify ordinary native Windows source-print cancellation for this candidate. No label-only assertion is presented as executed printing.

## Verification and limits

- Tests-only first compile caught 18 fixture compile errors: Backend.library returns QObject*, while the new fixture initially called FileLibrary methods through it. A checked `qobject_cast<FileLibrary *>` corrected this before any test execution. Log retained under Cycle159 evidence as `combined-first-compile-failed.log`; this is fixture correction, not product red.
- Genuine combined old-route red: **2 passed, 40 failed, 0 skipped**, 5.940 seconds. Cycle160 contributes exactly eleven intended missing Windows-node/registration failures: nine new rows plus Completions and Authors Setup Windows-component rows; the other 29 belong to Format159. Init/cleanup pass, with no unrelated failures. Log: Cycle159 `combined-missing-menu-red.log`.
- Required `./bin/build`: passed, Qt 6.11.2 ARM64/macOS. Complete output: `evidence/build.log`.
- Combined Format159/remaining-route160 focused green: **53 passed, 0 failed, 0 skipped**, **47.861 seconds**, exit 0. All thirteen Cycle160 host rows pass: nine new rows plus native-production/Windows-component rows for Completions and Authors Setup. The other forty results cover Format159 and selected existing controls, including init/cleanup. Complete output: `evidence/combined-menu-green.log`.
- The first combined post-route run was **51 passed, 2 failed, 0 skipped**. All Cycle160 rows passed; the two bottom Format rows failed pointer visibility before the Format fixture's real wheel correction. That separate fixture failure was retained by Cycle159; it is not old-route red evidence for this slice.
- `git diff --check`: clean for the new menu, tests and record. Required full integration and native Windows CI remain pending.
- Native ARM64/x64 execution, actual OS menu/keyboard input and source-print native cancellation remain pending. Host tests invoke actual menu signals; they do not claim native popup placement, scrolling or Win32 delivery.
- No clipboard or print/spool operation, VM activity, Backend dirty-state change, commit, push or package promotion was performed by this slice. Required complete integration/build/CI qualification remains the parent's responsibility.
