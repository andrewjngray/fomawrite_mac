# Cycle 169 — Source Find and PDF note cards on Mac

Build 169 (`0.3.0-dev64`).

- Source Find and Replace centre typed text using symmetric vertical padding and background insets. The inline prompts stay centred in empty fields.
- Hovering over a PDF note pin shows a short plain-text preview and the stored author when available. Right-click offers **Copy Note Text** and **Copy as Markdown Quote**. The quote includes a page link.
- Single-clicking a pin opens a movable note card. Drag its title area to uncover the PDF. **Done** applies changes, the red close icon discards the draft, and the yellow trash icon deletes the note in one undoable edit. The context menu's existing **Remove Note** action still removes the pin and leaves its body in the notes.
- New notes store author and creation time in the existing Markdown marker. Fomawrite takes the author from its authorship profile or the local account name. The footer shows creation time in local time. Old notes display **Creation date unavailable** when their marker has no creation time.

## Verification

- The three new regressions reproduced off-centre Source Find text, disabled note copy actions and missing note metadata against build 168.
- The full Mac suite passed all 429 checks with zero failures and skips.
- All six focused QtTest checks passed. The six native Cocoa checks also passed, covering Find and Replace geometry, pin clipboard and hover, moving and deleting the card, undoing deletion, and the original note workflow. [Split Find capture](captures/cycle169-split-find.png) · [Note card capture](captures/cycle169-pdf-note-card.png)
- The signed Mac bundle passed the native document-view harness, including both PDF reader and highlighting checks, with its embedded QML verified against the checkout. [Native report](native-bundle/report.json)
- [Hands-on checklist](../usability/cycle-169.md).
