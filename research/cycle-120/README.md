# Cycle 120 — persistent menu reveal

Build 0.3.0-dev16 / macOS 0.3.0 (120).

A document bar revealed at its edge stays visible after the pointer leaves. Source/Visual Edit input or scrolling clears the reveals; ordinary publishing clicks do not. Top and bottom remain independently revealable, with fixed document geometry, menu/keyboard access and the always-visible View preference preserved.

Studio now has a soft grey editor, including Source, Visual Edit and its header/footer. The publishing surround stays warm cream; Web/PDF pages retain their exported colors. Other themes retain their established backgrounds.

**183 regression tests pass, 0 failed, 0 skipped** (including setup/cleanup). Dev, demo and Applications each pass **149 native executable checks**, matching embedded UI/version, strict signatures and **zero QML warnings**. The native checks include repeated edge departure, source typing, publishing clicks/wheels and fixed geometry. Focused regressions also cover copy/select-all versus paste/Undo and Visual Edit typing. [Verified build identities](verified-builds.json). Synthetic captures and full logs stay local.

Refreshed runnable targets: `dist/Fomawrite Dev.app`, `dist/Fomawrite.app`, `/Applications/Fomawrite.app`. No public release or notarization.

Physical IME/VoiceOver, multiple displays and broader parser/export parity remain outside this targeted refinement.
