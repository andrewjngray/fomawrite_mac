# Cycle 9 — compact file rows and delayed tooltips

File tooltips and shared toolbar/organizer hints wait 700ms. The Previews button beside Sort by controls the same persisted excerpt preference as Show Text Excerpts. Filenames retain extensions. A one-time migration hides dates and excerpts for the requested compact default; future user selections persist. Show Date remains available separately.

Build and 23 tests passed. The restricted test run failed the existing file-watcher expectation; all tests passed when rerun with normal macOS access. Logs are in logs/. Native QA used the existing stable development app identity after closing its saved window normally. Verified compact default, then excerpts on and off; left excerpts off. Sample content was not edited. Organizer was hidden for screenshots to exclude private recent filenames; user-supplied screenshot was not copied into the repository.

Screenshots: screenshots/compact-files.png and screenshots/file-previews.png. Exact elapsed tooltip timing, moving across multiple rows, and dark appearance require manual verification. Artifacts: dist/Omawrite Dev.app and dist/Omawrite.app.

Final packaging and deep/strict signature verification passed.
