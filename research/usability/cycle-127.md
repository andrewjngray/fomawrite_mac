# Cycle 127 — optional publishing-preview review

Use **0.3.0-dev23 / macOS 0.3.0 (127)** installed in `/Applications/Fomawrite.app`; see the [passing automated checks](../cycle-127/README.md).

1. Open the document that showed the empty publishing pane. In Split, confirm Web shows the heading and body even if an image is missing; the image should show its alt text in a placeholder and a warning should explain unavailable assets.
2. Switch to PDF and confirm the same document remains readable. Scroll, change preview zoom, resize the divider and repeatedly choose the current theme. Watch for blank frames or flashing while unchanged output refreshes.
3. In a disposable sample, change text or a local image and confirm Web/PDF eventually show the new content without displaying a superseded result. Change paper settings and confirm PDF updates.
4. Try exporting a disposable sample with a missing image. Export should report the image problem; a valid sample should export normally.
5. Return to your writing and confirm its text, saved/Edited state and Undo/Redo remain correct. Report the build number, Web/PDF choice and the action immediately before any remaining blank or flash.

Use private writing locally; avoid committing its screenshots or exports. Acceptance of Andrew's original physical workflow remains open until this review.
