# Cycle 109 — Return inside a list

Review target: **0.3.0-dev10 / macOS 0.3.0 (111)** after the final refresh. Check the running copy in **Fomawrite → About Fomawrite**.

1. In a disposable document, enter `- firstsecond` and switch to Visual Edit. Put the caret before `second` and press Return. Expect two bullet items and the caret before the second half.
2. Type a few words, Undo that typing, then Undo the split. Redo both changes. Confirm the neighboring source stays unchanged.
3. Repeat with a numbered item and a checked task. The new task should be unchecked; following numbered source is not automatically renumbered.
4. Try a split inside bold text or a nested/continued item. Unsupported boundaries should keep the source unchanged and offer Source editing.

Record the example Markdown and caret location for any refusal that feels surprising. Avoid using important writing for the first check.
