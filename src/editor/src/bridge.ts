// QWebChannel connection + mock fallback.
//
// Real mode: window.qt.webChannelTransport exists and window.QWebChannel is defined
// (qrc:///qtwebchannel/qwebchannel.js). Mock mode otherwise (plain browser / dev server).

export interface Signal<A extends unknown[]> {
  connect(fn: (...args: A) => void): void;
}

/** The `bridge` QObject as seen from JS. */
export interface Bridge {
  // JS -> C++ slots
  ready(): void;
  documentChanged(changesJson: string, revision: number): void;
  cursorChanged(anchor: number, head: number, byUser?: boolean): void;
  metric(name: string, ms: number): void;
  log(message: string): void;
  textReply(token: number, text: string): void;
  // Answer to requestSelection: the main selection at the moment of the request (UTF-16 offsets).
  selectionReply(token: number, anchor: number, head: number): void;
  requestImage(token: number, src: string): void;
  // Paste/drop of an image: the host writes the bytes next to the document and answers with imageSaved.
  saveImage(token: number, name: string, mime: string, base64: string): void;
  // Vertical scroll position as a 0..1 fraction (debounced), for pane sync.
  scrolled(fraction: number): void;
  // Spelling (live mode). Optional: a host without them simply gets no spelling. See README "Spelling".
  // segmentsJson = [{from, to, text}] prose segments in document offsets; answered by spellingReply(token, rangesJson).
  checkSpelling?(token: number, segmentsJson: string): void;
  // Suggestions for one word; answered by suggestionsReply(token, wordsJson).
  spellingSuggestions?(token: number, word: string): void;
  learnWord?(word: string): void;
  ignoreWord?(word: string): void;
  // Ignore Grammar Issue: the host keeps the one ignore list all surfaces read.
  ignoreGrammar?(text: string, message: string): void;
  // C++ -> JS signals
  setDocument: Signal<[string, number]>;
  applyChanges: Signal<[string, number]>;
  setMode: Signal<[string]>;
  setTheme: Signal<[string]>;
  setAppearance: Signal<[string]>;
  focusEditor: Signal<[]>;
  requestText: Signal<[number]>;
  // Host asks for the current selection; answer with selectionReply(token, anchor, head).
  requestSelection: Signal<[number]>;
  simulateUserChanges: Signal<[string]>;
  undo: Signal<[]>;
  redo: Signal<[]>;
  imageReply: Signal<[number, string, string]>;
  // Answer to saveImage: document-relative path on success (error ""), or a message on failure.
  imageSaved: Signal<[number, string, string]>;
  // Host asks the editor to scroll to a 0..1 fraction (not echoed back as scrolled()).
  scrollToFraction: Signal<[number]>;
  // Host places the caret (UTF-16 offset) and scrolls it into view.
  setCursor: Signal<[number]>;
  // Host-invoked editor commands: "find", "replace", "selectAll".
  command: Signal<[string]>;
  // Spelling answers and the host's setting (every setSpellCheck also means "check again").
  spellingReply?: Signal<[number, string]>;
  suggestionsReply?: Signal<[number, string]>;
  setSpellCheck?: Signal<[boolean]>;
  // Harper, the host's lint service (README "Harper"). Optional: a host without them never asks.
  harperReady?(version: string): void;
  harperReply?(token: number, lintsJson: string): void;
  harperFailed?(token: number, error: string): void;
  harperLoad?: Signal<[string]>;
  harperLint?: Signal<[number, string]>;
  harperImportWords?: Signal<[string]>;
}

export interface BridgeConnection {
  bridge: Bridge;
  isMock: boolean;
}

export const SAMPLE_DOCUMENT = `# Fomawrite live preview

This is a **bold** statement, an *italic* aside, some \`inline code\`, ~~struck~~ text,
and a [link to somewhere](https://example.com/path "title").

## A list and a quote

- first item with **nested *emphasis***
- second item
  - nested item

> Quoted text with a *twist*.

1. one
2. two

### Code

\`\`\`js
const answer = 42;
\`\`\`

### Blocks

- [ ] an open task
- [x] a finished task

---

![a tiny image](example.png "title")

| Name | Value |
| ---- | ----- |
| one  | 1     |

> outer quote
> > nested quote

\`\`\`ts
const greet = (name: string) => \`hello \${name}\`;
\`\`\`

Move the cursor into any styled span to reveal its markers.
`;

const MOCK_PNG =
  "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==";

const SIGNALS = [
  "setDocument", "applyChanges", "setMode", "setTheme", "setAppearance",
  "focusEditor", "requestText", "requestSelection", "simulateUserChanges", "undo", "redo", "imageReply", "imageSaved", "scrollToFraction", "setCursor", "command",
  "spellingReply", "suggestionsReply", "setSpellCheck", "harperLoad", "harperLint", "harperImportWords"
] as const;

/**
 * The mock host's stand-in for Harper's findings (dev server and node tests, no WebAssembly): "recieve" (Spelling), a
 * doubled word (Repetition) and "very unique" (Enhancement, which the host files under Style). UTF-16 offsets into `text`.
 */
