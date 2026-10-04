# Cycle 118 — editing and layout review

Target: **0.3.0-dev14 / macOS 0.3.0 (118)**. All 178 regressions, 12 focused Cocoa checks and 147 actual-executable checks in each of the three refreshed app copies pass; check the [handoff status](../cycle-118/README.md) and running About version/path before starting.

Use a disposable Markdown file with ordinary paragraphs, a simple list and a small supported table.

1. Choose **Source**, then **Single**. The source editor should fill the document area. Choose **Split**: the same source editor remains at left and read-only output Preview appears at right.
2. While still in Split, choose **Visual Edit**. The left pane becomes the visual editor and Preview remains at right. Edit an ordinary paragraph on the left; the output should update. Try typing on the right: Preview must remain read-only.
3. Choose **Single**. Visual Edit should remain selected and occupy the document area. Choose Source and then Split again. Repeat this circuit several times; neither capsule should move or change its full button labels.
4. Change the editor appearance below the left pane and the output template below Preview independently. Drag the divider and narrow/widen the window: each menu should stay with its pane, shrinking before the fixed editing/layout controls. At minimum width the compact Aa label should remain readable, with no overlap.
5. Adjust each pane’s zoom. The left zoom controls the chosen editor, whether Source or Visual Edit; the right zoom controls output Preview. Switching Single/Split must preserve those choices. Try linked zoom separately if useful.
6. Enter **Preview only** through the view command. The document is read-only and neither footer capsule is selected. Choose Source or Visual Edit to resume editing; choose Single/Split to resume the retained editor in that layout.
7. Open Find while in Visual Edit: it should reveal Source in the same Single/Split arrangement. Switch back to Visual Edit and confirm Find closes. A Source completion popup should also close when you change editor, and an old hidden Source selection must not become a clipboard target.
8. Make one deliberate text edit, repeat mode/layout changes, then Undo and Redo. Those changes should affect only the text edit, not create extra Undo steps. Try the same circuit in dark appearance and a narrow window.

Record the running version/path, editing choice, layout and action if the grouping is unclear, a control moves or a selection/reading position is lost. Physical input, IME, VoiceOver and display-scale review remain valuable beyond automated native checks.
