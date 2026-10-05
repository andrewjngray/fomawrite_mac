# Cycle130 — compact Output Style menu

Build **0.3.0-dev26 / macOS 0.3.0 (130)**.

The footer now says **Output Style**, with the active style in its hover hint and menu checkmark. This distinguishes appearance from the adjacent Web/PDF format choice. A **Custom Themes ›** submenu above the basic presets holds all CSS themes, including bundled Claude Like and folder/imported themes. Both footer and View menus use this grouping. Hover/click opens the submenu; long catalogs scroll within the window. The parent stays compact as themes are added. Selection-check restoration traverses both menu levels, including rejected selections.

[Menu instructions](../../docs/output-style-menu.md) explain each preset, Custom Settings, JSON loading, CSS import, folder discovery, reload and the Typora gallery link. Existing actions retain their behavior; rendering and watcher code are unchanged.

## Verification

- Full regression suite: **204 passed, 0 failed, 0 skipped**. The first run caught one remaining legacy assertion expecting the old dynamic label; it was updated to assert Output Style plus the active-style hint. Its focused draft/Undo recheck passed.
- Native theme checks passed actual hover, CSS application, repeated/rejected choices, one authoritative checkmark, JSON/import/settings actions and menu closure. Adding 27 synthetic folder themes changed only the submenu; the parent count and dimensions stayed fixed. A visible cascading menu capture was inspected.
- The first broad native run had one unrelated preview readiness failure while its Cocoa window was unexposed; all menu checks passed. The subsequent focused preview run passed all four groups/eight painted-pane observations with zero QML warnings. This is not a claim that the first broad run passed.
- Final app identities/signatures and verification counts: [manifest](verified-builds.json). Raw synthetic captures/logs remain local. No private documents are committed.

Physical mouse/keyboard use, VoiceOver and multi-display behavior remain user acceptance work. [Optional review](../usability/cycle-130.md).
