# Cycle 19a — small editing tools (21 September 2026)

**Planned scope / changes:** ISO date, basic two-column table, Unicode case conversion and Clear Surrounding Inline Styles. Protected code/link-looking selections are refused for case/clear operations.

**Checks:** ./bin/build and ./bin/test passed at this increment (37 tests). Final combined build revalidated all 39 tests; existing Qt Material teardown warnings remain. Native checks below used the final combined build, not five separately installed bundles.

**Native verification:** Native Change Case → UPPERCASE converted the selected sample heading text.

**Known gaps:** Clear Styles is limited to surrounding inline markers; case conversion is not language-specific title casing. Native date/table coverage remains pending.

**Runnable artifacts:** dist/Omawrite Dev.app (stable io.github.andrewjngray.omawrite.dev) and dist/Omawrite.app refreshed after normal QA-app close; ordinary package signature verification passed. Logs: research/cycle-19a/logs/. Changes remain uncommitted alongside earlier work.

**Optional exercise:** Select a word and change its case; insert a table on a blank line and undo.

Final shared packaging/native evidence: [Cycle 20a](../cycle-20a/README.md).
