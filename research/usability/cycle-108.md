# Cycle 108 — lower controls and resizing

Review version: **0.3.0-dev9 / macOS 0.3.0 (108)**. Open **Fomawrite → About Fomawrite** to identify the running version and path.

1. At a fixed window size, repeat Source → Split → Full → Visual Edit → Split → Source. The four view controls remain together at the lower right; only their selected state changes.
2. In Split with Visual Edit on, focus Source, shrink the window until only Source fits, and click Visual Edit. It should open in one click.
3. Repeat after focusing Visual Edit. Turning it off in the narrowed window should leave full-width read-only Preview.
4. Grow a contracted Split window just far enough to enable Split, then click it. Both writing panes should appear immediately.
5. Use the View menu and toolbar view menu; their selected states should agree with the visible panes and footer. Appearance and Preview template choices stay separate from the view buttons.
6. Check a longer document halfway down, with a selected phrase. Return to the same view and width; reading position, selection and document text should survive.

Resizing changes text wrapping and may contract navigation columns. The view button group remains at the lower right; a change in document geometry is not a reason to insert, remove or relocate Source.
