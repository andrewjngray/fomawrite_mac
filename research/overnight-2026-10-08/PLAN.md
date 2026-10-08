# Overnight plan — 8 October 2026 (21:50 → 03:30)

Andrew's brief: run the next couple of cycles we discussed; Fable plans, cheap subagents build, a separate subagent reviews each cycle's work; be frugal with tokens; a complete report by the morning. Install-first rule from this morning: every cycle ends with an installed, aligned build and a one-line "what to try".

## What was discussed and what can be built without Andrew

| Discussed | Overnight? | Why |
| --- | --- | --- |
| Cycle 136 — Live follows the theme, lighter | Done, installing now | Decided this evening. |
| Grammarly Desktop experiment | No | Needs Andrew at the keyboard with his Grammarly account. Steps written for him in the report. |
| Roam "send to daily note" (harness shape 1) | No | Still a Maybe in IDEAS.md ("do not build a Maybe"), and there is no local Roam CLI or token path on this machine: the Roam connector is a claude.ai connector, so a Qt-side integration needs an API token decision from Andrew. A feasibility note is written instead. |
| Live as the daily editor: fidelity and robustness | Yes | Decided direction; the page agent reported concrete gaps in the theme mapping, and the native harness has three theme-folder failures. |

## Cycle 137 — Live theme fidelity (builder: page agent; reviewer: separate agent)

Goal: the bundled and Typora-style themes look right in filtered Live, closing the gaps the mapping agent reported.
1. Distinguish `ul`/`ol` lines (`fw-list-line` gains `fw-list-ul`/`fw-list-ol` from the Lezer list node) so `ol li`/`ul li` theme rules map correctly.
2. Absolute `px`/`pt` heading sizes in mapped copies become `em` relative to the theme's base size (or the host's 17px when the theme sets none), so Larger/Smaller act on headings too.
3. Quote mapping: `padding` shorthand and `margin-left` for `.fw-quote-line`, and theme `blockquote::before` glyphs are dropped deliberately (documented).
4. A gallery capture test: every bundled theme/preset rendered in filtered Live, light and dark, saved under `research/cycle-137/captures/`; frames must be non-uniform; reviewed by eye by the reviewer agent and by me.
5. Host: nothing unless the review finds it.

## Cycle 138 — native theme-folder checks and Live selection robustness (builder: Qt agent; reviewer)

1. Fix the three failing `check-document-views` theme-folder checks ("Cannot create disposable folder themes"): find why `writeTheme` fails against the build-135/136 bundle and make the fixture or the app right; rerun the harness.
2. The Live selection race: Format commands issued within the 30 ms cursor debounce could act on a stale selection. Page-side `wrap`/`replace` commands with arguments (`runCommandWithArgs`) so the page applies at its own current selection; host routes `liveWrapSelection`/`liveReplaceSelection` through them when the page is ready. Red/green test.
3. Install, report.

## Reviewer protocol

After each builder finishes and I merge: a Sonnet reviewer gets the diff range, the design note and the record, runs the page tests and the targeted Qt tests, and reports defects, test gaps and doc drift. I fix or send back; nothing is installed until the reviewer's findings are addressed or explicitly recorded as open.

## Time checks

21:50 start · 00:00 Cycle 137 installed · 02:30 Cycle 138 installed · 03:30 report written, memory updated, worktrees removed.
