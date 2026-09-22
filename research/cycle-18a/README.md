# Cycle 18a — block formatting (21 September 2026)

**Planned scope / changes:** Heading/body, bullet/number/task lists, task toggling, blockquote, indent/outdent and line moves. C++ transformations preserve multiline selection and use one undo block.

**Checks:** ./bin/build and ./bin/test passed at this increment (35 tests). Final combined build revalidated all 39 tests; existing Qt Material teardown warnings remain. Native checks below used the final combined build, not five separately installed bundles.

**Native verification:** Native Format → Headings converted the current sample line to heading 2.

**Known gaps:** Nested Markdown containers and quoted/indented fence variants are not a complete Markdown parser; exhaustive native list/line-move checks remain.

**Runnable artifacts:** dist/Omawrite Dev.app (stable io.github.andrewjngray.omawrite.dev) and dist/Omawrite.app refreshed after normal QA-app close; ordinary package signature verification passed. Logs: research/cycle-18a/logs/. Changes remain uncommitted alongside earlier work.

**Optional exercise:** Select two sample lines, apply Numbered List, then undo once.

Final shared packaging/native evidence: [Cycle 20a](../cycle-20a/README.md).
