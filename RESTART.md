# Restart script — Fomawrite, 11 October 2026 (morning after the Harper night)

Where things were left, what to look at first, and what to say to pick up again. The overnight report is [research/overnight-2026-10-10/REPORT.md](research/overnight-2026-10-10/REPORT.md).

## State at hand-over

- **Installed:** see the report for the build number on `/Applications/Fomawrite.app`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app` (all identical when the report was written).
- **Source:** master pushed, clean tree, CI green on the last checked push (the report names the commit).
- **Tests:** Qt and page counts in [STATUS.md](STATUS.md); `./bin/fetch-harper` must have run once (bin/build does it); the suite rewrites the tracked gallery captures, `git checkout research/cycle-13*/captures` afterwards.
- **Documents of record:** [MISSION.md](MISSION.md), [STATUS.md](STATUS.md), [docs/harper-plan.md](docs/harper-plan.md), [research/cycle-145/README.md](research/cycle-145/README.md), [research/cycle-146/README.md](research/cycle-146/README.md), [IDEAS.md](IDEAS.md).

## Ten minutes when you are back

1. Open the installed app, a real document, Source mode. Type a sentence with a doubled word, a misspelling and "could of". Red, blue and (after Edit → Writing Checker → Check Style While Typing) grey marks; right-click one.
2. Edit → Spelling and Grammar (⌘:): the Review pane lists them with Harper's kinds; the status line says "Harper 2.10.0 (built-in)".
3. Edit → Writing Checker → Dialect → American: "colour" goes red; back to Australian: it clears.
4. Help → Check for Writing Checker Updates…: a banner says up to date or installs the newer release and reloads the engine.
5. Output Style → Claude Like: the Claude Code look. If you want the old serif back, Import Theme… docs/themes/claude-serif.css.
6. Tell me what is wrong or awkward.

## Decisions still with you

- Three real documents to run Harper over (no Grammarly login needed).
- Memory: the hidden Live page with Harper costs about 450 MB per window. Options in the Cycle 145 record (do not warm up when the engine is macOS; one engine across windows; unload after idle).
- Roam: still a Maybe. AI tools "file is the API" spike: not started.

## What to say to resume

- "Resume Fomawrite" — I read STATUS.md, the two Harper records and the friction list, and report where we are.
- "Install" — only when the app is closed.
- "Next cycle from the friction list" — I plan, cheap agents build, a separate agent reviews, install-first.

## Rules we agreed

- Install first, then report; every cycle ends with an installed, aligned build; check CI after every push.
- Never build a Maybe from IDEAS.md. Questions to you end with a question mark.
