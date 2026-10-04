# Cycle 110 — find a section in the outline

Review target: **0.3.0-dev10 / macOS 0.3.0 (111)** after the final refresh.

1. Open a document with several headings through **View → Document Outline**. Check the levels, indentation and marker for the current Source section.
2. Type part of a heading. Use Up/Down and Return to jump. Confirm the exact intended section receives the Source caret.
3. Search for something absent, press Return, then clear the search. The empty result should do nothing; clearing should restore the heading list.
4. Open the outline with text selected, browse several results, then Escape. The original selection should remain.
5. Narrow the window, enter Full Visual Edit, and choose a heading from the outline. Source should become visible at that heading.

Note long-title readability, keyboard behavior and whether the current-section indicator helps. Filtering alone should never alter Markdown.
