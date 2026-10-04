# Cycle 111 — Find and Replace while writing

Review target: **0.3.0-dev10 / macOS 0.3.0 (111)** after the final refresh. Verify the app version/path in About.

1. Place the caret midway through a document and open Find. Search a repeated phrase; the first selected result should be at or after the caret, wrapping if needed.
2. Use the arrow buttons, Return and Shift-Return. Match counts and control locations should remain clear.
3. Show Replace, replace one result, then Undo. Try replacing a word with a longer phrase containing the same word; the next action should advance rather than repeatedly replace its own insertion.
4. Leave Find open, click Source and edit an unrelated sentence. Typing should stay at that caret while the result count updates.
5. Try a query with no matches, a narrow window and dark appearance. Disabled actions and feedback should be understandable.
6. Immediately after switching Full/Visual, open Find. It should retain keyboard focus. Escape should return to Source, and changing to Full should close hidden Find controls.

Use a disposable document for Replace All. Note any unexpected caret movement or difficult-to-read controls; the saved original should remain unchanged until you choose Save.
