# Cycle 167 — Mac PDF note popover

Build 167 (`0.3.0-dev62`) in Fomawrite Dev.

| Finding | Behaviour |
| --- | --- |
| Clicking the page with the Note tool moved the writer into Split and inserted an empty note. | The page click opens a focused editor beside the chosen point. Done adds the note and its pin in one Undo step. Escape and Cancel discard the draft. |
| Clicking a pin revealed its associated Markdown or a shortened preview. | A pin opens the complete note body for editing. Done replaces that body in place. The reader keeps the current layout. |
| Notes and highlights share a Markdown companion. | Save writes the existing `.pdf.md` companion. While editing a note, Save applies a valid draft first. Headings that would end a note and fences that would swallow the next marker can be edited through the Markdown surface. |

## Verification

- Both new regression tests failed on build 166 at the missing popover. With the popover in place, the focused PDF suite passed all nine QtTest checks, including the retained tests for pin placement, move, delete, and note history in Source and Live.
- The full Mac regression suite passed with 423 passes, zero failures and zero skips.
- The Mac app built and the stable Dev bundle passed strict signature verification. The distribution and Applications bundles are on build 166.
- The signed Dev bundle passed the native document-view harness, including both existing PDF reader and highlighting checks. The report also verified the embedded QML against the checkout. [Report](native-bundle/report.json)
- The two new regression tests also passed in a native Cocoa window with synthetic pointer and keyboard input. The local capture shows the popover over a disposable PDF. [Capture](captures/cycle167-pdf-note-popover.png)
- [Hands-on checklist](../usability/cycle-167.md)
