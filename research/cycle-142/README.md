# Cycle 142 — Grammarly experiment and the Source accessibility adapter

Build 142 (`0.3.0-dev38`), 9 October 2026.

## What happened

1. Andrew installed Grammarly Desktop and tried it against build 141: it works in Word, attaches to nothing in Fomawrite (Source, Live, link panel, Find), and its floating G never appears. Result recorded in [grammarly-experiment.md](../overnight-2026-10-08/grammarly-experiment.md).
2. A probe test (`accessibilityProbeForWritingAssistants`, [tests/cycle142-accessibility-probe.inc](../../tests/cycle142-accessibility-probe.inc)) asked Qt's accessibility layer what each surface exposes. Both report an editable, focused text area with text, cursor and selection, but **empty character bounds and no point-to-offset answer**. Both are upstream stubs: Qt Quick's `QAccessibleQuickItem` (`characterRect` returns `QRect()`, `offsetAtPoint` returns -1) and Qt WebEngine's `BrowserAccessibilityInterface` (`QT_NOT_YET_IMPLEMENTED`). On the Cocoa bridge those become `AXBoundsForRange` and `AXRangeForPosition`, which an overlay needs to place a button or an underline.
3. Andrew chose the Source-only attempt. [src/sourceaccessibility.cpp](../../src/sourceaccessibility.cpp) installs a `QAccessible` factory for the Source editor only: a `QAccessibleQuickItem` subclass that answers `characterRect` from the text item's `positionToRectangle` (widened to the next caret position on the same line) and `offsetAtPoint` from `positionAt`, both mapped through the item to screen coordinates, and keeps the editable-text interface (replace, insert, delete through the item's own `remove`/`insert`). Needs `QT += quick-private` for the private header, so the build is tied to the installed Qt minor version.
4. The probe now asserts the Source geometry (real rectangles, left-to-right order, round trip through `offsetAtPoint`, out-of-range rejected) and the editable interface, and still reports Live's tree for the record.

## Hand test to do (Andrew)

Open build 142, Source mode, type "Their going too the shop tomorow." and watch for Grammarly's G. If it appears and underlines, Grammarly in Source is real; if not, the adapter is not enough and the system checker route is the one left.

## Not done

- Live: no in-app fix is possible without a patched Qt WebEngine.
- No reviewer pass on the adapter yet (small, single file, probe-tested); schedule one if the hand test succeeds and the feature stays.
- Running the probe on the Cocoa platform shows a window for two seconds and takes focus; keep it offscreen.

## Verification

Qt suite 222 / 0 offscreen (`./bin/test`). Build 142 packaged, Dev bundle refreshed, installed with the app closed.
