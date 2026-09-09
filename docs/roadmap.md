# Omawrite Mac: personal Markdown workspace

Owner: Andrew Gray. Working name: Omawrite Mac. Starting point: the MIT-licensed omacom/omawrite project; preserve its history and attribution.

## Product direction

Make reading and writing local Markdown pleasant enough to use every day. Take the calm typography and focused writing experience Andrew likes in iA Writer, and the direct Markdown editing experience he likes in Typora. Add capabilities when they address a real workflow. These are design goals, not a claim of feature parity or shared proprietary code.

## Milestones

1. **Mac foundation:** build an app bundle; native menus and file dialogs; Command shortcuts; Finder document opening; system appearance; existing save, recovery and external-change behaviour. Add a reproducible development workflow.
2. **Daily writing:** folder navigation and quick switching, adjustable typography, focus mode, recent documents. Choose the first addition through actual use.
3. **Richer documents:** evaluate tables, local images, task lists, source/reading modes and export against real example documents. Establish Markdown round-trip fixtures before changing parsing or rendering.
4. **Personal workflows:** document templates, search across folders, links between notes and user-defined commands. Add integrations only when needed.
5. **Distribution:** icon and naming, accessibility audit, multi-document lifecycle polish, signing/notarisation and automated releases if Andrew wants other people to use it.

## Learning loop

Choose one concrete inconvenience. Describe expected behaviour with an example. Read the small part of the code involved, implement, verify, and use it. Commit each useful increment. Keep short decision notes that explain why the code changed.

## Architectural choices

Retain Qt Quick/QML and C++ to reuse Omawrite's working editor and stay close to upstream. QML controls the interface; C++ owns document I/O, recovery and Markdown formatting. macOS has a separate appearance implementation, with Linux's portal implementation retained. Plain UTF-8 Markdown remains the storage format.

The initial port uses one process per window, inherited from upstream. This simplifies recovery isolation but is not yet a full native multi-document app lifecycle. A future change should consolidate window ownership and coordinated quitting if that becomes important.
