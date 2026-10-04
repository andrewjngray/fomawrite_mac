# Cycle 119 — editor, publishing and quiet document bars

Build 0.3.0-dev15 / macOS 0.3.0 (119).

The left pane owns Source / Visual Edit and writing appearance. The right pane is read-only publishing output, with Web / PDF and output style. Single / Split stays at the far right. Single keeps the chosen editor; Split adds publishing output. Preview-only remains a separate reading command.

Web loads the same HTML, embedded local images and CSS produced by export. PDF displays the actual exported pages, including paper size, margins and page furniture. Both use bounded private temporary files, deleted with the document backend. Viewing and zooming do not edit the Markdown or alter output styling. Visual Edit retains the existing supported Markdown projection and Source fallback for unsupported structures.

Document bars hide while typing or scrolling. Moving to the upper or lower edge reveals that bar. The reserved geometry stays fixed, avoiding reading-position jumps. Keyboard access and open menus retain controls; View → Auto-Hide Document Bars switches the behavior off. Library and Organizer remain visible and reachable.

## Verification

Dev, demo and Applications are refreshed to build119. Each passes **149 actual-executable checks**, matching embedded UI/version, valid strict signatures and **zero QML warnings**. Demo and Applications have identical executables. [Verified identities](verified-builds.json).

The full regression run passed 181 cases with one outdated UI-label expectation (Preview · Helvetica versus the new Output · Helvetica). That expectation was corrected. The final focused rerun passed **8 cases, 0 failures, 0 skips**, covering the corrected label, native Web command, publishing/export parity, temporary-file lifecycle and auto-hide access. Counts include setup/cleanup. No known failure remains; this is not a claim of a subsequent clean full-suite rerun.

Packaging now repairs shared framework/dylib references for the nested WebEngine helper, removes host Homebrew search paths and rejects missing bundled dependencies before signing. The packaged app's Web and PDF renderers were exercised natively after that repair.

Production source commit: `19b3f89127709205f740793a37fb24e45b9b667d`. Logs and synthetic captures remain local. Physical IME/VoiceOver, multi-display behavior and complete parser/export parity remain outside the automated gate.

Review copies: `dist/Fomawrite Dev.app`, `dist/Fomawrite.app`, `/Applications/Fomawrite.app`. The previous Applications copy is retained in a local temporary backup. These are local ad-hoc builds; no public release or notarization was performed.

## Review

Switch Source / Visual Edit on the left and Web / PDF on the right; change output style and page setup. Compare PDF preview to an export. Scroll and type, reveal each bar at its edge, resize the divider, and use Single / Split. Check that each control belongs to the pane it changes and your reading position remains steady.