export function mockHarperLint(text: string): { start: number; end: number; kind: string; message: string; suggestions: string[] }[] {
  const out: { start: number; end: number; kind: string; message: string; suggestions: string[] }[] = [];
  for (const m of text.matchAll(/\brecieve\b/gi))
    out.push({ start: m.index!, end: m.index! + m[0].length, kind: "Spelling", message: "Did you mean to spell \"" + m[0] + "\" this way?", suggestions: ["receive"] });
  for (const m of text.matchAll(/\b([\p{L}']+) \1\b/giu))
    out.push({ start: m.index!, end: m.index! + m[0].length, kind: "Repetition", message: "Did you mean to repeat this word?", suggestions: [m[1]] });
  for (const m of text.matchAll(/\bvery unique\b/gi))
    out.push({ start: m.index!, end: m.index! + m[0].length, kind: "Enhancement", message: "\"Unique\" is absolute; \"very\" adds nothing.", suggestions: ["unique"] });
  return out.sort((a, b) => a.start - b.start);
}

export type MockBridge = Bridge & {
  /** Fire a signal from the "C++ side" (for manual testing in a browser). */
  emit(signal: (typeof SIGNALS)[number], ...args: unknown[]): void;
  calls: { name: string; args: unknown[] }[];
};

export function createMockBridge(): MockBridge {
  const handlers: Record<string, ((...a: any[]) => void)[]> = {};
  const calls: { name: string; args: unknown[] }[] = [];
  const mock: any = { calls };
  const slot = (name: string) => (...args: unknown[]) => {
    calls.push({ name, args });
    console.log("[bridge mock]", name, ...args);
  };
  for (const s of ["ready", "documentChanged", "cursorChanged", "metric", "log", "textReply", "selectionReply", "scrolled", "harperReady", "harperReply", "harperFailed"]) mock[s] = slot(s);
  for (const s of SIGNALS) {
    handlers[s] = [];
    mock[s] = { connect: (fn: (...a: any[]) => void) => handlers[s].push(fn) };
  }
  // Host stand-in: resolve every image to a 1x1 PNG after 50 ms.
  mock.requestImage = (token: number, src: string) => {
    slot("requestImage")(token, src);
    setTimeout(() => mock.emit("imageReply", token, MOCK_PNG, ""), 50);
  };
  // Host stand-in: "save" every pasted/dropped image to assets/<name> after 50 ms.
  mock.saveImage = (token: number, name: string, mime: string, base64: string) => {
    calls.push({ name: "saveImage", args: [token, name, mime, base64] });
    console.log("[bridge mock] saveImage", token, name, mime, `<${base64.length} base64 chars>`);
    setTimeout(() => mock.emit("imageSaved", token, "assets/" + name, ""), 50);
  };
  // Host stand-in for spelling and grammar: a tiny word list and a doubled-word rule instead of the system checker, answered after 20 ms.
  const MOCK_MISSPELLED: Record<string, string[]> = {
    tomorow: ["tomorrow"], teh: ["the", "tea"], recieve: ["receive"], mispelled: ["misspelled"], wrold: ["world", "word"],
  };
  mock.checkSpelling = (token: number, segmentsJson: string) => {
    calls.push({ name: "checkSpelling", args: [token, segmentsJson] });
    console.log("[bridge mock] checkSpelling", token, segmentsJson.length, "chars");
    const ranges: { from: number; to: number; category: string; message?: string; suggestions?: string[] }[] = [];
    try {
      for (const seg of JSON.parse(segmentsJson) as { from: number; text: string }[]) {
        for (const m of seg.text.matchAll(/[\p{L}']+/gu))
          if (MOCK_MISSPELLED[m[0].toLowerCase()]) ranges.push({ from: seg.from + m.index!, to: seg.from + m.index! + m[0].length, category: "Spelling" });
        // Grammar stand-in: a word repeated right after itself ("is is").
        for (const m of seg.text.matchAll(/\b([\p{L}']+) \1\b/giu))
          ranges.push({ from: seg.from + m.index!, to: seg.from + m.index! + m[0].length, category: "Grammar", message: "Repeated word", suggestions: [m[1]] });
      }
    } catch { /* malformed: answer with nothing */ }
    ranges.sort((a, b) => a.from - b.from);
    setTimeout(() => mock.emit("spellingReply", token, JSON.stringify(ranges)), 20);
  };
  mock.spellingSuggestions = (token: number, word: string) => {
    calls.push({ name: "spellingSuggestions", args: [token, word] });
    setTimeout(() => mock.emit("suggestionsReply", token, JSON.stringify(MOCK_MISSPELLED[word.toLowerCase()] ?? [])), 20);
  };
  for (const s of ["learnWord", "ignoreWord", "ignoreGrammar"]) mock[s] = slot(s);
  mock.emit = (signal: string, ...args: unknown[]) => handlers[signal]?.forEach((f) => f(...args));
  return mock as MockBridge;
}

declare global {
  interface Window {
    QWebChannel?: new (transport: unknown, cb: (channel: { objects: Record<string, unknown> }) => void) => unknown;
    qt?: { webChannelTransport?: unknown };
  }
}

/** Connect to the Qt side, or fall back to the mock. Never rejects. */
export function connectBridge(): Promise<BridgeConnection> {
  const transport = window.qt?.webChannelTransport;
  if (transport && typeof window.QWebChannel === "function") {
    return new Promise((resolve) => {
      try {
        new window.QWebChannel!(transport, (channel) => {
          const obj = channel.objects.bridge as Bridge | undefined;
          if (obj) resolve({ bridge: obj, isMock: false });
          else {
            console.warn("QWebChannel connected but no `bridge` object; using mock");
            resolve({ bridge: createMockBridge(), isMock: true });
          }
        });
      } catch (e) {
        console.warn("QWebChannel failed; using mock", e);
        resolve({ bridge: createMockBridge(), isMock: true });
      }
    });
  }
  return Promise.resolve({ bridge: createMockBridge(), isMock: true });
}

/** Connect a signal if the C++ side actually exposes it (older hosts may lack newer signals). */
export function onSignal<A extends unknown[]>(sig: Signal<A> | undefined, fn: (...args: A) => void): void {
  sig?.connect?.(fn);
}
