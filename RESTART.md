# Restart script — Fomawrite, 9 October 2026

Where things were left, what to check first, and what to say to pick up again.

## State at shutdown (07:50, 9 Oct)

- **Installed:** build 141 (`0.3.0-dev37`) on `/Applications/Fomawrite.app`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`, all identical.
- **Source:** master at `5b679a4`, pushed, clean tree, no agent worktrees, CI green on the last checked push.
- **Tests:** Qt 221 / page 256, both native harnesses passing against build 140.
- **Documents of record:** [MISSION.md](MISSION.md) (the measure: is Live good enough to be the daily editor?), [STATUS.md](STATUS.md), [docs/live-friction.md](docs/live-friction.md), [IDEAS.md](IDEAS.md), [research/cycle-140/README.md](research/cycle-140/README.md) (builds 140 and 141), [research/overnight-2026-10-08/REPORT.md](research/overnight-2026-10-08/REPORT.md).

## Ten minutes when you are back

1. Open the installed app. Open a document. Choose **Lapis Dark** in the Output Style: the page in Live and the Web pane should be dark with readable text, and the window chrome should go dark with it.
2. Click **Preview** in the footer capsule, then **Single**. Both should be one click.
3. Open **View → Editing** straight after a Preview Only start: Source or Live should show a tick.
4. Type `==a highlighted phrase==` in Live: it should render highlighted and stay readable under Lapis Dark.
5. Note anything wrong or awkward in [docs/live-friction.md](docs/live-friction.md), or just tell me.

## Decisions still with you

- **Grammarly**: ten-minute script in [research/overnight-2026-10-08/grammarly-experiment.md](research/overnight-2026-10-08/grammarly-experiment.md).
- **Roam** (Maybe in IDEAS.md): needs a Yes/No and an API token decision; see [research/overnight-2026-10-08/roam-feasibility.md](research/overnight-2026-10-08/roam-feasibility.md).

## Known open items (not blocking)

- A theme that sets only a dark `mark` background without `--highlight-color` can still be unreadable.
- Theme-set link and quote colours are not contrast-checked on dark pages.
- The highlight fix's red/green check has no recorded red run (see the record).
- A Source-editor capture under Lapis Dark and a Live → Preview → Single round-trip test are not written yet.

## What to say to resume

- "Resume Fomawrite" — I read STATUS.md and the friction list and report where we are.
- "Install" — only when the app is closed; it runs the suite, packages, refreshes the Dev bundle and installs.
- "Next cycle from the friction list" — I plan it, cheap agents build, a separate agent reviews, install-first.

## Rules we agreed

- Install first, then report; every cycle ends with an installed, aligned build.
- Never build a Maybe from IDEAS.md.
- Questions to you end with a question mark.
