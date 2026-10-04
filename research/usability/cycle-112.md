# Cycle 112 — comfortable independent reading sizes

Review target: **0.3.0-dev11 / macOS 0.3.0 (112)**, verified in Dev, demo and Applications. Confirm the version and executable path in About; see the [verification record](../cycle-112/README.md).

Use a disposable copy of a longer Markdown document.

1. Open Split. Set Source to 125% and Preview to 150% using the percentage menus. Each minus/plus group should change only its own pane; the formatting/template selection should remain unchanged.
2. Scroll both panes into the document. Try 125%, Reset to 100%, then the same sequence several times. The passage at the top should remain in view without a gradual line-by-line drift. Repeat in Visual Edit.
3. Enable Link zoom from Preview’s menu. Both percentages should adopt Preview’s value and move together. Turn linking off; each pane should keep its current value and become independent again.
4. Switch Source, Split, Full and Visual Edit repeatedly. The controls should stay in the appropriate header, the percentages should be retained, and the bottom mode controls should stay put. Click a writing pane, then use the View menu’s text-size command to check that it targets that pane.
5. Drag the writing divider, then double-click it. Double-click should balance the available writing area while respecting minimum pane widths. Try ordinary dragging again. Narrow the window with navigation collapsed and check that neither percentage group overlaps its neighbours; repeat in dark appearance.
6. Type an unsaved sentence, change zoom, then Undo and Redo. Only the sentence should be undone/redone. Exporting at two zoom values should retain the same output typography and page size; zoom should never save the source without your request.
7. Close and reopen the review app after saving or deliberately discarding the disposable draft. Both independent percentages and the Link setting should return as selected.

Note the mode, percentages, window width and input method if text jumps, a divider stops responding or controls become clipped. Physical mouse/trackpad and keyboard observations add coverage beyond the synthetic native checks.
