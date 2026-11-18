# Fomawrite — mission

Owner: Andrew Gray. This file is the one place the mission is stated. Edit it freely; the other documents point here rather than restating it. Every cycle, feature request and review is judged against it.

## The mission

A personal Markdown writing app for macOS in which plain `.md` files are the only truth.

- **The file is the truth.** Documents are saved byte-exactly, with no hidden state, no proprietary format and nothing the file cannot carry. Anything another Markdown tool wrote can be opened, edited and saved without being rewritten.
- **One text, several ways to work on it.** The same document can be edited as literal source (iA Writer style, including code), edited with its Markdown rendered in place (Typora style), or read as the published result. Switching between them is instant and loses nothing: not the caret, not the selection, not a byte.
- **Publishing is first class.** Typora CSS themes, real Web output and real PDF, previewed as they will export.
- **PDFs are read beside the writing.** Fomawrite writes Markdown. It also opens PDFs so that writing about them happens in one place: reading, highlighting and noting a PDF produces Markdown beside it, never hidden state. Fomawrite changes a PDF's own bytes only when Andrew explicitly asks for a page operation or an annotated export, and always through an atomic save with the previous version kept for recovery. (Agreed 19 November 2026; plan: [docs/pdf-plan.md](docs/pdf-plan.md).)
- **Calm.** Typography and focus come before chrome. Nothing interrupts writing.
- **Small enough to trust.** This is a one-person product built with AI help. It must stay small enough that its behaviour can be verified end to end, and every change ships with a test that would have failed before it.

## What "advancing the project" means right now

Not more features. The current measure is a single question: **is Live good enough to be the daily editor?** Advancing means finding that out by writing real documents in it, fixing what that reveals, and only then deciding what else the mission needs. A cycle that adds capability without moving that question forward is not progress.

When that question is answered the measure changes, and this section changes with it.

Agreed exception, 19 November 2026: the PDF plan ([docs/pdf-plan.md](docs/pdf-plan.md)) proceeds phase by phase alongside this measure; each phase still ships as verified cycles and must not regress Markdown writing.

## Where the mission is applied

- [docs/roadmap.md](docs/roadmap.md) — the sequence of cycles, each tied to the measure above.
- [AGENTS.md](AGENTS.md) — the working rules that keep cycles honest.
- [STATUS.md](STATUS.md) — the current build and what is open against the measure.
- [ARCHITECTURE.md](ARCHITECTURE.md) — how the shape of the code serves the mission.
- [IDEAS.md](IDEAS.md) — ideas for next steps with a Yes / No / Maybe decision on each; larger concepts such as [roam_concept.md](roam_concept.md) hang off it.
