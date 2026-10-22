# Repair checkpoint: ordinary Windows desktop evidence

22 October 2026. This is a bounded diagnostic checkpoint, not overall release qualification. The current integration identity remains in [Windows status](STATUS.md).

## Exact package and environment

- Git commit: `2f0f272770f031019a44fa2a7b0ebc99fecd8026`, branch `windows/port`.
- Native ARM64 executable SHA-256: `df8114dfe40d813750e45b9d02bd7e9ca996a7ce58eee98ba4f8b21be6c424a5`.
- Manifest SHA-256: `2219c9b608834d340361089f8789744dcb998c3499db15a4558183e08bd22d61`.
- Parallels Windows 11 ARM64, Windows-local NTFS staging, isolated disposable profile and public writing fixtures. Ordinary argument-free app launch, physical `SendInput`, read-only UI Automation and captured pixels; no application test API was used for these desktop actions.
- Local reports below are under `dist/windows-testing/results/`, intentionally excluded from Git because raw desktop captures can contain surrounding personal desktop information. Reviewed summaries are committed here.

## Completed ordinary workflows

| Workflow | Result and evidence |
| --- | --- |
| Startup, native folder/file pickers, exact Source edit/save, Live/Source, Exit/relaunch | **6/6 passed**, strict verifier passed. `final-2f0-arm64-r10-normal-63371551d5e840a8ba471b4322adc3c2/normal-launch-report.json`, SHA-256 `a7ac27981893db5ee175be9491def4971e3072173ab78c3add2bfee0cd383788`. Root independently verified all **80** recorded evidence hashes. |
| Live selected Bold → immediate Ctrl+S, Undo/Save, Redo/Save, Source, native reopen and unchanged original | Workflow and strict verifier passed. `final-2f0-arm64-r10-selection-save-42b11ec046774293a36c620e413ca366/evidence/selection-save-report.json`, SHA-256 `22f61fb3a2bd438baf7570df478dad92e803643cca1c5fe42ccd3bd9e3611e72`. Root verified **108** evidence hashes, five exact UTF-8/clean-state checkpoints, and visually confirmed exactly `input.` stayed selected before and while the Format menu was open. Separate `root-selection-review.json` records this review without rewriting the raw report. |

The Save sequence contains no inserted renderer delay. The earlier failed OCR expected plain text while Live correctly revealed Markdown around the caret. Revision 4 permits those markers only in the two formatted stages; baseline, Undo and original reopen expectations remain plain. All physical actions and exact disk assertions are unchanged. The earlier failed run is retained.

The ordinary Exit harness revision 10 reacquires the same owned popup after scrolling; it no longer requires an unrelated Page Setup row to remain exposed in the accessibility tree. It still requires repeated stable actual Exit pixels/OCR, current ownership and a physical click. It does not use fixed screen coordinates or invoke application actions directly.

## Additional ordinary publishing and Live evidence

The r11 publishing workflow passed **9/9 stages**, including A→B→A document identity in Web/PDF, Modern→Classic style/reload, native HTML/PDF exports and normal Exit. Root independently verified all **95 evidence hashes** and visually reviewed all **eight** format captures: matching document text, white PDF paper, warm surround, and the actual sans/serif change. Report `final-2f0-arm64-r11-publishing-156a4ceafca5418fab95467cf8cb2098/publishing-supplement-report.json`, SHA-256 `c83890c774a6aa1bb31df1e6c7ce76dda650d5d30019452dba40b84d0bb5832e`. Separate `root-publishing-review.json` records acceptance without rewriting raw qualification flags. Parallels returned wrapper status 255 despite the completed successful report and child exit 0; the transport log remains retained.

The r11 five-case Live series remains **failed**: settled, immediate and 100 ms cases passed; 50 ms completed edit/save but its Exit accessibility observation failed; 150 ms was interrupted before Live input by a concurrent Parallels command host taking foreground. The foreground guard correctly refused input. The unchanged 150 ms case then passed standalone with no concurrent VM commands, including exact bytes, Undo/Redo, clean state, Exit and relaunch. Its report is `final-2f0-arm64-r11-150-live-44c8e60bed0844c9915a9d32bd0dcd58/150ms/evidence/live-edit-report.json`, SHA-256 `e650e7dff4af878be436b75664194dc3f0373aa746f11e497239fd00e2a9f911`. This does not rewrite the failed full-series report or qualify later packages.

The maintained r11 driver now reacquires an already-owned HWND/PID for main-window reads and retries only the observed provider exceptions within its existing budget. Native non-UI guards passed **27/27**, including exact initial-read and typed-unavailable red/green cases, changed-owner rejection and unchanged deadlines. This is test-driver evidence, not application acceptance.

## Failures retained and work still open

- Exact checkpoint Windows CI returned **342 passed, 2 failed, 0 skipped on each architecture**. The failures concerned a closed Go→Hashtags parent and a Recent-menu fixture counting Material implementation children as menu items. Corrections require the actual open ancestor path and real menu membership, preserving all assertions. Both native appearance gates passed **3/0/0**. The separate regression-proof steps were skipped after the full-suite failure; they did not pass.
- The x64 deployed-view report passed; ARM64 exited with **`0xC0000005` access violation**, with no completed report. The last capture preceded a checker stage that programmatically accepts/closes native theme pickers. This locates the boundary but does not prove the crash cause. Shared Mac/editor CI passed. [Windows run](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39787797715), [shared run](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39787521524).
- Nested View actions: all 15 menu choices and reopened check-state observations, retained-selection edit/Undo/Save passed in one run. Windows **Waiting for printer connection** then blocked Paginated Preview. A separate bounded diagnostic canceled that owned prompt once and observed the actual preview remaining gray at **0.0% for 12 seconds**. It closed normally with the document and disk unchanged. This is a confirmed application preview defect, addressed by [Cycle 162](../../research/cycle-162/README.md); no printer settings or spool jobs were changed.
- Clipboard follow-through now passes Copy Markdown, Copy HTML, Copy Formatted and the five exact Source Unicode Paste/Delete/Undo/Save checkpoints. The soft-break expectation was corrected against an independent Qt probe. The latest strict workflow still fails: the actual Find command visibly opens with `café` and `1 of 3` matches, but its saved-root accessibility traversal omits the visible Find field. Live/auxiliary search assertions were not completed. Report `final-2f0-arm64-r11-clipboard-7cef3d5f1ac645149482369ff90c9ccd/evidence/clipboard-menu-report.json`, SHA-256 `b218419e9b6d34346ab000dc3be72d1ac206fc4b1383535be84675fc8740cea4`. Public-baseline restoration was verified; all failed attempts remain retained.
- The original user clipboard restoration from an earlier revision 3 diagnostic remains **unknown**. Later workflows initialize and restore only an explicit public test baseline, and must never describe this as preserving the original user clipboard.

These observations do not qualify later Cycles 159–161, x64 hardware, installers, multiple monitors, all keyboard layouts, real printer output or extended daily-writing endurance. Final promotion requires the completed integration's exact CI/package and applicable ordinary desktop gates.
