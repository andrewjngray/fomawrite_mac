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
Yes [ ]  No [ ]  Maybe [x]

Source mode uses the native Qt text editor and its spell check; the Live page is Chromium and has none today. Qt WebEngine can spell check but needs bundled `.bdic` dictionaries, which Homebrew Qt does not ship. Options: bundle dictionaries for the languages Andrew writes in, or rely on an external checker (see Grammarly below).

### Grammarly in the editor
Yes [ ]  No [ ]  Maybe [x]

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

### Authorship marks visible in Live
Yes [ ]  No [ ]  Maybe [x]

Authorship ranges are format-only `QTextDocument` edits and are deliberately not pushed to the page, so they are invisible in Live.

## Concepts beyond the editor

Bigger ideas where Fomawrite is the human side of a larger system. Each gets its own document when it is worked on.

### Roam Research as the home of distilled thought
Yes [ ]  No [ ]  Maybe [x]

Andrew uses Roam for his most considered, distilled notes and for daily notes, while Markdown files and AI tools generate far more material than that. The concept: Fomawrite is the human-to-machine interface for writing; Roam is where the small, precious, settled things live; the project becomes a harness between them. Worked out in [roam_concept.md](roam_concept.md).

### An AI workbench pane
Yes [ ]  No [ ]  Maybe [x]

A tab or left-hand pane from which Andrew can open and move between the AI tools he uses (Claude, Claude Code, Codex, ChatGPT, Cursor, Grok) with the current document as context, instead of switching apps and re-pasting. Open questions: which of these have APIs or deep links that can carry a document in and bring text back; whether this is a launcher (easy), a chat surface (medium) or a round-trip editor (hard, and it touches the "file is the truth" rule, because AI output must come back as plain Markdown edits).

### Vaults: deep search over a lifetime of data
Yes [ ]  No [ ]  Maybe [x]

The semantic indexing work with Rami: `potentiacap/vaultmcp` (document vault with a built-in MCP server, based on the Recoll indexing code), `potentiacap/super_email_search_mcp` and `potentiacap/pst-agent` for Outlook PST/OST archives going back about thirty years. Four vaults are registered on this machine today (a home folder, a PST archive and two legal matters; all stopped). The idea for Fomawrite: search those vaults from the editor, pull results in as links or quotes, and index the Markdown library itself the same way. Open questions: whether the vault MCP server is the integration point (likely) and how results become Markdown without breaking the file-is-truth rule.

## Decided

Moved here when a box becomes Yes or No, with the date and the reason.

- **Retire the Visual Edit pane** — Yes, 7 October 2026, Cycle 135. Live covers it with the Markdown text itself as the editing model.
