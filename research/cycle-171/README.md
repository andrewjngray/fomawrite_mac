# Cycle 171 — Wider sidebars in Mac PDF Split

Build 171 (`0.3.0-dev66`).

- PDF Split on Mac can compact Source to 360 px. The PDF still has a 320 px minimum. The Library and file-list caps on Mac are now 400 and 520 px when space allows. The same calculation is used by the layout, divider drag and double-click balance.
- At a 1,295 px window width with both sidebars visible and review hidden, the combined sidebar budget grows from 492 to 612 px. With the other sidebar at its minimum, the Library can reach 380 px or the file list can reach 428 px. The two sidebars share the budget.
- The existing per-window settings store sidebar widths within the expanded range.

## Verification

- The 1,295 px synthetic PDF test failed on 170 because the Library could not reach 350 px. On 171, an ordinary pointer drag widened it beyond 350 px and the file list beyond 420 px in separate trials, with Source Find open and the window width fixed.
- The focused suite passed eight checks, including the earlier constrained divider case, narrow PDF controls, Source Find alignment, double-click balance, and reference workspace composition.
- Native Cocoa passed both drag tests, the Split Source Find test, and the narrow PDF controls test. The stable Dev bundle was prepared and passed strict signature verification.
- The full Mac suite passed all 431 checks. The signed Dev app passed the native document-view harness. [Native report](native-bundle/report.json)
- [Hands-on checklist](../usability/cycle-171.md).
