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
  cursorChanged(anchor: number, head: number): void;
  metric(name: string, ms: number): void;
  log(message: string): void;
  textReply(token: number, text: string): void;
  // C++ -> JS signals
  setDocument: Signal<[string, number]>;
  applyChanges: Signal<[string, number]>;
  setMode: Signal<[string]>;
  setTheme: Signal<[string]>;
  setAppearance: Signal<[string]>;
  focusEditor: Signal<[]>;
  requestText: Signal<[number]>;
  simulateUserChanges: Signal<[string]>;
  undo: Signal<[]>;
  redo: Signal<[]>;
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

Move the cursor into any styled span to reveal its markers.
`;

const SIGNALS = [
  "setDocument", "applyChanges", "setMode", "setTheme", "setAppearance",
  "focusEditor", "requestText", "simulateUserChanges", "undo", "redo",
] as const;

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
  for (const s of ["ready", "documentChanged", "cursorChanged", "metric", "log", "textReply"]) mock[s] = slot(s);
  for (const s of SIGNALS) {
    handlers[s] = [];
    mock[s] = { connect: (fn: (...a: any[]) => void) => handlers[s].push(fn) };
  }
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
