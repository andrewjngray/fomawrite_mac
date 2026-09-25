# Next development phases: visual writing and export styles

Proposed 25 September 2026 after Cycle 61; updated after the Cycle 62–69 implementation pass. The table records original gates, not a claim that every gate is complete. It adds Ulysses and Typora-inspired work to the existing [iA Writer menu backlog](ia-writer-inventory.md) and [release acceptance ledger](release-acceptance.md); the open iA gaps remain open. Cycle numbers are proposed sequential checkpoints and may split into lettered increments when needed.

## Reference audit and current baseline

Andrew's 13 Ulysses screenshots are in the local `screenshots/ulysses_screenshots/` folder. They were inspected for this plan; the raw captures include unrelated desktop content and are not part of the committed evidence. Capture times below identify each reference without copying its imagery or assets.

| Reference surface | Screenshot times (25 September 2026) | Fomawrite today | Proposed change |
|---|---|---|---|
| Compact writing toolbar, link editor | 5:52:58, 5:53:09 | Bold, Italic, Link and other formatting exist in the lower toolbar and menus; Markdown source is editable | Put a compact, keyboard-accessible formatting cluster at the upper right; add a link target/title popover; share the existing commands and enabled states |
| Paragraph and insertion controls | 5:53:09, 5:53:30 | Heading/list/quote and many inline insertions exist, but the placement and grouping differ | Group paragraph styles and insertions by writing task; audit support before exposing Ulysses-like actions such as annotation, redaction, video, equation, comment and raw source |
| Editing rendered text | 5:52:58 and writing view throughout | Source editor is writable; `PreviewPane.qml` uses a read-only rendered `TextEdit` | Add a visual editing mode backed by the same plain Markdown document, with safe source-range edits and explicit limits for unsupported syntax |
| Export button and live style preview | 5:52:07, 5:54:08–5:54:51 | File menu exports HTML/PDF; a shared C++ formatter has named output presets, custom settings and paginated preview | Add an upper-right Export/Share affordance opening one export hub with format, style and paper controls plus live output preview |
| Export style gallery | 5:55:10, 5:55:26 | Several built-in output templates and one custom style exist; no browsable gallery/editor | Offer Fomawrite-owned preset gallery, duplication and bounded style editing; assess import separately |
| Typora's direct editing and CSS model | User's broader reference | Fomawrite has editor themes separate from output templates | Use as interaction inspiration for direct editing and user CSS, without assuming Typora theme files map to Fomawrite's Qt renderer |

The seven Ulysses export-preview captures show the same example in different PDF/DOCX-style treatments (Academia, Business, Columns, Manuscript, Papers, Revision and Swiss Knife), with A4 selection and a large page preview. Those are examples of the **range of layouts** to support, not preset names or style definitions to copy. The existing Fomawrite presets are Modern, Classic, Manuscript Mono, GitHub, Helvetica, Palatino and MLA Draft, plus Custom. Editor themes, visual editing, and export styles are separate controls.

## Product and technical decisions

- Plain UTF-8 Markdown remains the canonical document. Visual editing must apply bounded transformations to source spans. Do not simply make the current rendered `TextEdit` writable or serialize its whole document back to Markdown: either could rewrite untouched syntax, links, comments, tables or extensions.
- Start visual editing with an explicit supported subset. Preserve unsupported constructs visibly and route them to source editing when necessary. No silent deletion or normalization. Source and visual modes share dirty state, undo/redo, recovery, external-change handling and save prompts. Switching modes never saves or reformats by itself.
- The current C++ formatter remains the base for live preview and export. Style definitions need a versioned, local format with portable fonts/fallbacks and a documented supported-property set. Editor themes must remain independent.
- Draw original Fomawrite icons and styles. Ulysses' HTML/ePub styles use CSS, while its PDF/DOCX styles use a Ulysses-specific format. Its published terms do not clearly grant a blanket redistribution licence for third-party app bundles, so do not bundle downloaded Ulysses style files or copied assets. A later user-supplied style importer is conditional on a format and rights review. This is a planning constraint, not a legal determination.
- Keep existing iA menu behavior and shortcuts discoverable; new toolbar actions should use the same commands. Update the inventory only after actual native behavior is verified.

## Proposed delivery sequence

