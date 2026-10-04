# Cycle 117 — pane and library review

Target: **0.3.0-dev13 / macOS 0.3.0 (117)**. All 176 regressions, 12 focused native checks and all three local app executable gates pass. Dev, ordinary/demo and Applications are refreshed and signature-verified. Check the [handoff record](../cycle-117/README.md) and About before starting.

Use a disposable saved Markdown file whose filename differs from its first heading, plus a sibling file and a nested folder.

1. Open Split. Check that writing appearance is below Source and the output template is below Preview. Change each independently, then drag the divider: the menus should remain in their own panes.
2. Narrow and widen the window. Visual Edit should remain between the template picker and the fixed Source / Split / Full group, using its icon at compact sizes. Repeat all view changes and return to Source; controls must remain reachable without overlap.
3. Read the Preview header in Split and Full. It should show a document icon and the actual filename. Type an unsaved sentence: **— Edited** should appear. Save it and confirm the marker disappears. Try a long filename in a narrow pane.
4. Enable text excerpts and Date Modified in the library. Expect a document icon and bold filename, a short grey date to its right, and no more than two compact grey excerpt lines below. Selecting a row must open that exact file, including when its heading differs from its filename.
5. In Tree navigation, expand a folder and check its real children are indented consistently. In List navigation, opening a folder should show its sibling contents at one level. Try selection, right-click actions and the plain-list option again.
6. Repeat in dark appearance with an unsaved sentence. Menu choices and layout changes must not edit the writing; Undo/Redo should still affect only deliberate text edits.

Record the running version/path, view, pane widths and action if a label becomes unclear or a control overlaps. Physical mouse, keyboard, IME and VoiceOver review remains valuable beyond the automated native checks.
