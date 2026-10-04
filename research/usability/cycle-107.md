# Cycle 107 — steady view controls

Review build: **0.3.0-dev8 / macOS 0.3.0 (107)**.

1. Open a longer Markdown file and scroll halfway down. Select a phrase without editing it.
2. Use the lower-right Source, Split and Full buttons repeatedly. Their positions should stay fixed; returning to Split should retain the selection and reading position.
3. In Split, toggle Visual Edit, switch to Full, return to Split, then toggle Visual Edit off. Source remains the same direct button throughout.
4. Toggle synchronized scrolling and repeat. Resize the window, then collapse/reopen Library and Organizer. Their requested widths should remain intact.
5. In a narrow document area, use the lower-left Aa button to choose writing appearance or preview template. Widen again to see the two separate style controls. At a window narrower than 803px, Split is disabled with a widening hint.
6. Hover a style button until its hint appears, then click. Its menu should appear without that hint covering it.

Report the view before/after any remaining jump and whether synchronized scrolling was enabled. The document may rewrap when its width changes; the mode controls should not relocate.
