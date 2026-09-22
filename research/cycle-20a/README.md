# Cycle 20a — navigation and quick-open (21 September 2026)

**Planned scope / changes:** Independent per-window document and library histories, cursor restoration, enclosing folder, basic source-link opening and cancellable recursive filename Quick Open. Search excludes hidden/symlink/build/dependency folders and caps at 20,000 entries or 100 results.

**Checks:** ./bin/build and ./bin/test passed at this increment (39 tests). Final combined build revalidated all 39 tests; existing Qt Material teardown warnings remain. Native checks below used the final combined build, not five separately installed bundles.

**Native verification:** Quick Open found two disposable fixtures and Return opened the first. Open Link at Cursor opened the second; Back restored the first cursor. Forward with unsaved sample changes prompted; Cancel preserved them. Agent edits were undone and sample saved clean. Sample-only screenshots visually inspected.

**Known gaps:** History is session-local and New resets it; not coordinated across windows. Source links support basic inline/autolinks, not reference links or nested parentheses. Local nontext targets are unsupported. Native large-library stress, full keyboard/dark/narrow checks and earlier Move To/separate-volume/tooltip timing QA remain pending.

**Runnable artifacts:** dist/Omawrite Dev.app (stable io.github.andrewjngray.omawrite.dev) and dist/Omawrite.app refreshed after normal QA-app close; ordinary package signature verification passed. Logs: research/cycle-20a/logs/. Changes remain uncommitted alongside earlier work.

**Optional exercise:** In the examples location, Quick Open “Cycle20”; follow Second sample, go Back, edit, then try Forward and Cancel.

Final shared packaging/native evidence: [Cycle 20a](../cycle-20a/README.md).
