# Menu audit — 12 September 2026

Status: **in progress; native capture blocked pending approval for incidental private-window visibility.** No complete screenshot collection is claimed.

## Evidence

- iA Writer File menu was opened today and its command tree read. Recent document names were excluded from the written inventory.
- Other iA menus below use the existing 10 September native inventory in docs/ia-writer-inventory.md; they still need fresh screenshots.
- Typora access was rejected by automatic review because its initial app state can reveal private writing. Its menu names currently come from Andrew's request, not inspection.
- Omawrite menu commands are verified against src/Main.qml. The running Dev app may lag the packaged code; cycle 13 refresh remains pending unsaved-document handling.
- App screenshot capture returned unavailable for the native iA menu. A desktop capture did not show the requested app and was not retained in this project or used as menu evidence.

## Capture checklist

| App | Menus requested | Screenshots |
|---|---|---|
| iA Writer | File, Edit, Format, Authors, View, Focus, Go, Window, Help | Pending |
| Typora | File, Edit, Paragraph, Format, View, Themes, Window, Help | Pending |
| Omawrite Dev | App, File, Edit, Format, View, Help | Pending |

Capture each top-level dropdown and each static submenu separately. Record disabled items and keyboard shortcuts; do not execute commands just to inventory them. Dynamic Recent/Window lists must be excluded or redacted before any shared capture. Raw screenshots, if approved, stay local and untracked. Store menu-only reviewed images under the respective app folders.

See docs/menu-comparison.md for the working matrix and proposed sequence. No product code changed during this audit.
