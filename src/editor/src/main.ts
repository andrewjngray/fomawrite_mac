// Bootstrap + bridge glue. Mounts the CM6 view in #editor, connects to the Qt bridge
// (or a mock) and wires signals/slots per the contract documented in README.md.
import { EditorView } from "@codemirror/view";
import { openSearchPanel } from "@codemirror/search";
import { selectAll } from "@codemirror/commands";
import { SAMPLE_DOCUMENT, connectBridge, onSignal } from "./bridge";
import { setDecorateMetricSink } from "./live";
import { blocksExtension } from "./blocks";
import { tablesExtension } from "./tables";
import { mathExtension } from "./math";
import { extrasExtension } from "./extras";
import { imagesExtension } from "./images";
import { Session, applyModeClass, applyThemeDom, Appearance } from "./modes";
import { appearanceExtension } from "./appearance";

declare global {
  interface Window {
    fomawriteEditor?: unknown;
  }
}

const CURSOR_DEBOUNCE_MS = 30;
const DECORATE_REPORT_INTERVAL_MS = 100;
const KEYSTROKE_WINDOW_MS = 1000;

async function main() {
  const { bridge, isMock } = await connectBridge();

  const safeLog = (msg: string) => {
    try {
      bridge.log(msg);
    } catch {
      /* ignore */
    }
  };
  const guard = <A extends unknown[]>(name: string, fn: (...a: A) => void) => (...a: A) => {
    try {
      fn(...a);
    } catch (e) {
      safeLog(`${name} failed: ${e instanceof Error ? e.message : String(e)}`);
    }
  };

  // ---- keystroke -> dispatch timing
  let lastInputAt = -1;
  const markInput = () => {
    lastInputAt = performance.now();
  };

  // ---- cursor reporting (debounced)
  let cursorTimer: ReturnType<typeof setTimeout> | null = null;
  const scheduleCursor = (view: EditorView) => {
    if (cursorTimer !== null) clearTimeout(cursorTimer);
    cursorTimer = setTimeout(() => {
      cursorTimer = null;
      const sel = view.state.selection.main;
      bridge.cursorChanged(sel.anchor, sel.head);
    }, CURSOR_DEBOUNCE_MS);
  };

  const session = new Session([
    appearanceExtension(),
    blocksExtension(bridge),
    tablesExtension(),
    mathExtension(),
    extrasExtension(),
    imagesExtension(bridge),
    EditorView.updateListener.of((u) => {
      if (u.docChanged) session.handleTransactions(u.transactions);
      if (u.selectionSet || u.docChanged) scheduleCursor(u.view);
    }),
  ]);

  const parent = document.getElementById("editor")!;
  const view = new EditorView({ state: session.createState(""), parent });
  session.attach(view);
  applyModeClass(session.mode);

  for (const ev of ["keydown", "beforeinput", "input", "compositionupdate"])
    view.dom.addEventListener(ev, markInput, { capture: true, passive: true });

  session.onDocChanged = (json, revision) => {
    bridge.documentChanged(json, revision);
    if (lastInputAt >= 0) {
      const dt = performance.now() - lastInputAt;
      lastInputAt = -1;
      if (dt < KEYSTROKE_WINDOW_MS) bridge.metric("keystroke-to-dispatch", dt);
    }
  };

  // ---- decorate metric (throttled: report the slowest sample per interval)
  let decMax = 0, decTimer: ReturnType<typeof setTimeout> | null = null;
  setDecorateMetricSink((ms) => {
    decMax = Math.max(decMax, ms);
    if (decTimer === null)
      decTimer = setTimeout(() => {
        decTimer = null;
        const v = decMax;
        decMax = 0;
        bridge.metric("decorate", v);
      }, DECORATE_REPORT_INTERVAL_MS);
  });

  // ---- host -> JS signals
  onSignal(bridge.setDocument, guard("setDocument", (text: string, revision: number) => {
    const t0 = performance.now();
    session.setDocument(text, revision);
    bridge.metric("setDocument", performance.now() - t0);
    bridge.cursorChanged(0, 0); // selection was reset; tell the host immediately
  }));
  onSignal(bridge.applyChanges, guard("applyChanges", (json: string, revision: number) => session.applyChanges(json, revision)));
  onSignal(bridge.setMode, guard("setMode", (mode: string) => session.setMode(mode)));
  onSignal(bridge.setTheme, guard("setTheme", (css: string) => applyThemeDom(css)));
  onSignal(bridge.setAppearance, guard("setAppearance", (json: string) => session.setAppearance(JSON.parse(json) as Appearance)));
  onSignal(bridge.focusEditor, guard("focusEditor", () => view.focus()));
  onSignal(bridge.requestText, guard("requestText", (token: number) => bridge.textReply(token, session.getText())));
  onSignal(bridge.simulateUserChanges, guard("simulateUserChanges", (json: string) => session.simulateUserChanges(json)));
  onSignal(bridge.undo, guard("undo", () => void session.undo()));
  // Scroll sync with the host's other panes: report the editor's vertical
  // position as a fraction, and follow the host's requests without echoing them.
  let applyingHostScroll = 0;
  let scrollTimer: ReturnType<typeof setTimeout> | undefined;
  const scrollFraction = () => {
    const el = view.scrollDOM;
    const range = el.scrollHeight - el.clientHeight;
    return range > 0 ? Math.max(0, Math.min(1, el.scrollTop / range)) : 0;
  };
  view.scrollDOM.addEventListener("scroll", () => {
    if (applyingHostScroll > 0) return;
    if (scrollTimer) clearTimeout(scrollTimer);
    scrollTimer = setTimeout(() => bridge.scrolled(scrollFraction()), 60);
  }, { passive: true });
  onSignal(bridge.command, guard("command", (name: string) => {
    switch (name) {
      case "find": case "replace": view.focus(); openSearchPanel(view); break;
      case "selectAll": view.focus(); selectAll(view); break;
      default: bridge.log("unknown command: " + name);
    }
  }));
  onSignal(bridge.setCursor, guard("setCursor", (pos: number) => {
    const at = Math.max(0, Math.min(view.state.doc.length, Math.floor(pos)));
    view.dispatch({ selection: { anchor: at }, scrollIntoView: true });
  }));
  onSignal(bridge.scrollToFraction, guard("scrollToFraction", (fraction: number) => {
    const el = view.scrollDOM;
    const range = el.scrollHeight - el.clientHeight;
    if (range <= 0) return;
    applyingHostScroll++;
    el.scrollTop = Math.max(0, Math.min(1, fraction)) * range;
    setTimeout(() => { applyingHostScroll = Math.max(0, applyingHostScroll - 1); }, 120);
  }));
  onSignal(bridge.redo, guard("redo", () => void session.redo()));

  window.fomawriteEditor = {
    getText: () => session.getText(),
    setText: (text: string) => session.setDocument(text, session.revision),
    setMode: (mode: string) => session.setMode(mode),
    getRevision: () => session.revision,
    getMode: () => session.mode,
    simulateUserChanges: (json: string) => session.simulateUserChanges(json),
    undo: () => session.undo(),
    redo: () => session.redo(),
    view,
  };

  if (isMock) {
    session.setDocument(SAMPLE_DOCUMENT, 0);
    session.setMode(new URLSearchParams(location.search).get("mode") === "source" ? "source" : "live");
  }

  bridge.ready();
}

main().catch((e) => console.error("fomawrite editor failed to start", e));
