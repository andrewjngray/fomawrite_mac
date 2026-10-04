# Cycle 107 — stable document view controls

Date: 4 October 2026. Review target: **0.3.0-dev8 / macOS 0.3.0 (107)**.

Andrew reported that Source, Split, Full and Visual Edit caused the lower controls to move and made the document view unclear. The previous implementation owned separate Source and Preview footers, removed one in Full, and inserted an extra Source-return control in Visual Edit.

## Changes

- One persistent document footer spans Source and Preview. Source / Split / Full remain together at the lower right, with Visual Edit always beside them. All four retain their coordinates at a fixed window size.
- Source opens canonical Markdown and turns visual editing off. Split and Full preserve the rendered editing mode. Visual Edit enters Full from Source, or edits the right pane in Split; turning it off retains the arrangement and shows read-only Preview.
- Writing appearance and Preview template remain at the left in every view. A compact Aa menu keeps both reachable below 660px of document width. A 320px document region fits all view controls. Split is disabled when its two minimum pane widths cannot fit, with a widening hint.
- Layout changes suspend synchronized scrolling while the two panes rewrap, then restore their reading positions. Hidden Source caret changes no longer scroll the Preview. Opening a document or actual keyboard/pointer editing and scrolling takes precedence over pending restoration.
- Preview scroll extent uses only its visible document. Leaving a long Visual Edit document no longer leaves a stale blank scrolling tail in Preview or in a subsequently opened shorter document.
- F6 / Shift+F6 recognizes the shared footer, and compact resizing rescues focus from a hidden style control. View changes restore the appropriate writing focus. Native menu and shortcut view changes use the same transition path.
- Pressing a toolbar button dismisses its pending tooltip so the trigger hint cannot cover its menu. No Markdown or saved appearance migration is introduced.

## Verification and review

- `./bin/build` and the final package rebuild pass. [Build log](build.log), [package log](package.log).
- Full `./bin/test`: **150 passed, 0 failed, 0 skipped**. [Log](test.log). The log retains known font-substitution and asynchronous teardown warnings; no test is skipped or weakened to hide them.
- Six affected native Cocoa workflows: **8 passed including setup/cleanup, 0 failed, 0 skipped**. [Log](native-footer.log). These use actual pointer clicks through Source, Split, Full, Visual Edit and appearance/template menus. Native mouse-only fixtures are non-activating and isolate unsolicited OS typing; separate offscreen keyboard tests exercise F6 / Shift+F6 and compact focus recovery.
- New repeat-cycle checks cover 1440, 1280, 960, 739 and 720px windows, light/dark, three repeated sequences, checked states, footer alignment, control bounds and untruncated view labels. The 739px case leaves approximately 321px for the shared footer, testing its practical minimum.
- Long-document checks repeat Full/Source/Split/Visual transitions with synchronized scrolling on/off and navigation expanded/collapsed. They retain exact Unicode Markdown, selection, cursor, dirty state, Undo/Redo, disk bytes, preferred pane widths and Source scroll position within 2px on return to the same geometry. They also check Preview range after a Visual Edit roundtrip, switching to a shorter document, and opening a document during an outstanding transition.
- Native screenshot review caught clipped short labels despite passing bounds checks. Reduced internal padding fixes the labels without moving the controls; explicit Text.truncated checks now prevent recurrence. Pre-polish geometry assertions wait for Qt's completed layout rather than treating an intermediate rectangle as a painted frame. The native log may record this transient while waiting; final geometry and captures pass.

## Native UI captures

These are native Qt captures of the production Main components with disposable documents, not mockups.

- [Source](screenshots/cycle107-view-1440-1-0.png)
- [Split](screenshots/cycle107-view-1440-4-1.png)
- [Full Preview](screenshots/cycle107-view-1440-0-2.png)
- [Visual Edit](screenshots/cycle107-view-1440-2-2.png)
- [Minimum document bar](screenshots/cycle107-view-739-2-2.png)
- [Compact template menu](screenshots/cycle107-compact-template-menu.png)
- [Dark appearance](screenshots/cycle107-view-dark.png)

Review confirms one aligned footer, fixed right-hand controls, full short labels and readable active/disabled states. Navigation may contract responsively at smaller window widths; the right-hand view controls remain anchored.

## Review builds

Stable `dist/Fomawrite Dev.app`, ordinary/demo `dist/Fomawrite.app` and `/Applications/Fomawrite.app` are synchronized to build107, with strict/deep local signatures and matching demo/Applications executables. [Exact identities](artifacts.json), [Dev preparation](dev-install.log), [Applications update and preserved previous copy](install.log).

[Optional review exercise](../usability/cycle-107.md). [Shared UI standard](../../docs/workspace-ui-redesign-plan.md).

Physical VoiceOver/display-scale acceptance and Andrew's normal writing review remain separate checks. Public RC1 is unchanged; broader visual image/table/list ergonomics remain separate work.
