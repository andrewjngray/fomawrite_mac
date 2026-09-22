# Omawrite Mac: personal Markdown workspace

Owner: Andrew Gray. Working name: Omawrite Mac. Starting point: the MIT-licensed omacom/omawrite project; preserve its history and attribution.

## Product direction

Make reading and writing local Markdown pleasant enough to use every day. Take the calm typography and focused writing experience Andrew likes in iA Writer, and the direct Markdown editing experience he likes in Typora. Add capabilities when they address a real workflow. These are design goals, not a claim of feature parity or shared proprietary code.

## Current implementation sequence

The [17 September staged development plan](development-plan.md) turns the [iA menu parity log](ia-menu-parity-2026-09-17.md) into proposed Cycles 14–28: menu access, safe file/editing commands, navigation, richer documents/output, native lifecycle, then advanced writing tools. Codex remains the visual reference. Full functional parity includes explicit validation of hidden submenu behavior and is not claimed by matching labels.

## Milestones

1. **Mac foundation:** build an app bundle; native menus and file dialogs; Command shortcuts; Finder document opening; system appearance; existing save, recovery and external-change behaviour. Add a reproducible development workflow.
2. **Daily writing:** folder navigation and quick switching, adjustable typography, focus mode, recent documents. Choose the first addition through actual use.
3. **Richer documents:** evaluate tables, local images, task lists, source/reading modes and export against real example documents. Establish Markdown round-trip fixtures before changing parsing or rendering.
4. **Personal workflows:** document templates, search across folders, links between notes and user-defined commands. Add integrations only when needed.
5. **Distribution:** icon and naming, accessibility audit, multi-document lifecycle polish, signing/notarisation and automated releases if Andrew wants other people to use it.

## Learning loop

Choose one concrete inconvenience. Describe expected behaviour with an example. Read the small part of the code involved, implement, verify, and use it. Commit each useful increment. Keep short decision notes that explain why the code changed.

## Architectural choices

Retain Qt Quick/QML and C++ to reuse Omawrite's working editor and stay close to upstream. QML controls the interface; C++ owns document I/O, recovery and Markdown formatting. macOS has a separate appearance implementation, with Linux's portal implementation retained. Plain UTF-8 Markdown remains the storage format.

The closeout pass consolidates windows in one process with per-window backends/recovery and sequential guarded quitting. Native tab controls are available. Cross-launch coordination and persisted window/tab restoration remain in the closeout audit.

## Current follow-through — Cycle 32 onward

Saved-file workspace/tab restoration and same-installation launch forwarding are implemented in Cycle 32. See [the remaining ten-cycle plan](remaining-cycles.md), including Cycle 33 whole-workspace themes. The remaining lifecycle stress matrix is assigned to Cycle 34; full parity is not claimed.
