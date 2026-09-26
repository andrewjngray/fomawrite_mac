# Cycle 81 — export gallery layout

Andrew's screenshots showed a horizontally shifted style heading, clipped card edges and a vertical scrollbar drawn over the cards. The gallery's duplicate-name field and button sat in one row wider than the available middle column. The default scrollbar also occupied the card edge. The original screenshots were used for diagnosis but were not copied into the repository.

The gallery now has a reserved right gutter, a slim vertical thumb and a fixed single-column content width. Copy controls stack vertically and long PDF option labels are shorter. The dialog follows the host window's available width and height, switches to compact layout below 900 px, and scrolls its options in compact mode. A separate detachable/resizable export window is not part of this cycle.

`./bin/build` passed. `./bin/test` passed 113 tests with zero failures and skips; the export UI regression checks wide and 720 × 520 host sizes, horizontal gallery bounds and dialog bounds. Both bundles were packaged locally. After the Dev process exited, its bundle was refreshed and launched on a disposable Markdown sample. The already-running Applications app was left untouched. No private-screen capture was made.

Andrew should inspect the revised dialog at his display scale, especially the middle column while scrolling. A visual PDF/page comparison and the wider release acceptance matrix remain open.
