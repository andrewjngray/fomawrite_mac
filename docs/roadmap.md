# Omawrite Mac: personal Markdown workspace

**Current status — 24 September 2026:** Cycle 59 is an implemented-subset integrated checkpoint at code commit `5f8d64b` on macOS 27.0. Subsequent Cycles 54b–54c add independent opt-in Smart Quotes and bounded Smart Dashes; Cycle 50b adds the captured Transformations labels and a directly observed Capitalize subset. `./bin/build` and all **102 tests pass**. Both ordinary and Dev bundles were refreshed, locally ad-hoc signed and strict-verified. Dev verified Capitalize and Undo in a disposable draft, discarded it and returned to a clean saved sample; the ordinary app reopened a clean saved document. Native evidence accumulated across Cycles 48–58 covers all ten top-level menu families and representative workflows, including Fillers, offline Help, fragment navigation and Window Center. It is not a complete saved/untitled/dirty state matrix, exact iA parity or final closeout. VoiceOver, multi-display, dark/narrow and the remaining writing substitutions/correction, style/syntax groups, post-setup authorship workflows, AppKit actions, parser/output/cross-app gaps and distinct Title Case semantics remain open. No screenshot file was persisted. The local artifacts are ad-hoc signed; the GitHub RC1 remains the older Cycle 42 release. See the [acceptance ledger](release-acceptance.md), [Cycle 59 record](../research/cycle-59/README.md) and [Cycle 50 record](../research/cycle-50/README.md).


Owner: Andrew Gray. Working name: Omawrite Mac. Starting point: the MIT-licensed omacom/omawrite project; preserve its history and attribution.

## Product direction

Make reading and writing local Markdown pleasant enough to use every day. Take the calm typography and focused writing experience Andrew likes in iA Writer, and the direct Markdown editing experience he likes in Typora. Add capabilities when they address a real workflow. These are design goals, not a claim of feature parity or shared proprietary code.

## Current implementation sequence

Andrew's current request is to close the observed iA Writer menu gaps one cycle at a time. [Cycles 48–59](menu-closeout-plan.md) supersede the older 14–28 sequence for future work. The older plan and checkpoints remain as implementation history.

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

Cycle 33 themes is complete; Cycle 34a adds recovery-baseline protection and provisional multi-document Quit approval. Nine cycles remain including the unfinished portion of Cycle 34; use the remaining-cycles plan for current status.


### Cycles 34c–38 checkpoint

Added opt-in previous-save history, authorship-aware Markdown clipboard and metadata export, richer footnotes/local content blocks, persisted paginated output with portable HTML images, and organizer saved queries/browsable tags. Build and 66 tests pass; representative native sample checks and rendered PDF review completed. Four untouched feature/release cycles remain (39–42), plus documented acceptance gaps in 34–38. See [the remaining plan](remaining-cycles.md).
