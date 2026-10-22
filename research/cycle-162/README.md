# Cycle 162 — Windows printer-independent Paginated Preview

22 October 2026. Bounded repair of the existing Windows Paginated Preview command. Current build and qualification identity remain in [Windows STATUS](../../docs/windows/STATUS.md).

## Finding and change

The exact checkpoint158 ordinary Windows diagnostic showed a native "Waiting for printer connection" prompt before preview creation. Canceling that prompt opened QPrintPreviewDialog, but the viewer remained gray at 0.0% for 12 seconds. The document and saved bytes were unchanged. A publishing preview should not depend on the default printer being reachable.

Windows now opens a separate application-modal Qt Quick window using the existing PublishingPreview PDF component. Publisher generates its private temporary browser PDF; the existing PdfDocument/PdfPageImage path loads and lazily paints its pages. No QPrinter or printer-backed preview is constructed on this Windows route. The wrapper owns its temporary zoom and scroll; closing cancels the request and destroys its Publisher consumer, including cache/pin/renderer cleanup. Reopening starts a new consumer at 100%.

The Mac QPrintPreviewDialog implementation remains unchanged under the platform branch. Existing File Print and Page Setup commands remain available. The Windows preview has local zoom controls and no embedded printer/PageSetup toolbar. This repair introduces no Qt modules, external viewer or system printer configuration changes.

## Verification

- Tests-only checkpoint `19e0fb13b8500f86f11aa8a31159375a93a95779` red: **2 passed, 2 failed, 0 skipped**, **0.621 seconds**, exit2. Both rows require the missing printer-independent component before any old printer dialog is opened. Log: `evidence/old-code-red.log`.
- Final focused Source/Live gate: **4 passed, 0 failed, 0 skipped**, **11.046 seconds**, exit0. Log: `evidence/focused-green.log`.
- Actual white page captures paint **46,054 / 46,288** selected blue-CSS pixels per page on both Source/Live and both completed openings. All PDFs contain the correct Unicode headings, two letter-landscape pages (792×612points) and unsaved ending. Six public captures include the complete Source/Live preview windows; the complete Live frame was visually checked for white paper and readable black body text.
- Required `./bin/build` passed on Qt6.11.2 ARM64/macOS (`evidence/build.log`). The complete integration host suite passed **425/0/1**, **579.310 seconds** (`evidence/complete-host-suite.log`); the sole skip is Windows-only appearance. The later Windows-only unavailable-shortcut fixture correction is separately compiled/checked; its real negative branch requires native Windows CI. New native package qualification remains pending.

The first post-change test found that Ctrl+S in the transient viewer could activate the owner's WindowShortcut and save the dirty file. The wrapper now accepts ShortcutOverride for document commands, keeps Escape, leaves Alt+F4 to the OS, and handles StandardKey.Close locally. The final test requires zero saves and unchanged disk/dirty state after actual viewer key input. It closes one opening with Escape, another with the platform Close chord, and a third while refresh is pending.

Two fixture assumptions were corrected without weakening preservation: Backend::open retains the supplied file URL, so the test preserves its exact initial URL; Qt's QML Settings property cache writes initial defaults after a delay, so the test observes those defaults in storage before taking its whole-settings baseline. Every persisted key and relevant live workspace/layout property remains compared.

The Close-chord diagnostic also exposed a QtTest input-lifetime problem: its release helper processes events after the letter release, then sends modifier release through the original raw QWindow pointer. The loader can destroy that window between those steps. The final test presses the actual platform chord, requires window/consumer destruction, and sends the modifier key-release to the parent that now owns focus. Product close remains direct; no deferred-close workaround is retained. Intermediate failure/diagnostic logs remain in `evidence/`; they are not passing qualification.

The Mac preview method is byte-for-byte equivalent after removing the new Windows preprocessor branch: SHA256 `9224fd2310be6432b9754c73a7d314b6d0da52ea90794eab44a8ba359588e622` (`evidence/mac-preview-preservation.json`).

The regression requires actual two-page letter-landscape PDF text and real painted page-image pixels for a public Unicode/pagebreak/colored-CSS fixture. It also checks local zoom, read-only/modal input, close/reopen, close while refresh is pending, destruction of the Publisher consumer/renderer, and unchanged source, file identity, revision, dirty/Undo, selection, disk bytes, workspace layout/format/zoom and persisted settings.

Host rows expose the actual Windows File menu target, then open the same production wrapper after asserting ordinary native Mac menu boundaries. Native Windows rows additionally send synthetic Qt keyboard navigation and a final Qt pointer click to the actual File → Paginated Preview menu, require the Backend signal and reject a printer-backed viewer. Host QML evidence cannot prove the Windows platform branch, physical input, ordinary desktop paint or absence of OS printer prompts. Those require the verified Windows package and remain pending.

The existing Publisher request/document identities and canonical-text binding continue to reject stale render results and refresh on incoming Live changes. No new document barrier was added. Immediate in-flight Live input at menu entry is not separately qualified by these selected, unsaved-draft checks. Alt+F4, ordinary physical menu input, printer-independent native paint, and the original misconfigured-printer environment remain native acceptance gates. No VM input, clipboard action, system configuration change or package promotion was performed by this slice.

## Native shortcut fixture follow-through

The first negative-guard correction used the window content item. A real Qt focus probe showed that item is an already-active FocusScope: `hasActiveFocus` was true while Source still owned actual keyboard focus. The corrected test uses a plain child QQuickItem and requires `activeFocusItem()` equality. The probe reproduces the old focus error and proves actual focus transfer with persistent Source selection preserved. The separately compiled focused host suite passes **7/0/0**, 1.052 seconds; the actual unavailable-command branch still requires native Windows execution. This changes test focus only, not application shortcuts or AltGr behavior. Local evidence: `/private/tmp/fomawrite-altgr-focus-probe/result-persistent.log` and `/private/tmp/fomawrite-shortcut-plain-focus-focused.log`.
