# Cycle 170 — Split view pane resizing

Build 170 (`0.3.0-dev65`).

At a 1,295-pixel window width with both navigation panes visible, the Source pane could already be at its 480-pixel minimum. Qt's sidebar divider used only the Source pane as the space donor. Dragging the Library or file-list divider therefore did nothing even when the PDF pane still had room to narrow.

In Split view, the navigation dividers now take space from the Source pane above its minimum and then from Preview, the other sidebar, or the review pane as needed. They still respect the existing pane minimums and maximums. The document divider continues to adjust the balance between Source and Preview.

## Verification

- A disposable PDF in a 1,295-pixel window reproduced the stalled Library divider on build 169 with Source Find open.
- The focused Mac suite passed six checks on build 170. It verified dragging both sidebar dividers, dragging the document divider, preserving the Find query and note text, and the existing footer and double-click behaviour.
- The new drag regression, Source Find and the footer check passed in native Cocoa. The stable Dev bundle was prepared and passed strict signature verification.
- The full Mac regression suite passed all 430 checks. The signed Dev bundle passed the native document-view harness. [Native report](native-bundle/report.json)
- [Hands-on checklist](../usability/cycle-170.md).
