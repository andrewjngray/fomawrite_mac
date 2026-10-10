# Grammarly-like writing help in Fomawrite — plan of inquiry

Written 10 October 2026 after the Grammarly experiment. Andrew's requirement: Grammarly-quality spelling and grammar help inside Fomawrite, in both writing surfaces, switchable on and off, without being pushed into Grammarly's own editor. This note says what we know, what the candidate routes are, what each costs, and what to test in what order. Decisions are Andrew's; the boxes in [IDEAS.md](../IDEAS.md) record them.

## What we know

- **Grammarly Desktop attaches to nothing in Fomawrite** (build 141) although it works in Word on the same Mac. It reads other apps only through macOS Accessibility. Grammarly's own developer note confirms the bar for a native Mac app: use `NSTextView`, or make a custom view conform to the navigable-static-text accessibility protocol, post value and layout change notifications, and implement get and set of the selected text range.
- **Neither of our surfaces clears that bar as shipped.** A probe test showed Qt Quick's text item and Qt WebEngine's page both present an editable text area with text and cursor but return no character bounds and no point-to-offset answer. Both are upstream stubs in Qt. Record: [grammarly-experiment.md](../research/overnight-2026-10-08/grammarly-experiment.md), [cycle-142](../research/cycle-142/README.md).
- **Build 142 gives the Source editor real character bounds** through its own accessibility factory. Whether that is enough for Grammarly is untested; Grammarly may also need the change notifications its note lists. Live cannot be fixed from inside the app.
- **Grammarly's developer SDK is gone** (discontinued 10 January 2024). There is no supported way to put Grammarly's engine inside an app.
- **Apple's Writing Tools** reach custom views only through AppKit's coordinator object or `NSTextView` on TextKit 2. Qt Quick is neither, and Chromium web content is not supported. Not a route for us.
- **ProWritingAid's desktop app** works the same way Grammarly Desktop does, over accessibility, so it will fail the same way until the accessibility work is proven. **Slashit**-style hotkey rewriters are a different thing (rewrite on demand, no inline checking).
- **What already works today**, built in Cycle 143: one spell checker behind both surfaces, the macOS system checker, with a Check Spelling While Typing setting, a language, suggestions, learned and ignored words (`src/spellcheck.h`). Underlines in the document and right-click corrections are the current cycle.

## Routes, ranked

| | Route | Quality | Surfaces | Switchable | Cost to us | Status |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | Grammarly Desktop over the Source editor via accessibility | Grammarly's | Source only | Grammarly's own snooze or exclude | done if the hand test passes; otherwise a further accessibility step (notifications) | **hand test pending on build 142** |
| 2 | Our own checker pipeline, macOS spelling engine | spelling only; grammar weak | both | yes | Cycle 143, in progress | building |
| 3 | Same pipeline, **LanguageTool** engine | grammar and style, close to Grammarly for English; multi-language | both | yes | a `/v2/check` client (~200 lines), a local server or an API key; one cycle | not started |
| 4 | Same pipeline, a cloud API (Sapling, or LanguageTool cloud with Premium) | good; Premium improves 3 | both | yes | key in Keychain; network | not started |
| 5 | Same pipeline, an LLM review (Claude API) on demand per paragraph | best for style, too slow and costly per keystroke | both | yes | needs the AI-tools decision first | idea only |
| 6 | Replace the Source editor with a native `NSTextView` | Grammarly's | Source only | | rewrite of the Source editor; not proposed | no |

The pipeline in rows 2 to 5 is one design: each surface marks which stretches are prose, an engine returns issues with offsets, categories and replacements, the surface underlines them and offers the replacements on right-click, and a review pane lists them. Engines plug in behind one interface; the macOS checker is the first engine and the floor.

## Why LanguageTool is the one to test next

- Open HTTP API (`POST /v2/check` returns matches with offset, length, category, message, replacements), self-hostable (a Java server, Docker images exist), and a paid tier that improves the suggestions. The text can stay on the machine with the local server.
- It fits the pipeline with no change to the surfaces: same offsets, same underline and right-click code as spelling, plus a category colour.
- Pricing on the order of five euros a month for Premium; the open-source core costs nothing.
- Risks to check: quality on Andrew's actual prose versus Grammarly; memory of a local Java server; whether the cloud API's terms suit a personal desktop app.

## Experiments, in order

1. **Build 142 hand test (Andrew, two minutes).** Source mode, type a sentence with errors, watch for Grammarly's G. Pass: G appears and underlines. If it fails, the remaining accessibility step is value-changed notifications; one more bounded attempt, then stop.
2. **Cycle 143 (now).** Spelling underlines in Source and Live, right-click suggestions, Learn and Ignore, the Edit menu toggle. Pass: misspellings underline as you type in both surfaces and a right-click fixes one.
3. **LanguageTool spike (half a day, Fable).** Run the server locally, post three of Andrew's real paragraphs plus the test sentence, compare the matches with what Grammarly shows for the same text in Word. Record the comparison with screenshots. Pass: it catches the grammar Grammarly catches on those samples.
4. **Engine seam (one cycle).** Promote `SpellCheck` to a `WritingCheck` interface with categories, add the LanguageTool client behind it, colour by category, and replace the Spelling and Grammar dialog with a resizable review pane fed by the same issues.
5. **Decide the paid question.** Premium LanguageTool, a cloud key, or free local only.

## Decisions for Andrew

- If build 142 shows the G in Source, is Grammarly-in-Source-only worth keeping, given Live is the daily editor?
- Run a local LanguageTool server (a Java process on the Mac) or use their cloud API with a key?
- Pay for LanguageTool Premium if the free engine is close enough?
- Keep Grammarly for Word and Chrome, and let Fomawrite have its own checker?

## Sources

- Grammarly developer note on integrating with an application (now on the Superhuman help centre): https://help.superhuman.com/hc/en-us/articles/46242102540813-How-do-I-integrate-Grammarly-with-my-website-or-application
- Grammarly on accessibility access: https://support.grammarly.com/hc/en-us/articles/360059770451-Why-does-Grammarly-need-access-to-accessibility-features
- Text Editor SDK shutdown: https://techcrunch.com/2023/07/13/grammarly-to-shut-down-the-text-editor-sdk-in-january/
- Apple, Writing Tools for custom views: https://developer.apple.com/documentation/appkit/adding-writing-tools-support-to-a-custom-nsview
- LanguageTool self-hosting and `/v2/check` (third-party guides; verify against the project's own docs): https://railway.com/deploy/languagetool , https://www.hostinger.com/applications/languagetool
- Gemini answer Andrew pasted on 10 October 2026 (the overlay requirements and the alternatives table): treated as a lead, checked against the sources above.
