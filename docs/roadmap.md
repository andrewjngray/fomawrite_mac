# Fomawrite: local Markdown workspace

> The current build, test count and open items are in [STATUS.md](../STATUS.md); every cycle is listed in [CHANGELOG.md](../CHANGELOG.md). The checkpoints below are kept as the history of goals and plans.

**Cycle 60 identity update:** The current product and app bundles are Fomawrite and Fomawrite Dev. The GitHub repository is `andrewjngray/fomawrite_mac`; upstream Omawrite history and licences remain intact. The new macOS identities migrate prior preferences and copy matching local workspace/recovery state on first launch. See [rename notes](archive/product-rename.md).

**Pre-rename menu status — 24 September 2026:** Cycle 59 is an implemented-subset integrated checkpoint at code commit `5f8d64b` on macOS 27.0. Subsequent Cycles 54b–54c add independent opt-in Smart Quotes and bounded Smart Dashes; Cycles 50b–50c add the captured Transformations labels, a directly observed Capitalize subset, and distinct Make Title Case behavior on two iA samples. `./bin/build` and all **102 tests pass**. Both ordinary and Dev bundles were refreshed, locally ad-hoc signed and strict-verified. Dev verified Capitalize and Make Title Case with Undo in disposable drafts, discarded them and returned to a clean saved sample; the ordinary app reopened a clean saved document. Native evidence accumulated across Cycles 48–58 covers all ten top-level menu families and representative workflows, including Fillers, offline Help, fragment navigation and Window Center. It is not a complete saved/untitled/dirty state matrix, exact iA parity or final closeout. VoiceOver, multi-display, dark/narrow and the remaining writing substitutions/correction, style/syntax groups, post-setup authorship workflows, AppKit actions, parser/output/cross-app gaps and unobserved Title Case locale/style semantics remain open. No screenshot file was persisted. The local artifacts are ad-hoc signed; the GitHub RC1 remains the older Cycle 42 release. See the [acceptance ledger](release-acceptance.md), [Cycle 59 record](../research/cycle-59/README.md) and [Cycle 50 record](../research/cycle-50/README.md).


Owner: Andrew Gray. Product name: Fomawrite (renamed in Cycle 60). Starting point: the MIT-licensed omacom/omawrite project; preserve its history and attribution.

## Product direction

Make reading and writing local Markdown pleasant enough to use every day. Take the calm typography and focused writing experience Andrew likes in iA Writer, and the direct Markdown editing experience he likes in Typora. Add capabilities when they address a real workflow. These are design goals, not a claim of feature parity or shared proprietary code.

## Current implementation sequence

**Cycle118 — independent editing and layout:** Andrew approved separate Source / Visual Edit and Single / Split groups after the build117 footer review. Single keeps the chosen editor; Split places that editor beside read-only output Preview. Preview-only is a separate reading action. This replaces the mixed Source / Split / Full control and the separate Visual Edit toggle. Both new capsules keep fixed positions through mode changes, and the editor appearance and output template remain independent. Hidden-Source clipboard/completion guards, explicit Find transitions and version1/version2 checkpoint migration protect the existing writing state. Review build `0.3.0-dev14`, macOS `0.3.0 (118)`; implementation and 178 passing regressions are complete. All three local copies are refreshed and each passes 147 actual-executable checks, matching resources/version and strict signatures with no QML warnings. All 12 focused Cocoa checks pass, with no failures or skips. [Cycle118 status](../research/cycle-118/README.md).

Cycle 61 completed the local checkout path rename. The [next development phases](archive/next-development-phases.md) propose Cycles 62–69 for direct visual editing, compact writing controls and a richer export/style workflow, using Andrew's Ulysses screenshots and Typora as interaction references. The older [iA menu closeout plan](archive/menu-closeout-plan.md) and its still-open [acceptance ledger](release-acceptance.md) remain separate work; neither is silently marked complete by the new plan.

After Cycle 84, the [editor surface redesign](editor-surface-redesign.md) refocuses the next proposed cycles on the everyday writing view: optional Studio column tones and Editorial/Book/Manuscript appearances first, then richer safe in-place editing. It distinguishes a visual styling pass from the larger Markdown round-trip work required for editable image and link objects.

The [17 September staged development plan](archive/development-plan.md) turns the [iA menu parity log](archive/ia-menu-parity-2026-09-17.md) into proposed Cycles 14–28: menu access, safe file/editing commands, navigation, richer documents/output, native lifecycle, then advanced writing tools. Codex remains the visual reference. Full functional parity includes explicit validation of hidden submenu behavior and is not claimed by matching labels.

