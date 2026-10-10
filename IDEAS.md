# Fomawrite — ideas

A parking place for ideas about where the project could go. Nothing here is a commitment. An idea may end up changing [MISSION.md](MISSION.md), [ARCHITECTURE.md](ARCHITECTURE.md), the README, the roadmap or nothing at all. Andrew owns the decisions; anyone can add an entry.

Each idea carries one decision box. New ideas start as **Maybe**. Move the `x` when a decision is made and add a line saying why.

```
Yes [ ]  No [ ]  Maybe [x]
```

Larger concepts get their own document and only a pointer here.

## Likely candidates

Things the Cycle 134–135 work surfaced that are plausible next cycles once the current measure ([MISSION.md](MISSION.md): is Live the daily editor?) is answered.

### Spell check in Live
Yes [x]  No [ ]  Maybe [ ]

Yes, 10 October 2026 (Andrew): spell check in the document first, in both surfaces, from the macOS checker, with a toggle; the review dialog is reworked after. Cycle 143.

Source mode uses the native Qt text editor and its spell check; the Live page is Chromium and has none today. Qt WebEngine can spell check but needs bundled `.bdic` dictionaries, which Homebrew Qt does not ship. Options: bundle dictionaries for the languages Andrew writes in, or rely on an external checker (see Grammarly below).

### Grammarly in the editor
Yes [ ]  No [ ]  Maybe [x]

Ten-minute experiment script for Andrew: [research/overnight-2026-10-08/grammarly-experiment.md](research/overnight-2026-10-08/grammarly-experiment.md).

Plan of inquiry for a Grammarly-like result by other means: [docs/grammarly-like-plan.md](docs/grammarly-like-plan.md).

**Decision, 10 October 2026 (Andrew):** build Harper (Automattic, Apache-2.0, offline) into the tool as the grammar engine, with periodic update checks and a menu trigger. Plan: [docs/harper-plan.md](docs/harper-plan.md).

**Result, 9 October 2026:** Grammarly Desktop attaches to nothing in Fomawrite (works in Word on the same Mac). Cause found by probe: neither Qt Quick nor Qt WebEngine reports character bounds to macOS Accessibility (upstream stubs), so an overlay cannot place its button or underlines. The SDK route closed in January 2024. Remaining options are in the experiment note: a Source-only accessibility fix in the app (unproven), nothing for Live short of patching Qt, or the system checker wired into Live with a toggle.

Requirement from Andrew: it must be switchable on and off. The Grammarly overlay (the small moving icon) is distracting when writing and creating; the purity of the editor and the clean panes are a feature, not a bug. Andrew will supply screenshots of how it behaves on his other devices.

Andrew has a Grammarly account and finds its grammar checking good when it appears over text fields in Chrome. Three routes, in order of likelihood:
1. **Grammarly Desktop for Mac** overlays its suggestions on text fields of other apps. Whether it attaches to the Live page (a Chromium `contenteditable` inside Qt WebEngine) and to the Source editor (a Qt Quick `TextEdit`) is untested. Cheapest first step: install Grammarly Desktop, open build 135, try both surfaces, record what happens.
2. **Grammarly's browser-side SDK.** Grammarly offered a JavaScript Text Editor SDK for web apps; its current availability and terms need checking before planning on it. If it still exists it would fit the Live page directly.
3. **No official API.** If neither works, the fallback is native spell check plus a "copy to Grammarly / paste back" convenience, which is not much of an integration.

### Images and footnote references inside table cells (Live)
Yes [ ]  No [ ]  Maybe [x]

Cells render bold, italic, code, links and now inline math; images show as alt text and `[^n]` stays literal. Needs the image request plumbing from `blocks.ts` and the footnote numbering from `extras.ts` inside the table widget.

### Mermaid diagrams in Live and publishing
Yes [ ]  No [ ]  Maybe [x]

Typora renders ` ```mermaid ` fences. Adds about 2.5 MB to the editor bundle; publishing would need the same renderer in the Web/PDF path. Only worth it if Andrew writes diagrams.

### Drop the C, C++, PHP and Rust fence grammars
Yes [ ]  No [ ]  Maybe [x]

Saves about 280 KB of the 1.4 MB editor bundle. Costs highlighting for fences in those languages.

### A fuller code editing appearance
Yes [ ]  No [ ]  Maybe [x]

Code today is a lightweight Source appearance: line numbers, indent guides, no wrap, syntax colours for known fences. Andrew likes it as is; fleshing it out (bracket matching, folding, more languages) is possible later.

### Authorship marks visible in Live
Yes [ ]  No [ ]  Maybe [x]

Authorship ranges are format-only `QTextDocument` edits and are deliberately not pushed to the page, so they are invisible in Live.

## Concepts beyond the editor

Bigger ideas where Fomawrite is the human side of a larger system. Each gets its own document when it is worked on.

### Roam Research as the home of distilled thought
Yes [ ]  No [ ]  Maybe [x]

Andrew uses Roam for his most considered, distilled notes and for daily notes, while Markdown files and AI tools generate far more material than that. The concept: Fomawrite is the human-to-machine interface for writing; Roam is where the small, precious, settled things live; the project becomes a harness between them. Worked out in [roam_concept.md](roam_concept.md).
Feasibility of the first shape (needs an API token decision; no local Roam CLI exists): [research/overnight-2026-10-08/roam-feasibility.md](research/overnight-2026-10-08/roam-feasibility.md).

### An AI workbench pane
Yes [ ]  No [ ]  Maybe [x]

A tab or left-hand pane from which Andrew can open and move between the AI tools he uses (Claude, Claude Code, Codex, ChatGPT, Cursor, Grok) with the current document as context, instead of switching apps and re-pasting. Open questions: which of these have APIs or deep links that can carry a document in and bring text back; whether this is a launcher (easy), a chat surface (medium) or a round-trip editor (hard, and it touches the "file is the truth" rule, because AI output must come back as plain Markdown edits).

### Vaults: deep search over a lifetime of data
Yes [ ]  No [ ]  Maybe [x]

The semantic indexing work with Rami: `potentiacap/vaultmcp` (document vault with a built-in MCP server, based on the Recoll indexing code), `potentiacap/super_email_search_mcp` and `potentiacap/pst-agent` for Outlook PST/OST archives going back about thirty years. Four vaults are registered on this machine today (a home folder, a PST archive and two legal matters; all stopped). The idea for Fomawrite: search those vaults from the editor, pull results in as links or quotes, and index the Markdown library itself the same way. Open questions: whether the vault MCP server is the integration point (likely) and how results become Markdown without breaking the file-is-truth rule.

## Decided

Moved here when a box becomes Yes or No, with the date and the reason.

- **Retire the Visual Edit pane** — Yes, 7 October 2026, Cycle 135. Live covers it with the Markdown text itself as the editing model.
- **Live follows the publishing theme, lighter (Option 3 via a CSS filter)** — Yes, 7 October 2026; design in [docs/live-appearance-design.md](docs/live-appearance-design.md). Text size shared with Source; Split kept; an exact-theme setting as the escape hatch.
- **Code in the Live dropdown drops back to Source silently** — Yes (keep as is), 7 October 2026. Code is a specific kind of editing; the silent switch works well in practice. A fuller code editor is a possible later idea, but the lightweight line-numbered appearance is enough for now.
