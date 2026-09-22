# Cycle 18b — inline and structural insertion (21 September 2026)

**Planned scope / changes:** Toggle bold/italic/strike/inline code; backtick-safe delimiters, atomic link insertion, fenced code block and horizontal rule insertion.

**Checks:** ./bin/build and ./bin/test passed at this increment (36 tests). Final combined build revalidated all 39 tests; existing Qt Material teardown warnings remain. Native checks below used the final combined build, not five separately installed bundles.

**Native verification:** Command-B applied and removed bold on the selected heading text, retaining the inner selection.

**Known gaps:** Exact iA delimiter semantics and exhaustive escaped/nested markup combinations remain unverified.

**Runnable artifacts:** dist/Omawrite Dev.app (stable io.github.andrewjngray.omawrite.dev) and dist/Omawrite.app refreshed after normal QA-app close; ordinary package signature verification passed. Logs: research/cycle-18b/logs/. Changes remain uncommitted alongside earlier work.

**Optional exercise:** Select a word, press Command-B twice, then try Code Block and undo.

Final shared packaging/native evidence: [Cycle 20a](../cycle-20a/README.md).
