// Editor extensions, source/live mode switching, appearance and theme injection,
// plus the `Session` that owns mode + revision and is driven by the bridge glue (main.ts).
// Nothing here touches the DOM at import time, so it can be unit-tested in plain node.
import {
  Annotation, Compartment, EditorState, Extension, Prec, Transaction, TransactionSpec,
} from "@codemirror/state";
import { EditorView, drawSelection, keymap } from "@codemirror/view";
import { defaultKeymap, history, historyKeymap, undo as cmUndo, redo as cmRedo } from "@codemirror/commands";
import { HighlightStyle, syntaxHighlighting, syntaxTree } from "@codemirror/language";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { searchKeymap, search } from "@codemirror/search";
import { Tag, styleTags, tags as t } from "@lezer/highlight";
import { changesToJson, jsonToChangeSpecs } from "./changes";
import { liveExtension } from "./live";
import { appearanceEffect, parseAppearancePatch, AppearanceState } from "./appearance";
import { fenceLanguages } from "./blocks";
import { mathMarkdown } from "./math";
import { extrasMarkdown } from "./extras";
import { HOST_SIZE_CLASS, LIVE_OVERLAY_ID, MAPPED_ID, THEME_ID, LiveThemeState, applyLiveTheme, parseLiveThemePatch } from "./livetheme";

export type Mode = "source" | "live";

export interface Appearance {
  fontFamily?: string;
  fontSize?: number;
  lineHeight?: number;
  dark?: boolean;
  typewriter?: boolean;
  focus?: boolean;
  /** "manuscript" | "editorial" | "book" | "code" - see appearance.ts */
  appearance?: string;
  /** Live mode only: apply the editing overlay over the theme (default true). false = theme exactly. */
  liveThemeFilter?: boolean;
  /** Live mode only: host colours used when the theme sets no background / text colour on #write. */
  palette?: { background: string; text: string };
}

/** Marks transactions that come from the host (setDocument/applyChanges/mode) so they are not echoed back. */
export const External = Annotation.define<boolean>();

// ---------------------------------------------------------------- source-mode highlighting
// `t.list` tags the whole list, not just the marker; give ListMark its own tag so `.fw-list-mark` is only the "-"/"1."
const listMarkTag = Tag.define();

export const fwHighlight = HighlightStyle.define([
  { tag: [t.heading1, t.heading2, t.heading3, t.heading4, t.heading5, t.heading6], class: "fw-heading" },
  { tag: t.emphasis, class: "fw-emphasis" },
  { tag: t.strong, class: "fw-strong" },
  { tag: t.monospace, class: "fw-code" },
  { tag: [t.link, t.url], class: "fw-link" },
  { tag: t.quote, class: "fw-quote" },
  { tag: listMarkTag, class: "fw-list-mark" },
  { tag: t.strikethrough, class: "fw-strike" },
  { tag: t.processingInstruction, class: "fw-mark" },
]);

// ---------------------------------------------------------------- tab handling in lists
function inListItem(state: EditorState, pos: number): boolean {
  for (let n: any = syntaxTree(state).resolveInner(pos, -1); n; n = n.parent) if (n.name === "ListItem") return true;
  return false;
}

function selectedLineStarts(state: EditorState): number[] {
  const starts = new Set<number>();
  for (const r of state.selection.ranges) {
    let l = state.doc.lineAt(r.from);
    for (;;) {
      starts.add(l.from);
      if (l.to >= r.to || l.number >= state.doc.lines) break;
      l = state.doc.line(l.number + 1);
    }
  }
  return [...starts].sort((a, b) => a - b);
}

export function listIndent(view: { state: EditorState; dispatch: (s: TransactionSpec) => void }): boolean {
  const { state } = view;
  if (!state.selection.ranges.some((r) => inListItem(state, r.head))) return false;
  view.dispatch({
    changes: selectedLineStarts(state).map((from) => ({ from, insert: "  " })),
    userEvent: "input.indent",
  });
  return true;
}

