# Grammarly experiment — ten minutes at the keyboard (for Andrew)

Purpose: decide the spell-check entry in IDEAS.md with evidence instead of guesses. Nothing to build first; build 136 is installed.

1. Install **Grammarly Desktop for Mac** (the overlay app, not the browser extension) and sign in with your account.
2. Open `/Applications/Fomawrite.app`, open any document, and type a sentence with two obvious errors ("Their going too the shop tomorow.") in **Source** mode. Note: does the Grammarly indicator appear, does it underline, can you accept a suggestion?
3. Switch to **Live** and type the same sentence. Note the same three things. Live is a Chromium page inside the app, so this is the case most likely to behave differently.
4. Try once in the **link panel** field in Live and once in the **Find** field, to see whether the overlay attaches to small fields.
5. Quit Grammarly Desktop and note whether the editor feels any different (the "little bug chasing around" problem you described).

What the answers decide:
- Works in both surfaces: the spell-check entry becomes "rely on Grammarly Desktop; add a View toggle that hides nothing of ours but documents how to pause Grammarly" (we cannot turn their overlay off from inside the app; a per-app disable exists in Grammarly's own settings).
- Works in Source only: Live needs its own spell check (bundled dictionaries) or Grammarly stays a Source-mode habit.
- Works in neither: native spell check for Source plus dictionaries for Live, Grammarly stays in Chrome.

Screenshots of each step would be useful; drop them in `research/overnight-2026-10-08/` and I will fold the result into IDEAS.md.

## Result (Andrew, 9 October 2026)

Grammarly Desktop is installed and works in Word on the same Mac. In Fomawrite it attaches to **nothing**: Source, Live, the link panel and the Find field all stay silent and the floating G never appears.

### Why (probe `accessibilityProbeForWritingAssistants`, `tests/cycle142-accessibility-probe.inc`)

Grammarly Desktop reads other apps through macOS Accessibility. Qt is the only route from Fomawrite to that API, and the probe shows what Qt offers:

| Surface | Role and state | Text, cursor, selection | Character bounds (`AXBoundsForRange`) | Point → offset |
| --- | --- | --- | --- | --- |
| Source (Qt Quick `TextEdit`) | editable text, focused, editable | yes, with an editable-text interface | **empty rect for every offset** | **-1** |
| Live (Qt WebEngine page) | editable text, focused, multi-line (on Cocoa only; nothing offscreen) | text and cursor yes; no editable-text interface | **empty rect for every offset** | **0** |

Both gaps are upstream stubs, not Fomawrite code: Qt Quick's `QAccessibleQuickItem` declares `characterRect` as `return QRect()` and `offsetAtPoint` as `return -1`; Qt WebEngine's `BrowserAccessibilityInterface` marks both `QT_NOT_YET_IMPLEMENTED`. Without character bounds an assistant cannot place a G button or an underline, which matches what Andrew saw.

The Text Editor SDK route is closed: Grammarly discontinued it on 10 January 2024.

### What could still be done

- **Source only, in-app:** install a `QAccessible` factory for the Source editor that implements `characterRect` from `TextEdit.positionToRectangle` and `offsetAtPoint` from `positionAt`, mapped to screen coordinates. About two hours plus a hand test. Unproven: Grammarly may need more than bounds.
- **Live:** no in-app fix; it would need a patched Qt WebEngine.
- **Floor that is fully ours:** the system spelling and grammar checker already behind the Spelling and Grammar dialog, wired into Live as underlines with a toggle.

Running the probe on the Cocoa platform shows a real window for two seconds and takes keyboard focus; run it offscreen (`./bin/test accessibilityProbeForWritingAssistants`) unless the desk is free.
