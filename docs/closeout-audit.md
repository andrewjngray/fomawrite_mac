# Closeout implementation audit — 21 September 2026

Andrew asked to close the remaining cycles. This pass implements working slices across 20b–27 and starts the Cycle 28 audit. **The original plan is not fully closed:** several slices remain narrower than the planned acceptance criteria. Touching all cycle areas is not the same as verified iA parity. Keep the gaps below open; do not reset the remaining work count to zero.

Final verification: `./bin/build`; `./bin/test` **48 passed, 0 failed** with normal macOS filesystem access. An initial sandboxed 20b run could not access recovery files; its normal-access rerun passed. Subsequent feature subsets were integrated and revalidated in combined builds, not fifteen separate native installations. Final logs: [Cycle 28](../research/cycle-28/logs/). Existing Qt teardown warnings remain. Code/document whitespace checks pass; pre-existing user example whitespace was not changed.

Stable artifacts: `dist/Omawrite Dev.app`, bundle ID `io.github.andrewjngray.omawrite.dev`, and `dist/Omawrite.app`, locally ad-hoc signed. User's existing dirty document was saved through the app before normal close; no private screenshot was retained. Agent-created temporary dirty samples were discarded. Changes remain uncommitted with pre-existing work.

## Current coverage and unfinished acceptance

| Cycle | Implemented | Still open |
|---|---|---|
| 20b | Command palette and sentence focus | Palette covers the existing workspace registry, not every new file/edit command. Unicode sentence boundaries are not an abbreviation/language grammar model; broader language/performance checks remain. |
| 21a | Bounded saved-content search | Not a persistent index: rereads bounded files. Unsaved buffers are excluded. Tree/list/date options not confirmed beyond existing controls; native large-library stress remains. |
| 21b | Saved queries and tags | Saved queries are in Quick Open, not organizer smart-folder rows. Nested Markdown code cases and a browsable tag index remain; these are partial smart-folder semantics. |
| 22a | Preview navigation | Scroll synchronization is proportional, not semantic paragraph/image alignment. Full long-document native anchor/scroll and cross-file fragment checks remain. |
| 22b | Markdown extension subset | Wikilinks do not search nearest matches across the library. Multiline/backlinked footnotes, CSV/image/code content blocks, full title syntax and rebasing ordinary relative links inside nested includes remain. Missing includes show a message. |
| 23a | Export and print | Native export picker cancellation, multi-page/image/table layout, page-break insertion and portable asset bundling remain unverified/unimplemented. Exported HTML links local assets; moving the HTML alone is not a self-contained export. |
| 23b | Output style foundation | This is a font-style foundation, not complete preview/output templates. No persisted template selection, template asset system, headers/footers/title pages, paginated preview or fit-page/fit-width modes. |
| 24 | Mac presentation subset | Share-sheet open/cancel passed; remaining window-presentation checks remain. Title/toolbar fade/statistics-only modes and external-display behavior are not implemented in this pass. |
| 25a | Coordinated document windows | Separate application launches are not consolidated. Saved-window restoration across restart and exhaustive crash/OS-quit/minimized-window matrix remain. Clean windows already closed before a later Cancel are not reopened automatically. |
| 25b | Native tabs | Restart restoration/tab-group persistence and exhaustive dirty-close/overview/fullscreen/keyboard states remain. Some tab commands remain enabled when not applicable. |
| 25c | Opt-in autosave and saved versions | No automatic version capture per save or native Versions-browser UI. Versions cover Markdown, not authorship sidecars. Autosave has a remaining external-writer race between comparison and write; disk-full/disconnection/crash stress remains. Default is off. |
| 26a | Native spelling subset | Inline spelling/grammar, automatic substitutions/corrections, speech and completion integration remain. Selection spelling reports words only; no suggestions or replacement workflow. |
| 26b | Writing analysis foundation | Analysis is an explicit panel, not live parts-of-speech highlighting/style checking. Select prose manually; code not excluded automatically. No exact iA linguistic-output claim. |
| 27 | Manual authorship annotations | Annotations are assertions, not verified provenance. Inserted text can inherit nearby labels. Clipboard/export, paste-edits/author-aware merging and full attribution visualization remain. Native versions do not version sidecars. |
| 28 | Release/parity audit — still open | Not full parity or release sign-off. Complete the acceptance gaps in this matrix and earlier Move To success/separate-volume, exact tooltip timing, clipboard cross-app, accessibility/dark/narrow/picker checks. No notarization/public release. |

## Reference sources

Extension implementation consulted iA's published [content-block syntax](https://github.com/iainc/Markdown-Content-Blocks), [wikilink support](https://ia.net/writer/support/library/wikilinks) and [settings syntax](https://ia.net/writer/support/basics/settings). The implementation is deliberately documented as a subset. Output uses [Qt QTextDocument](https://doc.qt.io/qt-6/qtextdocument.html). Native tab/version integration uses AppKit/Foundation APIs; checkpoints use coordinated NSFileVersion creation. No proprietary templates or implementation code were copied.

## Follow-through order

1. Finish data-safety integration: ownership across launches, tab/session restore, version/autosave race coverage, authorship path/clipboard/recovery edge cases.
2. Complete output/Markdown scope: templates/paginated preview, page breaks, asset portability, extended footnotes/content blocks/link resolution.
3. Finish library and writing-service scope: organizer smart folders, tag navigation, spelling/grammar/substitutions/speech and live linguistic highlighting.
4. Finish reference-menu/settings/context-menu inspection and native accessibility/dark/narrow/hardware QA, then decide public signing/notarization separately.


## Cycle 29 follow-up

Authorship migration for Duplicate/Rename/Move is implemented and verified with 51 passing tests and native sample operations. Dirty annotations stay in the current buffer and recovery snapshot; Duplicate includes them, while Rename/Move transport saved sidecars with saved bytes. See [Cycle 29](../research/cycle-29/README.md) for verification and remaining failure-mode limits. This closes that path-operation subset only; the broader audit above remains open.

## Cycle 32 follow-up

Same-installation cross-launch ownership and saved-file window/cursor/native-tab restoration are now implemented. Native two-tab restart and duplicate-launch focus passed; 56 regression tests pass. The Cycle 25a/25b rows above describe the original audit and are superseded for these specific items. Their exhaustive lifecycle, multi-dirty cancellation, fullscreen/minimized and display checks remain open. [Ten planned cycles remain](remaining-cycles.md), including Andrew's new themes concept as Cycle 33.

## Cycles 33 / 34a follow-up — 23 September 2026

Workspace themes are implemented as a separate product addition. Cycle 34a fixes recovery baseline handling and makes multi-document Quit cancellation retain earlier windows/drafts. A real synthetic subprocess-exit test recovers two annotated drafts and protects intervening external edits; native two-tab Discard/Cancel passed. 59 tests pass. The old sequential-quit limitation is superseded; remaining safety/version/display stress is still open. Nine cycles remain, counting the unfinished part of 34.
