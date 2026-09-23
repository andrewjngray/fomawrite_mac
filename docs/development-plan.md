# Staged development plan — iA functional parity

Proposed 17 September 2026 from Andrew’s seven menu screenshots and current source. The command-level acceptance log is [ia-menu-parity-2026-09-17.md](ia-menu-parity-2026-09-17.md). The standing functional backlog remains [ia-writer-inventory.md](ia-writer-inventory.md). Codex remains the visual reference.

This supersedes the old speculative cycle numbers in the inventory and “Upcoming cycles” table. Cycles 0–13 are historical; 14 onward below are proposed. Larger stages may be split into lettered increments (for example 16a/16b), each independently runnable. These are scope and dependency estimates, not delivery-date promises.

**Current progress (21 September, closeout implementation pass):** Working slices now exist across 20b–27; Cycle 28 integration/audit is underway. Final build and 48 tests pass; native palette/focus, tag query, dirty-window quit, native tabs, saved-version restore/undo and manual annotations verified. Both bundles refreshed. **The original plan is not fully complete.** Output templates, advanced services/authorship and other acceptance gaps remain in the [closeout audit](closeout-audit.md). The earlier “15 increments remaining” count described untouched stages; do not interpret this implementation pass as zero remaining work.

## Delivery order

| Cycle | Runnable increment | Included parity-log IDs | Completion evidence / optional exercise |
|---|---|---|---|
| 13 closeout | Finish visual verification of existing monochrome interface; safely refresh stable Dev app after preserving unsaved work | Existing implementation | Compare normal, narrow and dark layouts; record sample-only screenshot and remaining issues |
| 14 — menu foundation | Shared actions between native menus, toolbars and command registry. View pane/bar/sort/text-size/layout controls; explicit preview reload; existing paragraph/typewriter focus and strike/inline code actions | V01–V08 (existing subsets), V13, C01/C03/C04, M06/M08 | Checkmarks, enabled states and labels stay synchronized; shortcut collision audit; keyboard menu operation and restart persistence. Exercise: configure workspace entirely from menus |
| 15 — everyday file access | File New variants, recent files, Finder/library reveal; Go locations/add-location and recents | F01–F07, F12–F13, G07–G08 (recent-list subset) | Untitled/dirty/saved files; canceled dialogs; missing recents; outside-root/filtered reveal; no duplicate creation. Exercise: create, save, reopen and reveal a disposable note |
| 16a — duplicate and rename | Duplicate first; rename after path-state handling is covered | F08–F09 | Unsaved-buffer policy, Unicode names, existing destination, permission error and canceled action; active path, watcher, recovery, favorites/recents remain correct |
| 16b — move | Move within/across folders and volumes, with explicit failure behavior | F10 | Failed copy must not delete original; verify contents before removal where copying is required; external changes, links and cross-volume failures. Exercise: move a sample and reopen it through Recents |
| 17 — search/edit menus | Native Find/Replace/Next/Previous, selection search where confirmed; Delete and editing action states | E01–E02, E09–E10 | Empty/no-match searches, Unicode, search wrap, selection and one-step Replace All undo. Exercise: replace three matches, then undo once |
| 18a — block formatting | Heading/body, lists/tasks, blockquote, indent/outdent and line moves after inspecting Structure | M01–M04 | C++ formatting transformations; multiline/reversed selections, existing markers, nested lists, fences and one-step undo |
| 18b — inline and structural insertion | Consistent bold/italic/strike/code/link semantics; code block and horizontal rule | M05–M10 (excluding highlight), M18 | Escaped delimiters, embedded backticks, blank selections, cursor placement and Markdown round trip. Exercise: format a sample without typing syntax |
| 19a — small editing tools | Date, basic table insertion, clear supported styles, Unicode case transformations | M15–M16/M20, E13 | Preserve links/code; define unsupported Clear Styles cases; source and undo checks |
| 19b — rich clipboard | Copy formatted/HTML/Markdown and confirmed non-authorship paste conversions | E03–E06 (ordinary clipboard only) | Verify clipboard MIME types in another native app; selection-only output; predictable plain-text fallback; source untouched on copy |
| 20a — navigation | Separate document/library histories, enclosing folder, editor Open Link and recursive filename quick-open | G01–G05 (filename scope) | Dirty navigation cancellation does not advance history; cursor restore; missing files, symlinks, cancellation and large-folder responsiveness |
| 20b — command palette and sentence focus | Search shared actions; sentence focus with defined language/punctuation scope | G06, C01–C02 | Keyboard-only palette, disabled actions, sentence abbreviations/punctuation/code, stable scrolling |
| 21a — indexed library | Bounded background filename/content search; complete confirmed tree/list/date/extension options | G05, V04 | No UI stalls, canceled/stale results suppressed, filesystem changes reflected, explicit index scope and exclusions |
| 21b — tags and smart folders | Hashtag insertion/index/navigation; saved query smart folders and recent query | G08–G10, M14 | Tags inside code excluded; query persistence, rename/move/delete updates, large-folder limits. Exercise: save a query and see it update after editing a sample |
| 22a — preview navigation | Heading anchors, document TOC, synchronized source/preview scrolling | M17, V08, G04 | Repeated headings, long documents, images/tables, reverse scrolling and no feedback loop |
| 22b — Markdown extensions | Wikilinks, footnotes, highlight, content blocks, each as a separate tested increment | M07/M11–M13 | Inspect reference syntax first; relative paths, missing/ambiguous targets, inclusion cycles; source stays portable and explicit |
| 23a — export/print | Confirm formats, then HTML and PDF export; rendered/plain-text print, page setup and page break | F15–F17, M19, V08 | Local images/relative links, Unicode, tables across pages, headings, margins, cancellation and source unchanged; visually inspect exported pages |
| 23b — templates | Independently designed preview/output templates, custom-template workflow and confirmed fit modes | V09, V08 | Consistent preview/export, offline assets, paper sizes, long content and readable dark preview; no copying proprietary template assets |
| 24 — macOS presentation | Native share, confirmed title/toolbar modes/statistics, native Window actions supported by OS | F14, V10–V11, W01/W03–W07, V13 | Native sheet cancellation, keyboard access, fullscreen, narrow/dark/scaled displays; connected-display actions only when applicable |
| 25a — document ownership | Replace per-window detached processes with coordinated document/window lifecycle | W02/W08/W11; earlier Close All/Quit variants | Multiple dirty windows, canceled quit, save failure, same file twice, crash recovery and window reopen; preserve per-document recovery isolation |
| 25b — native tabs | Tab bar/overview, previous/next, detach and merge | V12, W09–W10 | Moving tabs never drops unsaved content; close/cancel and restore across restart |
| 25c — autosave and versions | Inspect Revert To; design native versions, autosave/revert and explicit-save coexistence | F07/F11 | Version restore cancel/accept, external writers, disk-full/save failures and crash recovery; recovery snapshots remain distinct from version history |
| 26a — native writing services | Spelling/grammar, substitutions, speech and available dictation/emoji/AutoFill; inspect completions | E11–E12/E14–E17, V06 | Check actual Qt/macOS support first; exclude code where appropriate; language support, undo and accessibility |
| 26b — linguistic aids | Sentence analysis foundations, parts-of-speech highlighting and style rules/custom patterns | C05–C06 | Define supported languages and rule coverage; sample corpus, false positives, opt-out and responsiveness. Do not claim exact iA linguistic results |
| 27 — authorship | Research data model first, then tracked author ranges, paste-edits/mark-as and Human/AI/Reference display | E06 (authorship)/E07–E08, C07; earlier Authors menu | Undo/redo, edits that shift ranges, cross-file clipboard, export/reopen and attribution loss behavior; explicit portable sidecar/metadata decision |
| 28 — parity and release review | Close all inspected menu gaps; complete app/Authors/Help/settings/context-menu audit; own help/release notes and release readiness | All remaining IDs | Each row backed by behavior and evidence; full accessibility/keyboard/dark audit; signing/notarization if distribution is wanted |

