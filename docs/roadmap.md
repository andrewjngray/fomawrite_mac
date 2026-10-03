# Fomawrite: local Markdown workspace

**Cycle 60 identity update:** The current product and app bundles are Fomawrite and Fomawrite Dev. The GitHub repository is `andrewjngray/fomawrite_mac`; upstream Omawrite history and licences remain intact. The new macOS identities migrate prior preferences and copy matching local workspace/recovery state on first launch. See [rename notes](product-rename.md).

**Pre-rename menu status — 24 September 2026:** Cycle 59 is an implemented-subset integrated checkpoint at code commit `5f8d64b` on macOS 27.0. Subsequent Cycles 54b–54c add independent opt-in Smart Quotes and bounded Smart Dashes; Cycles 50b–50c add the captured Transformations labels, a directly observed Capitalize subset, and distinct Make Title Case behavior on two iA samples. `./bin/build` and all **102 tests pass**. Both ordinary and Dev bundles were refreshed, locally ad-hoc signed and strict-verified. Dev verified Capitalize and Make Title Case with Undo in disposable drafts, discarded them and returned to a clean saved sample; the ordinary app reopened a clean saved document. Native evidence accumulated across Cycles 48–58 covers all ten top-level menu families and representative workflows, including Fillers, offline Help, fragment navigation and Window Center. It is not a complete saved/untitled/dirty state matrix, exact iA parity or final closeout. VoiceOver, multi-display, dark/narrow and the remaining writing substitutions/correction, style/syntax groups, post-setup authorship workflows, AppKit actions, parser/output/cross-app gaps and unobserved Title Case locale/style semantics remain open. No screenshot file was persisted. The local artifacts are ad-hoc signed; the GitHub RC1 remains the older Cycle 42 release. See the [acceptance ledger](release-acceptance.md), [Cycle 59 record](../research/cycle-59/README.md) and [Cycle 50 record](../research/cycle-50/README.md).


Owner: Andrew Gray. Product name: Fomawrite (renamed in Cycle 60). Starting point: the MIT-licensed omacom/omawrite project; preserve its history and attribution.

## Product direction

Make reading and writing local Markdown pleasant enough to use every day. Take the calm typography and focused writing experience Andrew likes in iA Writer, and the direct Markdown editing experience he likes in Typora. Add capabilities when they address a real workflow. These are design goals, not a claim of feature parity or shared proprietary code.

## Current implementation sequence

Cycle 61 completed the local checkout path rename. The [next development phases](next-development-phases.md) propose Cycles 62–69 for direct visual editing, compact writing controls and a richer export/style workflow, using Andrew's Ulysses screenshots and Typora as interaction references. The older [iA menu closeout plan](menu-closeout-plan.md) and its still-open [acceptance ledger](release-acceptance.md) remain separate work; neither is silently marked complete by the new plan.

After Cycle 84, the [editor surface redesign](editor-surface-redesign.md) refocuses the next proposed cycles on the everyday writing view: optional Studio column tones and Editorial/Book/Manuscript appearances first, then richer safe in-place editing. It distinguishes a visual styling pass from the larger Markdown round-trip work required for editable image and link objects.

The [17 September staged development plan](development-plan.md) turns the [iA menu parity log](ia-menu-parity-2026-09-17.md) into proposed Cycles 14–28: menu access, safe file/editing commands, navigation, richer documents/output, native lifecycle, then advanced writing tools. Codex remains the visual reference. Full functional parity includes explicit validation of hidden submenu behavior and is not claimed by matching labels.

The [3 October workspace UI plan](workspace-ui-redesign-plan.md) is approved and Cycles 90–96 are integrated in the Cycle 97 candidate: `0.3.0-dev1`, macOS bundle `0.3.0` build `97`. It adds column-owned headers, shared controls and palette roles, independent responsive pane state, per-window persistence, writing presentation and navigation/supporting-surface refinements. Build, 122 regression tests and recorded native checks pass; Andrew's broad acceptance, Cycle 88's outstanding review and earlier functional gaps remain open. See the [candidate record](../research/cycle-97/README.md).

## Next bounded work after the workspace candidate

| Cycle | Recommended scope | Gate |
| --- | --- | --- |
| 98 | Incorporate Andrew's writing feedback; review keyboard/VoiceOver, inactive/dark states, display scaling, full screen, native tabs and independent-window restoration. | Record the acceptance matrix and resolve specific regressions before expanding editing scope. |
| 99 | Extend safe visual projection for blocks, images, lists and tables in small increments. | Exact Markdown round trips, Unicode/paste/Undo fixtures and explicit Source fallback for unsupported edits. General WYSIWYG support is not presumed. |
| 100 | Select the most useful workflow, parser or export gaps from the [acceptance ledger](release-acceptance.md) and inventories. | Define a bounded feature and validate its real workflow; matching menu labels is insufficient. |
| 101 | Distribution, if Andrew wants it: signing/notarization, another-machine installation and release packaging. | Accepted product build and verified public-distribution artifacts. Current local signing does not close this gate. |

These are recommendations for separate scopes, not a promise that the broader parity backlog will be complete in four cycles.

## Milestones

1. **Mac foundation:** build an app bundle; native menus and file dialogs; Command shortcuts; Finder document opening; system appearance; existing save, recovery and external-change behaviour. Add a reproducible development workflow.
2. **Daily writing:** folder navigation and quick switching, adjustable typography, focus mode, recent documents. Choose the first addition through actual use.
3. **Richer documents:** evaluate tables, local images, task lists, source/reading modes and export against real example documents. Establish Markdown round-trip fixtures before changing parsing or rendering.
4. **Personal workflows:** document templates, search across folders, links between notes and user-defined commands. Add integrations only when needed.
5. **Distribution:** final short-connector app icon (Cycle 72) and naming, accessibility audit, multi-document lifecycle polish, signing/notarisation and automated releases if Andrew wants other people to use it.

## Learning loop

Choose one concrete inconvenience. Describe expected behaviour with an example. Read the small part of the code involved, implement, verify, and use it. Commit each useful increment. Keep short decision notes that explain why the code changed.

## Architectural choices

Retain Qt Quick/QML and C++ to reuse the upstream editor and stay close to upstream. QML controls the interface; C++ owns document I/O, recovery and Markdown formatting. macOS has a separate appearance implementation, with Linux's portal implementation retained. Plain UTF-8 Markdown remains the storage format.

The closeout pass consolidates windows in one process with per-window backends/recovery and sequential guarded quitting. Native tab controls are available. Cross-launch coordination and persisted window/tab restoration remain in the closeout audit.

## Current follow-through — Cycle 32 onward

Saved-file workspace/tab restoration and same-installation launch forwarding are implemented in Cycle 32. See [the remaining ten-cycle plan](remaining-cycles.md), including Cycle 33 whole-workspace themes. The remaining lifecycle stress matrix is assigned to Cycle 34; full parity is not claimed.

Cycle 33 themes is complete; Cycle 34a adds recovery-baseline protection and provisional multi-document Quit approval. Nine cycles remain including the unfinished portion of Cycle 34; use the remaining-cycles plan for current status.


### Cycles 34c–38 checkpoint

Added opt-in previous-save history, authorship-aware Markdown clipboard and metadata export, richer footnotes/local content blocks, persisted paginated output with portable HTML images, and organizer saved queries/browsable tags. Build and 66 tests pass; representative native sample checks and rendered PDF review completed. Four untouched feature/release cycles remain (39–42), plus documented acceptance gaps in 34–38. See [the remaining plan](remaining-cycles.md).
