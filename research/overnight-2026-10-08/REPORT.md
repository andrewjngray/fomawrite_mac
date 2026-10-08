# Overnight report — 8 October 2026

Brief: run the next cycles we discussed; Fable plans, cheaper subagents build, a separate subagent reviews; install-first; a complete report by morning.

## Outcome in one paragraph

Three cycles landed and one build is installed: **build 138** (`0.3.0-dev34`) in `/Applications`, `dist/Fomawrite.app` and `dist/Fomawrite Dev.app`, all identical. Cycle 136 (Live follows the publishing theme, lighter) was built, reviewed, found wanting in two important ways, and fixed in Cycle 137; Cycle 138 fixed the Live selection race and the native theme-folder acceptance failures. The Qt suite is at 212, the page suite at 248, the native document-views harness passes in full for the first time since build 130, and CI is CI_STATUS. The Grammarly experiment and the Roam harness were not built: the first needs you at the keyboard, the second is still a Maybe and needs a token decision; both have notes ready.

## What to try this morning (build 138 is installed)

1. Open a document, switch to **Live**, and choose an Output Style from the right-hand pane. Live should take the theme's faces, colours, heading rules and quote styling but keep the editor's own column width and no decoration. Switch dark mode on: text should stay readable under every bundled style.
2. **View → Live Follows Theme Exactly** (only enabled in Live): the theme's geometry comes back.
3. In Live, the left footer control names the Output Style and opens the same theme menu as the right pane. Larger/Smaller act on Live too.
4. Select a few words and immediately press ⌘B: the bold lands on what you selected, not the previous selection.

## Cycle by cycle

| Cycle | Built by | Reviewed by | Verdict → action |
| --- | --- | --- | --- |
| 136 Live follows the theme, lighter | two page agents (overlay; selector mapping) + host work by Fable | reviewer agent | Ship with fixes: host font beat a theme's body font; readability fill misfired on body-painted themes (Claude Like unreadable in dark); exact mode lost mapped typography; tables not full measure; revealed links kept theme underline; host nits. All fixed in 137. |
| 137 Live theme fidelity | page agent (ul/ol, px→em, quote spacing, bundled-CSS robustness); fix agent for the 136 findings; gallery test by Fable | reviewer agent | **Do not ship** as first built: the seven basic presets were unreadable in dark Live (white `html` read through an opaque dark body), the revealed-link fix did not match the real DOM, dark table header under a light theme, missing `measureTheme` tests, cascade engine skipped `>`; exact-theme command gated only in the native menu. Second fix agent closed all of it; a WCAG contrast assertion on every gallery frame was red before (preset 0 dark: 1.01) and is green now. |
| 138 Selection race and theme-folder checks | one Qt+page agent | reviewer agent | Ship with fixes: queued commands not invalidated on document swap; late reply clobbered the last cursor. Fixed by Fable with a bare-bridge test. Root cause of the harness failures: a fixture that assumed a folder the app deliberately stopped creating in Cycle 132; fixture fixed, app unchanged. |

Records: [cycle-136](../cycle-136/README.md), [cycle-137](../cycle-137/README.md), [cycle-138](../cycle-138/README.md). Plan as written at 21:50: [PLAN.md](PLAN.md).

## What the reviewers caught that tests had missed

- A green suite with 16 gallery captures still hid unreadable dark presets, because the test only compared frames for difference. The fix was a contrast measurement on the captured pixels, which now guards every bundled style in both appearances.
- A unit test on a fake DOM passed while the real DOM structure made the rule a no-op (revealed links). The replacement test is built from the real decoration output.
- The page's own cascade test engine silently ignored child combinators, so an overlay rule could be asserted "applied" when it was never evaluated. It now parses `>` and throws on forms it cannot model.

## Not done, and why

- **Grammarly**: needs your account and eyes; ten-minute script in [grammarly-experiment.md](grammarly-experiment.md).
- **Roam send-to-daily-note**: still a Maybe in IDEAS.md and there is no local Roam CLI or token path on this machine (the connector is a claude.ai one); feasibility and the decision you would need to make are in [roam-feasibility.md](roam-feasibility.md).
- **Live by physical input**: still never done by a person. Build 138 is the one to try.

## Process notes for you

- Subagent spend: nine agents (five builders, three reviewers, one fix pass) at roughly 100–225k tokens each; Fable did planning, merges, host QML/C++ work, the gallery/contrast test, the 138 review fixes, docs and installs.
- Two reviewers changed the outcome: without them build 137 would have shipped with unreadable dark presets and a revealed-link rule that did nothing. Keeping a separate reviewer per cycle is worth its cost.
- One mistake of mine, caught by a reviewer: a worktree gitlink was committed twice before `.claude/` went into `.gitignore`. The second one is removed from the tree (it remains in history).
- Open follow-ups recorded in STATUS.md: theme-set colours are not contrast-checked (a theme's own dim quote or link colour stays dim on a dark page); commands sent through other paths (undo, link panel) are not queued behind a pending selection request; Live typewriter and focus modes under the new table CSS were not visually checked.