export function listOutdent(view: { state: EditorState; dispatch: (s: TransactionSpec) => void }): boolean {
  const { state } = view;
  if (!state.selection.ranges.some((r) => inListItem(state, r.head))) return false;
  const changes: { from: number; to: number }[] = [];
  for (const from of selectedLineStarts(state)) {
    const text = state.doc.lineAt(from).text;
    const n = text.startsWith("  ") ? 2 : text.startsWith(" ") ? 1 : 0;
    if (n) changes.push({ from, to: from + n });
  }
  if (changes.length) view.dispatch({ changes, userEvent: "delete.dedent" });
  return true;
}

// ---------------------------------------------------------------- extensions
const modeCompartment = new Compartment();

function modeExtension(mode: Mode): Extension {
  return mode === "live" ? liveExtension : syntaxHighlighting(fwHighlight);
}

export function baseExtensions(): Extension[] {
  return [
    history(),
    drawSelection(),
    search({ top: true }),
    // GFM (Table, TaskList, Strikethrough, Autolink) is already part of markdownLanguage's parser.
    markdown({ base: markdownLanguage, codeLanguages: fenceLanguages, extensions: [{ props: [styleTags({ ListMark: listMarkTag })] }, mathMarkdown, extrasMarkdown] }),
    // Line wrapping lives in appearance.ts (off in Code appearance).
    EditorView.contentAttributes.of({ spellcheck: "true", autocorrect: "off" }),
    Prec.high(
      keymap.of([
        { key: "Tab", run: listIndent },
        { key: "Shift-Tab", run: listOutdent },
      ]),
    ),
    keymap.of([...searchKeymap, ...historyKeymap, ...defaultKeymap]),
  ];
}

// ---------------------------------------------------------------- DOM helpers (guarded)
export function applyModeClass(mode: Mode, doc: Document | undefined = typeof document !== "undefined" ? document : undefined) {
  const write = doc?.getElementById("write");
  if (!write) return;
  write.classList.toggle("fw-mode-live", mode === "live");
  write.classList.toggle("fw-mode-source", mode === "source");
}

export function applyAppearanceDom(a: Appearance, doc: Document | undefined = typeof document !== "undefined" ? document : undefined) {
  if (!doc) return;
  const write = doc.getElementById("write");
  if (write) {
    const set = (name: string, v: string | null) =>
      v === null ? write.style.removeProperty(name) : write.style.setProperty(name, v);
    if (a.fontFamily !== undefined) set("--fw-font", a.fontFamily || null);
    if (typeof a.fontSize === "number") {
      set("--fw-font-size", a.fontSize + "px");
      write.classList.add(HOST_SIZE_CLASS); // lets the host size beat a theme's `#write { font-size }` in filtered Live
    }
    if (typeof a.lineHeight === "number") set("--fw-line-height", String(a.lineHeight));
  }
  if (a.dark !== undefined) doc.documentElement.classList.toggle("dark", !!a.dark);
}

export function applyThemeDom(css: string, doc: Document | undefined = typeof document !== "undefined" ? document : undefined) {
  if (!doc) return;
  let el = doc.getElementById(THEME_ID);
  if (!css) {
    el?.remove();
    return;
  }
  if (!el) {
    el = doc.createElement("style");
    el.id = THEME_ID;
    // Order in <head>: theme, mapped theme, live overlay (later wins on precedence).
    const after = doc.getElementById(MAPPED_ID) ?? doc.getElementById(LIVE_OVERLAY_ID);
    if (after) doc.head.insertBefore(el, after);
    else doc.head.appendChild(el);
  }
  el.textContent = css;
}

// ---------------------------------------------------------------- session
/** The subset of EditorView the session needs (lets tests use a fake without a DOM). */
export interface ViewLike {
  state: EditorState;
  dispatch(...specs: (TransactionSpec | Transaction)[]): void;
  setState(state: EditorState): void;
}