## Milestones and dependencies

1. **Everyday menu coverage (14–19):** existing functionality becomes discoverable; safe file operations and the main editing commands work. This is useful daily-writing coverage, not full parity.
2. **Navigation and richer documents (20–23):** search, tags, smart folders, links and repeatable output. Build the shared action registry in 14 before the command palette; indexing in 21 before query-based folders. Stabilize Markdown/anchor behavior before output templates.
3. **Native Mac lifecycle (24–25):** system actions and coordinated windows/tabs/versions. Document ownership is a prerequisite for tab transfers and application-wide quitting.
4. **Advanced writing and closure (26–28):** native language services, linguistic tools, authorship, then remaining reference surfaces. Linguistic and provenance features require explicit behavior design; menu names alone do not define them.

The order prioritizes daily utility and confines the largest data-safety changes to their own stages. No stage requires a speculative rewrite of the entire app. If multiwindow problems become an immediate inconvenience, move 25a earlier without bundling tabs/autosave into it.

## Repeatable build cycle

For every increment:

1. Inspect only the relevant reference submenu using a sample document; log labels, shortcuts, enabled states and actual behavior. Do not block already-understood independent work on unrelated missing captures.
2. Record intended behavior and mapped parity IDs in `docs/build-cycles.md`. QML owns UI/state; C++ owns document I/O and new formatting transformations. Keep macOS implementation separate from Linux.
3. Implement one useful slice. Preserve UTF-8 Markdown, selection/cursor, undo, dirty prompts, recovery and external-change handling. Never add dead menu placeholders to simulate completion.
4. Run `./bin/build` and `./bin/test`. Add behavior tests appropriate to the change, especially for text/file mutations and shared action states.
5. Close QA app normally after preserving unsaved work, then run `./bin/prepare-dev-app`. Reuse `dist/Fomawrite Dev.app` and `io.github.andrewjngray.fomawrite.dev`; announce app attachment. Follow the existing [app-access procedure](development-app-approvals.md).
6. Verify affected native workflows with disposable fixtures; save sanitized screenshots/logs under `research/cycle-NN/` and an optional short exercise under `research/usability/cycle-NN.md`. Inspect exported artifacts when output changes.
7. Record tests, native result, known gaps, runnable artifact and commit. Update the parity row only when its behavior is verified; partial coverage stays Partial. Never commit build/dist outputs or private screenshots. Incorporate Andrew’s feedback into the next increment.

## First concrete build

Start with Cycle 13 native closeout, then Cycle 14. Deliver View and Focus access plus existing inline formatting in native menus, with the same state as the toolbar. Retain current shortcuts unless a documented collision requires a change. Leave unknown nested options for their relevant stage; no inert iA-shaped menus.

## Planning verification

This plan is documentation only. All seven supplied screenshots were viewed and compared with current QML/C++ implementation; no product code was changed and no build, tests or native-app checks were run in this planning pass. Historical 25-test success remains the last recorded result, not a fresh result.
