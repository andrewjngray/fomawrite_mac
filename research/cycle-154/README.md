# Cycle 154 — Fomawrite as the Mac's default text editor

Build 154 (`0.3.0-dev50`), 28 October 2026. Andrew, from Codex's Open menu on a `.txt` file: Fomawrite should be the default editor for text and Markdown files, and at least listed with the others, so files open directly in it; and it should be part of the install.

What was found:
- `.md` already opened in Fomawrite by default; `.txt` opened in TextEdit, and Fomawrite was not offered for it. The app declared its file types by extension only (`md`, `markdown`, `txt`, rank Alternate), which current macOS largely ignores for plain text.
- Codex's Open menu (VS Code, Cursor, Sublime Text, Windsurf, terminals) is Codex's own fixed list; nothing on the Mac adds an app to it. Its **Default app** entry opens the file's macOS default, so with Fomawrite as the default it opens in Fomawrite.
- Launch Services held about 25 registrations of Fomawrite, most for folders that no longer exist (old install staging folders, deleted worktrees).

What changed:
- `macos/Info.plist` declares Markdown (`net.daringfireball.markdown`; `md`, `markdown`, `mdown`; rank Default) and plain text (`public.plain-text`; `txt`, `text`; rank Alternate) by content type, and imports the Markdown type. Fomawrite now appears in Open With for every plain-text kind (logs, CSV and so on keep their own defaults).
- `bin/set-default-editor` makes Fomawrite the default app for Markdown and plain text, waits for Launch Services to settle, and reports what it set.
- `bin/install-mac` registers the installed bundle, forgets registrations whose folders are gone, and runs `bin/set-default-editor` when installing to /Applications. `FOMAWRITE_KEEP_DEFAULT_APPS=1` skips the default-app step.
- `bin/prepare-dev-app` sets the Dev bundle's handler rank to None and re-registers it, so "Fomawrite Dev" no longer appears in Open With lists.

Verified on Andrew's Mac after installing build 154: the default app for `.md`, `.markdown`, `.txt` and `.text` is /Applications/Fomawrite.app; Fomawrite is listed for `.log` and `.csv`; the Dev bundle is not listed. No Qt or page code changed, so the test suites were not re-run.