export class Session {
  revision = 0;
  mode: Mode = "source";
  /** Legacy font/colour keys only. Focus, typewriter and the named appearance are owned by appearance.ts
   *  (so `appearance.typewriter` is never set here; `presentation` holds the merged patch). */
  appearance: Appearance = {};
  private presentation: Partial<AppearanceState> = {};
  /** Live-theme filter state (livetheme.ts); the overlay is on unless the host sends `liveThemeFilter: false`. */
  private liveTheme: Omit<LiveThemeState, "mode"> = { filter: true };
  /** Called for every user (non-External) doc-changing transaction. */
  onDocChanged: ((changesJson: string, revision: number) => void) | null = null;
  private view!: ViewLike;

  constructor(private extra: Extension[] = []) {}

  attach(view: ViewLike) {
    this.view = view;
  }

  createState(doc: string): EditorState {
    return EditorState.create({
      doc,
      extensions: [
        baseExtensions(),
        modeCompartment.of(modeExtension(this.mode)),
        this.extra,
      ],
    });
  }

  getMode(): Mode {
    return this.mode;
  }

  getText(): string {
    return this.view.state.doc.toString();
  }

  /** Replace the whole document; resets undo history, selection and the revision counter. */
  setDocument(text: string, revision: number) {
    this.view.setState(this.createState(text));
    this.revision = revision;
    // A fresh state starts with default presentation; put the current one back (same tick, no repaint between).
    this.view.dispatch({ effects: appearanceEffect.of({ ...this.presentation, mode: this.mode }), annotations: External.of(true) });
  }

  /** Apply host-side changes without echoing them back. Throws on malformed/out-of-range input. */
  applyChanges(json: string, revision: number) {
    const specs = jsonToChangeSpecs(json, this.view.state.doc.length);
    this.view.dispatch({ changes: specs, annotations: External.of(true), addToHistory: false } as TransactionSpec);
    this.revision = revision;
  }

  /** Apply changes as an ordinary user transaction (reported via documentChanged). */
  simulateUserChanges(json: string) {
    const specs = jsonToChangeSpecs(json, this.view.state.doc.length);
    this.view.dispatch({ changes: specs, userEvent: "input" });
  }

  private target() {
    // CM6 commands destructure `dispatch`, so hand them a bound one.
    return { state: this.view.state, dispatch: (tr: Transaction) => this.view.dispatch(tr) };
  }

  undo(): boolean {
    return cmUndo(this.target());
  }
  redo(): boolean {
    return cmRedo(this.target());
  }

  setMode(mode: string) {
    if (mode !== "source" && mode !== "live") throw new Error("unknown mode: " + mode);
    if (mode === this.mode) return;
    this.mode = mode;
    this.view.dispatch({
      effects: [modeCompartment.reconfigure(modeExtension(mode)), appearanceEffect.of({ mode })],
      annotations: External.of(true),
    });
    applyModeClass(mode);
    this.syncLiveTheme();
  }

  /** Re-apply the live overlay / readability fill; call after the theme or mode changes. No-op without a DOM. */
  syncLiveTheme(doc: Document | undefined = typeof document !== "undefined" ? document : undefined) {
    applyLiveTheme({ ...this.liveTheme, mode: this.mode }, doc);
  }

  setAppearance(a: Appearance) {
    const { focus: _f, typewriter: _t, appearance: _a, liveThemeFilter: _l, palette: _p, ...legacy } = a;
    this.appearance = { ...this.appearance, ...legacy };
    applyAppearanceDom(a);
    const live = parseLiveThemePatch(a);
    if (live.liveThemeFilter !== undefined) this.liveTheme.filter = live.liveThemeFilter;
    if (live.palette) this.liveTheme.palette = live.palette;
    this.syncLiveTheme();
    const patch = parseAppearancePatch(a);
    this.presentation = { ...this.presentation, ...patch };
    if (Object.keys(patch).length)
      this.view.dispatch({ effects: appearanceEffect.of(patch), annotations: External.of(true) });
  }

  /** Feed transactions from the view's update listener; reports user doc changes. */
  handleTransactions(trs: readonly Transaction[]) {
    for (const tr of trs) {
      if (!tr.docChanged || tr.annotation(External)) continue;
      this.revision++;
      this.onDocChanged?.(changesToJson(tr.changes), this.revision);
    }
  }
}
