# Writing workspace development

Authorized by Andrew on 9 September 2026: inspect the open iA Writer Mac app and independently implement a richer Omawrite in small tested batches. Prioritize a left file explorer and right live Markdown preview. Continue for roughly two hours, until 2026-09-09 13:35 UTC / 23:35 Sydney, then finish verification and hand over. Heartbeat automation: build-omawrite-writing-workspace.

## Reference observations

Inspected iA Writer with computer use. Current layout has organizer/library on the left, Markdown editor in the centre and HTML preview on the right. View menu exposes tree/list navigation, filename/date sorting, file filter, text size, split/full preview and several preview typography templates. Writing tools include bold, italic, strike, link, table and TOC. The reference document was our own AGENTS.md; no user writing was edited.

## Planned batches

1. File explorer with folder selection, lazy expansion, visible-file filter and new document; three resizable panes; independently scrolling live Markdown preview supporting Qt's Markdown dialect, local images, tables and task lists. Preserve safe switching.
2. Persistent pane/typography preferences, source-markup toggle, typewriter mode and paragraph focus; richer formatting controls and statistics.
3. Outline/quick-open/recent files and export improvements as time allows. Compare against iA Writer using example documents, not private content.

## Current status

Implementation starting from clean af93f14. Existing 13 tests pass. No new feature has been verified yet.

## Rules for continuations

Read this log and git status first. Continue unfinished work, never reset it. Test meaningful editing/data-preservation behaviours and inspect the actual app. Commit and push stable milestones, record exact tests and limitations, and package dist/Omawrite.app. Do not start new features after the deadline; pause the heartbeat after the final handover. Avoid routine questions. Keep notifications quiet unless a meaningful milestone, failure or final handover occurs.

## Batch 1 — three-pane workspace (11:48 UTC)

Implemented a lazy, watched Markdown/text library with expandable folders, visible-file filtering, folder/file creation without overwrite, and remembered root. Added resizable library/editor/rendered-preview panes and persisted Editor/Split/Preview layout. Preview updates after 120 ms and resolves image resources relative to the document. Library switching uses the existing unsaved-change dialog. Normalized macOS symlink paths after a failing expansion test. Opening a document now resets the editor scroll to its start. Native screenshot checked all three panes; subsequently neutralized the Material toolbar background.

Validation: `./bin/build` passed; `./bin/test` 15 passed, 0 failed (library safety and preview source preservation included). Qt Material emits an intermittent SplitView teardown warning in tests; no runtime warning on launch. Not yet verified: local image preview, relative document links (currently external URL handling only), narrow layouts, final bundled packaging. Current preview is Qt Markdown, not browser CSS; paragraph spacing needs refinement.

Reference: iA View menu directly inspected; [official focus documentation](https://ia.net/writer/support/editor/focus-mode/focus-mode-mac) confirms sentence/paragraph dimming and vertically centred typewriter mode. CUA later returns a stale View menu for iA and no screenshot, so further reference inspection may need a fresh app selection.

## Batch 2 — writing presentation (11:50 UTC)

Added persistent Aa controls: visible Markdown syntax (default), paragraph focus (physical text paragraph/line), vertically centered typewriter scrolling, 12–32 px writing size, and Sans/Serif/Mono preview typefaces. The editor remains the original source document; preview formatting operates on a separate read-only document. Improved preview line/paragraph spacing and neutral toolbar typography. Relative Markdown/text links resolve against the document folder and use protected file switching. Added examples/Workspace-tour.md.

Validation: build passed; 17 tests passed, including source/undo preservation under syntax/focus changes, local image resources, relative link resolution and cursor centering. Native screenshots confirmed focus dimming, source markers, preview spacing, tables and task checkboxes. Runtime logs clean. Batch 1 timestamp above was estimated; clock at end of batch 2 was around 11:50 UTC.

Remaining: preview fragment-only anchors not handled; library filter only searches visible expanded directories; no outline/quick open/global search yet; preview font is available system fonts and retained OFL mono. Package again after next batch or before handover. CUA app chooses earliest process by app bundle: close only known unchanged test windows with the AX close button, then select app again to inspect latest process. Keyboard commands did not reliably close these windows.
