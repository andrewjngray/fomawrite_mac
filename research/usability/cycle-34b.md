# Try safe version restore

On a disposable saved document:

1. File → Create Version of Saved File.
2. Edit a paragraph.
3. File → Restore Version in Editor and select the saved version.
4. Confirm historical text returns and autosave is paused.
5. Undo should return the previous draft. Save explicitly when satisfied.

Native versions contain Markdown only; historical text has no authorship labels. Undo restores the labels belonging to the previous draft.