The [3 October workspace UI plan](workspace-ui-redesign-plan.md) established Cycles 90–97, followed by bounded editing/navigation in 98–100 and pane/template access in 102. Andrew’s build 102 comparison showed that the native presentation still diverged from the approved concept. Cycle 103 makes visual fidelity the immediate priority: shared geometry, reference typography/icons/cards, first-launch adoption and native screenshot comparison. Cycle 104 follows Andrew’s toolbar feedback with an explicit folder heading, typographic formatting controls and quick text sizing. Cycle 105 refines the actual toolbar against Andrew’s Ulysses reference. Cycle 106 unifies the workspace footers with the header controls, aligns their baselines and verifies repeated menu interactions. Cycle 107 provides a shared, stationary document footer. Cycle 108 fixes responsive view-state mismatches and checks the actual installed bundle, with explicit version/path identification. Cycles109–111 add safe simple-list splitting, searchable keyboard outline navigation and responsive Find/Replace, plus a Redo-history repair. Cycle112 adds independent Source/Preview zoom, optional linking, retained reading anchors and divider balance. Cycles113–116 add formatting focus ownership, safe inline-link editing, wider actual-executable acceptance and bounded visual table keyboard navigation. Cycle117 follows Andrew’s next review with pane-owned footer menus, rendered-header document identity and compact filename/date/excerpt rows. Previous verified review build: `0.3.0-dev13`, macOS `0.3.0 (117)`, with 176 passing regressions, 12 passing focused native checks and passing executable gates in all three local app copies. At that checkpoint, all three local copies were refreshed with matching resource/version identities and valid strict signatures; see the [Cycle117 handoff](../research/cycle-117/README.md). The wider native-window fixture remains activation-blocked after 79 assertions; later OS keyboard/full-screen/print/share acceptance is open. Earlier historical results above remain unchanged.

## Follow-through after the workspace candidate

| Cycle | Implemented bounded scope | Evidence and limits |
| --- | --- | --- |
| 98 | F6/Shift+F6 pane navigation; compact Drawer focus and Escape restoration; hidden-pane focus rescue; inactive toolbar focus treatment. | Integrated native fixture passes 111 assertions across Cycles 98/100, including full screen, tab detach/restoration, independent pane checkpoints and exact draft/Undo preservation. Physical VoiceOver, display scaling and arbitrary-workspace relaunch remain acceptance checks. |
| 99 | Safe simple table-cell and inline-code edits; local image thumbnails with alt-text editing; end-of-item list Return, unchecked task continuation and empty-item exit; grapheme-safe replacements, empty-document input and caret visibility. | Source edits are mapped and validated without whole-document conversion. All 134 integrated tests and focused native Visual Edit checks pass. Arbitrary multiline paste, splitting within a list item, complex/nested structures, spatial image placement and general WYSIWYG retain Source fallback. |
| 100 | Transfer heading fragments to an already-open window/tab; resolve against its live draft; display missing-heading feedback; reject stale preview-refresh callbacks. | Native cross-window Unicode heading, dirty-buffer preservation, source Cancel and Undo pass. Broader parser/output and cross-application clipboard gaps remain open. |

Cycle 102 keeps hidden navigation controls reachable, improves the View menu and makes the Preview template label interactive. Cycles103–104 address the approved visual reference and toolbar feedback. Following independent pane zoom in Cycle112, the authorized Cycles113–116 implement the close-out priorities: Source formatting ownership, a validated link-edit session, wider daily-writing acceptance and Tab/Shift-Tab through supported table cells. Their bounded integrated gate and all three app-bundle checks pass. Physical input/display/accessibility and the activation-blocked native scenarios remain separate acceptance work. See the [Cycle116 handoff](../research/cycle-116/README.md). Cycle117 verified footer ownership, filename/Edited clarity and compact library navigation; its 176 regressions, 12 focused native checks and all three local app executable gates pass. Cycle118 addresses the resulting editing/layout confusion before another structural increment. Then use Andrew’s normal writing and physical input/display feedback to select a small structural increment: clearer image-object placement and keyboard access, table row/column restructuring, or richer safe lists. Typewriter scrolling is explicitly Source-only. Each increment needs exact-source, Unicode, Undo/Redo, narrow/dark and native checks before expansion.

**Cycle 101 remains an optional distribution track**, only if Andrew wants it: Developer ID signing/notarization, another-machine installation and release packaging. Local build/installation work does not authorize a public release or close this gate. The [acceptance ledger](release-acceptance.md) retains earlier workflow/parser/export and hardware gaps; these cycles do not imply complete competitor parity.

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

Saved-file workspace/tab restoration and same-installation launch forwarding are implemented in Cycle 32. See [the remaining ten-cycle plan](archive/remaining-cycles.md), including Cycle 33 whole-workspace themes. The remaining lifecycle stress matrix is assigned to Cycle 34; full parity is not claimed.

Cycle 33 themes is complete; Cycle 34a adds recovery-baseline protection and provisional multi-document Quit approval. Nine cycles remain including the unfinished portion of Cycle 34; use the remaining-cycles plan for current status.


### Cycles 34c–38 checkpoint

Added opt-in previous-save history, authorship-aware Markdown clipboard and metadata export, richer footnotes/local content blocks, persisted paginated output with portable HTML images, and organizer saved queries/browsable tags. Build and 66 tests pass; representative native sample checks and rendered PDF review completed. Four untouched feature/release cycles remain (39–42), plus documented acceptance gaps in 34–38. See [the remaining plan](archive/remaining-cycles.md).
