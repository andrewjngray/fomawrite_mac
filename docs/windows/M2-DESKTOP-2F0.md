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

## Failures retained and work still open

- Exact checkpoint Windows CI returned **342 passed, 2 failed, 0 skipped on each architecture**. The failures concerned a closed Go→Hashtags parent and a Recent-menu fixture counting Material implementation children as menu items. Corrections require the actual open ancestor path and real menu membership, preserving all assertions. Both native appearance gates passed **3/0/0**. The separate regression-proof steps were skipped after the full-suite failure; they did not pass.
- The x64 deployed-view report passed; ARM64 exited with **`0xC0000005` access violation**, with no completed report. The last capture preceded a checker stage that programmatically accepts/closes native theme pickers. This locates the boundary but does not prove the crash cause. Shared Mac/editor CI passed. [Windows run](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39787797715), [shared run](https://github.com/andrewjngray/fomawrite_mac/actions/runs/39787521524).
- Nested View actions: all 15 menu choices and reopened check-state observations, retained-selection edit/Undo/Save passed in one run. The workflow then encountered Windows **Waiting for printer connection** before Paginated Preview. One owned Cancel unblocked the native preview window; rendered content and normal preview close are still separate checks. No printer settings or spool jobs were changed.
- Clipboard revision 8 passed Copy Markdown, exact Unicode Paste/Delete/Undo/Save and Copy HTML, then rejected Copy Formatted. The fixture expected a hard newline for a Markdown soft break; an independent Qt rendering probe produced a space. Revision 9 changes that one expected value and still requires an exact Windows result. The failed run remains evidence, not a pass.
- The original user clipboard restoration from an earlier revision 3 diagnostic remains **unknown**. Later workflows initialize and restore only an explicit public test baseline, and must never describe this as preserving the original user clipboard.

These observations do not qualify later Cycles 159–161, x64 hardware, installers, multiple monitors, all keyboard layouts, real printer output or extended daily-writing endurance. Final promotion requires the completed integration's exact CI/package and applicable ordinary desktop gates.
