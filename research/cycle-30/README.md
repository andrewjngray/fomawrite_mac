# Cycle 30 — Open by Path

File → Open by Path…; Shift–Command–O on macOS. Accept absolute paths, home-relative paths, paired quotes, and local file URLs. Folders change the library while retaining the editor; files use the existing save/discard/cancel guard. C++ validation rejects missing, unreadable, remote and unsupported file paths without executing input.

Build and 53 tests passed (`logs/build.log`, `logs/test.log`). Native checks: shortcut opens dialog; home-relative sample path with spaces opens the document; folder path changes the library and retains document; dirty sample prompts and Cancel retains text; missing path shows inline error. Screenshot `path-error.png` inspected; only sample text is visible. User's previously open clean document was closed normally before Dev replacement; only agent-created edits were discarded at QA completion.

Both app artifacts updated; packaged app locally ad-hoc signed, not notarized. No relative paths, shell expansion or completion. Dark/narrow and permission-revocation races remain unverified. The broader closeout audit remains open.
