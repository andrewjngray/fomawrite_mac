# Cycle 31 — Filter field polish

Fix: replace Material floating-label field with explicitly styled Basic TextField. Bound layout width to the pane's inner width and remove content-dependent implicit minimum size. Radius is half the field height.

Build passed; 54 tests passed (`logs/`). Geometry regression covers focused field at 1100, 900 and 720 window widths, then long input. Native focused empty placeholder and typed input inspected; screenshot shows sample-only library with both rounded ends visible. Native drag attempts did not change geometry; do not count as a successful resize test. Dark appearance not visually verified.

The user's dirty document was saved through the app before normal quit. QA sample state was also saved before closing. No private document text or screenshots retained here.

Runnable: dist/Omawrite Dev.app and dist/Omawrite.app (ad-hoc signed).