| Cycle | Shippable result | Acceptance gate |
|---|---|---|
| **62 — source/visual editing foundation** | Define supported Markdown subset and fixture corpus; prototype source-span-to-rendered-range mapping and round-trip invariants; record a design decision for visual mode, selection and undo | Untouched source bytes remain identical across mode switches on fixtures with headings, links, lists, tables, footnotes, code fences, images, comments and Unicode. Unsupported constructs have a clear source-edit route. No user document is migrated. |
| **63 — compact writing controls** | Upper-right Bold/Italic/Link/paragraph cluster and link popover; context-sensitive placement for narrow windows; original icons | Source editor actions match existing Format menu, keyboard commands, selection/caret behavior and single-step Undo. Link editing preserves title/URL escaping. VoiceOver names, focus order and dark mode are checked. |
| **64 — visual editing baseline** | Writable rendered mode for ordinary paragraphs, headings, emphasis and strong text; mode or split-view switching against the same document | Edit, save, reopen, Undo/Redo and recovery reproduce the intended Markdown. Unsupported blocks stay intact, with no whole-document rewrite; source and rendered selection follow each other where mapping is supported. |
| **65 — richer visual editing** | Lists, task boxes, quotes, links and images in manageable slices; insertion palette only for actions the editor supports | Multiline selection, nested constructs, Unicode/IME, paste, drag where supported, and one-step Undo are checked. Malformed/unsupported syntax retains its source; partial syntax coverage is labelled clearly. |
| **66 — unified export hub** | Upper-right Export/Share control; PDF/HTML preview, format, style, paper size/orientation and destination selection, reusing the current formatter | Live preview and saved output match on representative pages, tables, images and headers/footers. Cancel/save failure leave source and prior output untouched. Existing File menu and context export routes open the same flow. |
| **67 — Fomawrite style gallery/editor** | Original preset thumbnails; duplicate/create/edit/delete user styles; bounded font, hierarchy, spacing, margins, page furniture and supported layout controls | Style changes preview without modifying Markdown; restart restores selections; invalid styles have useful errors and safe fallback. Visual PDF and HTML fixtures compare styles and paper sizes. |
| **68 — format and import expansion** | Make separate decisions on DOCX/ePub export and user-provided CSS/import. Implement the first viable, demand-backed slice rather than promising Ulysses file compatibility | Exported documents open correctly in representative readers. CSS is scoped to output and validated. Any Ulysses-specific importer requires a demonstrated parser/mapping and rights decision first; unsupported constructs are reported rather than ignored. |
| **69 — integrated acceptance** | Cross-mode/output regression, accessibility and release audit; reconcile new work with iA backlog and documentation | Saved/untitled/dirty/recovered documents, multiple windows, long files, external edits, narrow/dark layouts, keyboard/VoiceOver and sample exports pass. `./bin/build`, `./bin/test`, native Dev app checks and sanitized evidence are recorded. Remaining gaps stay explicit. |

Cycles 62–68 now have bounded implementations; Cycle 69 is a partial native acceptance checkpoint. The final native matrix and export-output comparisons in the gate remain open. Cycles 64–65 are the largest data-integrity risk; they should be split if source mapping or undo needs more bounded work. Cycles 66–67 can advance independently once the style/output model is settled, but their UI should share one export flow. Cycle 68 is a decision gate, not a commitment to support every proprietary style or output format. Completion of Cycle 69 means these *newly scoped* features meet their gates; it does not automatically close the older iA Writer release ledger.

For each implemented cycle, update [build-cycles.md](build-cycles.md) with scope, changes, tests, native verification, known gaps, runnable artifact and a short optional exercise. Keep sanitized sample evidence under `research/cycle-NN/`; exclude private writing and raw reference screenshots. Run `./bin/build` and `./bin/test` for editor changes, then verify affected native workflows in the stable Fomawrite Dev app after preserving unsaved work.

## External reference notes

- [Ulysses: Getting Started with Styles & Themes](https://help.ulysses.app/customize-ulysses-with-styles-and-themes?kb_language=en_US) distinguishes editor themes from export styles and explains CSS versus Ulysses-specific PDF/DOCX styles.
- [Ulysses: Export](https://help.ulysses.app/export?kb_language=en_US) describes its toolbar export flow and formats.
- [Ulysses: PDF/DOCX style format](https://help.ulysses.app/styles-themes/pdf-docx-styles) documents its proprietary style model.
- [Ulysses Styles & Themes terms](https://styles.ulysses.app/terms) inform the redistribution caution above.
- [Typora: custom CSS](https://support.typora.io/Add-Custom-CSS/) is a reference for user-controlled presentation.
