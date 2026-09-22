# Cycle 19b — rich clipboard (21 September 2026)

**Planned scope / changes:** Copy As Markdown, HTML source or Formatted Text; Paste As Plain Text or Markdown from HTML. Clipboard conversions use Qt Markdown/HTML and atomic replacement.

**Checks:** ./bin/build and ./bin/test passed at this increment (38 tests). Final combined build revalidated all 39 tests; existing Qt Material teardown warnings remain. Native checks below used the final combined build, not five separately installed bundles.

**Native verification:** Formatted Text copy → Markdown from HTML paste preserved sample heading structure; one undo restored exact source. Sample screenshot visually inspected.

**Known gaps:** Cross-application paste interoperability remains untested; Qt conversion normalizes Markdown and is not lossless for every extension. Authorship paste is future work.

**Runnable artifacts:** dist/Omawrite Dev.app (stable io.github.andrewjngray.omawrite.dev) and dist/Omawrite.app refreshed after normal QA-app close; ordinary package signature verification passed. Logs: research/cycle-19b/logs/. Changes remain uncommitted alongside earlier work.

**Optional exercise:** Copy a sample heading as formatted text, paste as Markdown, then undo once.

Final shared packaging/native evidence: [Cycle 20a](../cycle-20a/README.md).
