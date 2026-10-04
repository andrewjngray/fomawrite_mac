# Cycle 121 — Title Bar fade choices

Build 0.3.0-dev17 / macOS 0.3.0 (121).

View → Title Bar offers Fade In/Out and Always Show. Both use the same persisted setting as Auto-Hide Document Bars, so the menu reflects actual behavior. Fade keeps an edge-revealed bar visible until editing/scrolling; Always Show keeps both document bars visible. The legacy title-opacity gate no longer contradicts the menu. Toolbar content preferences and pane geometry stay unchanged.

**183 regression tests pass, 0 failed, 0 skipped**, including setup/cleanup. The persistence check verifies the saved setting after reopening. Dev, demo and Applications each pass **149 native executable checks** with matching UI/version, strict signatures and zero QML warnings. All three copies are refreshed. [Verified build identities](verified-builds.json). Logs and synthetic captures remain local. Targets: `dist/Fomawrite Dev.app`, `dist/Fomawrite.app`, `/Applications/Fomawrite.app`. No public release or notarization. Prior physical input/accessibility and parser/export limitations remain open.

Verified source commit: `151bec95c869f934c99c56952237eb829cfc1efc`.
