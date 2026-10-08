// Source/Code mode presentation: the four writing appearances (manuscript, editorial, book, code),
// focus mode (dim every paragraph but the caret's) and typewriter mode (caret line kept centred).
//
// Everything is driven through the existing `setAppearance` JSON. `Session.setAppearance` (modes.ts)
// dispatches `appearanceEffect` with the parsed patch; the extension below keeps the resolved state in a
// StateField, adds/removes compartment contents in the *same* transaction (transactionExtender) and mirrors
// the state onto `#write` as classes. The pure helpers (parse/reduce/plan/paragraph ranges/recenter
// decision) have no DOM dependency and are unit-tested in plain node.
import {
  Compartment, EditorState, Extension, Prec, StateEffect, StateField, Text, Transaction,
} from "@codemirror/state";
import { Decoration, DecorationSet, EditorView, ViewPlugin, ViewUpdate, keymap, lineNumbers } from "@codemirror/view";
import { indentWithTab } from "@codemirror/commands";
import { indentUnit, syntaxTree } from "@codemirror/language";

// ---------------------------------------------------------------- pure model
export const APPEARANCE_NAMES = ["manuscript", "editorial", "book", "code"] as const;
export type AppearanceName = (typeof APPEARANCE_NAMES)[number];

/** Resolved presentation state. `appearance === null` means "never set": the page keeps its base look. */
export interface AppearanceState {
  appearance: AppearanceName | null;
  focus: boolean;
  typewriter: boolean;
  /** Source/Code presentation only applies in source mode; live mode keeps its own styling. */
  mode: "source" | "live";
}

export const INITIAL_APPEARANCE: AppearanceState = Object.freeze({
  appearance: null, focus: false, typewriter: false, mode: "source",
}) as AppearanceState;

/** Extract the presentation keys from a `setAppearance` payload. Unknown keys / bad values are ignored. */
export function parseAppearancePatch(raw: unknown): Partial<AppearanceState> {
  const out: Partial<AppearanceState> = {};
  if (typeof raw !== "object" || raw === null) return out;
  const o = raw as Record<string, unknown>;
  if (typeof o.appearance === "string") {
    const name = o.appearance.toLowerCase();
    if ((APPEARANCE_NAMES as readonly string[]).includes(name)) out.appearance = name as AppearanceName;
  }
  if (o.focus !== undefined) out.focus = !!o.focus;
  if (o.typewriter !== undefined) out.typewriter = !!o.typewriter;
  return out;
}

/** Merge a patch into the state; returns `prev` itself when nothing changes. */
export function reduceAppearance(prev: AppearanceState, patch: Partial<AppearanceState>): AppearanceState {
  const next = { ...prev };
  if (patch.appearance !== undefined) next.appearance = patch.appearance;
  if (patch.focus !== undefined) next.focus = patch.focus;
  if (patch.typewriter !== undefined) next.typewriter = patch.typewriter;
  if (patch.mode !== undefined) next.mode = patch.mode;
  return next.appearance === prev.appearance && next.focus === prev.focus
    && next.typewriter === prev.typewriter && next.mode === prev.mode ? prev : next;
}

export const APPEARANCE_CLASSES = APPEARANCE_NAMES.map((n) => `fw-appearance-${n}`);

/** Classes that should be present on `#write` for this state (everything else in the set is removed). */
export function appearanceClasses(s: AppearanceState): string[] {
  const out: string[] = [];
  if (s.appearance) out.push(`fw-appearance-${s.appearance}`);
  if (s.focus) out.push("fw-focus");
  if (s.typewriter) out.push("fw-typewriter");
  return out;
}

/** Classes for `#write` given the mode. Live ignores the Source appearances (Manuscript / Editorial / Book / Code
 *  have no meaning there): no `fw-appearance-*` class, while focus and typewriter still apply. Source: unchanged. */
export function effectiveAppearanceClasses(mode: "source" | "live", s: AppearanceState): string[] {
  return appearanceClasses(mode === "live" ? { ...s, appearance: null } : s);
}

/** What each compartment holds for a state. */
export interface Plan {
  wrap: boolean;
  code: boolean; // line numbers + indentation guides + Tab/Shift-Tab + 4-space indent unit
  hanging: boolean;
  focus: boolean;
}

export function planFor(s: AppearanceState): Plan {
  const source = s.mode === "source";
  return {
    wrap: !(source && s.appearance === "code"),
    code: source && s.appearance === "code",
    hanging: source && s.appearance === "manuscript",
    focus: s.focus,
  };
}

// ---------------------------------------------------------------- focus mode (paragraph ranges)
export interface LineSource {
  readonly lines: number;
  line(n: number): { text: string };
}

const BLANK = /^\s*$/;

/** Inclusive 1-based line range of the paragraph (run of non-blank lines) containing `lineNo`.
 *  A blank line is its own (empty) paragraph. */
export function paragraphRange(doc: LineSource, lineNo: number): [number, number] {
  if (BLANK.test(doc.line(lineNo).text)) return [lineNo, lineNo];
  let a = lineNo, b = lineNo;
  while (a > 1 && !BLANK.test(doc.line(a - 1).text)) a--;
  while (b < doc.lines && !BLANK.test(doc.line(b + 1).text)) b++;
  return [a, b];
}

/** Line ranges (inclusive, sorted, merged) that stay undimmed for the given selection ranges. */
export function focusActiveLines(doc: Text, ranges: readonly { from: number; to: number }[]): [number, number][] {
  const out: [number, number][] = [];
  for (const r of ranges) {
    const a = paragraphRange(doc, doc.lineAt(r.from).number)[0];
    const b = paragraphRange(doc, doc.lineAt(r.to).number)[1];
    out.push([a, b]);
  }
  out.sort((x, y) => x[0] - y[0]);
  const merged: [number, number][] = [];
  for (const r of out) {
    const last = merged[merged.length - 1];
    if (last && r[0] <= last[1] + 1) last[1] = Math.max(last[1], r[1]);
    else merged.push([r[0], r[1]]);
  }
  return merged;
}

// ---------------------------------------------------------------- typewriter decision
export interface RecenterInput {
  typewriter: boolean;
  /** typewriter was just switched on in this update */
  justEnabled: boolean;
  docChanged: boolean;
  selectionSet: boolean;
  /** the update came from typing/deleting/keyboard caret movement/undo (not host edits or pointer clicks) */
  userDriven: boolean;
  head: number;
  prevHead: number;
}

export function shouldRecenter(i: RecenterInput): boolean {
  if (!i.typewriter) return false;
  if (i.justEnabled) return true;
  if (!i.userDriven) return false;
  return i.docChanged || (i.selectionSet && i.head !== i.prevHead);
}

function isCaretDriven(tr: Transaction): boolean {
  if (tr.isUserEvent("select.pointer")) return false; // do not yank the page from under the mouse
  return tr.isUserEvent("select") || tr.isUserEvent("input") || tr.isUserEvent("delete")
    || tr.isUserEvent("move") || tr.isUserEvent("undo") || tr.isUserEvent("redo");
}

// ---------------------------------------------------------------- hanging markers (manuscript)
export const MARKER_TAB_SIZE = 4;

function leadingColumns(text: string, tabSize: number): { cols: number; chars: number } {
  let cols = 0, chars = 0;
  for (; chars < text.length; chars++) {
    const c = text[chars];
    if (c === " ") cols++;
    else if (c === "\t") cols += tabSize - (cols % tabSize);
    else break;
  }
  return { cols, chars };
}

/** For a line that starts with a heading / blockquote / list marker: `hang` = columns before the body
 *  text (indent + marker + following space), `lead` = columns of leading indentation (list nesting).
 *  Pure text analysis; the caller skips lines inside code blocks. */
export function hangForLine(text: string, tabSize = MARKER_TAB_SIZE): { hang: number; lead: number } | null {
  const ws = leadingColumns(text, tabSize);
  const rest = text.slice(ws.chars);
  let m = /^(#{1,6})([ \t]+|$)/.exec(rest);
  if (m && ws.cols < 4) return m[2] === "" ? null : { hang: ws.cols + m[1].length + m[2].length, lead: ws.cols };
  m = /^(?:>[ ]?)+/.exec(rest);
  if (m && ws.cols < 4) return { hang: ws.cols + m[0].length, lead: ws.cols };
  m = /^([-*+]|\d{1,9}[.)])([ \t]+)(?=\S)/.exec(rest);
  if (m) return { hang: ws.cols + m[1].length + m[2].length, lead: ws.cols };
  return null;
}

const hangDecoCache = new Map<string, Decoration>();
function hangDeco(hang: number, lead: number): Decoration {
  const key = hang + "," + lead;
  let d = hangDecoCache.get(key);
  if (!d) {
    d = Decoration.line({ attributes: { class: "fw-hang", style: `--fw-hang:${hang};--fw-lead:${lead}` } });
    hangDecoCache.set(key, d);
  }
  return d;
}

function inCodeBlock(state: EditorState, pos: number): boolean {
  for (let n: { name: string; parent: any } | null = syntaxTree(state).resolveInner(pos, 1); n; n = n.parent)
    if (n.name === "FencedCode" || n.name === "CodeBlock") return true;
  return false;
}

const hangingPlugin = ViewPlugin.fromClass(
  class {
    decorations: DecorationSet;
    constructor(view: EditorView) {
      this.decorations = this.build(view);
    }
    update(u: ViewUpdate) {
      if (u.docChanged || u.viewportChanged || syntaxTree(u.startState) !== syntaxTree(u.state))
        this.decorations = this.build(u.view);
    }
    build(view: EditorView): DecorationSet {
      const out = [];
      let last = -1;
      for (const { from, to } of view.visibleRanges) {
        for (let pos = from; pos <= to; ) {
          const l = view.state.doc.lineAt(pos);
          pos = l.to + 1;
          if (l.from <= last) continue;
          last = l.from;
          const h = hangForLine(l.text);
          if (h && !inCodeBlock(view.state, l.from)) out.push(hangDeco(h.hang, h.lead).range(l.from));
        }
      }
      return Decoration.set(out);
    }
  },
  { decorations: (v) => v.decorations },
);

// ---------------------------------------------------------------- indentation guides (code)
export const CODE_INDENT = 4;

/** Indent level of a line: leading columns (tabs expand to the next tab stop) / unit, rounded down. */
export function indentLevel(text: string, unit = CODE_INDENT, tabSize = CODE_INDENT): number {
  return Math.floor(leadingColumns(text, tabSize).cols / unit);
}

/** Guide count for line `n`. Blank lines continue the guides common to the nearest code lines around them. */
export function guideLevel(doc: LineSource, n: number, unit = CODE_INDENT, tabSize = CODE_INDENT): number {
  const text = doc.line(n).text;
  if (!BLANK.test(text)) return indentLevel(text, unit, tabSize);
  const SCAN = 40;
  let above = 0, below = 0;
  for (let i = n - 1; i >= 1 && i >= n - SCAN; i--) {
    const t = doc.line(i).text;
    if (!BLANK.test(t)) { above = indentLevel(t, unit, tabSize); break; }
  }
  for (let i = n + 1; i <= doc.lines && i <= n + SCAN; i++) {
    const t = doc.line(i).text;
    if (!BLANK.test(t)) { below = indentLevel(t, unit, tabSize); break; }
  }
  return Math.min(above, below);
}

const guideDecoCache: Decoration[] = [];
function guideDeco(levels: number): Decoration {
  return (guideDecoCache[levels] ??= Decoration.line({
    attributes: { class: "fw-guided", style: `--fw-guides:${levels}` },
  }));
}

const guidesPlugin = ViewPlugin.fromClass(
  class {
    decorations: DecorationSet;
    constructor(view: EditorView) {
      this.decorations = this.build(view);
    }
    update(u: ViewUpdate) {
      if (u.docChanged || u.viewportChanged) this.decorations = this.build(u.view);
    }
    build(view: EditorView): DecorationSet {
      const out = [];
      let last = -1;
      for (const { from, to } of view.visibleRanges) {
        for (let pos = from; pos <= to; ) {
          const l = view.state.doc.lineAt(pos);
          pos = l.to + 1;
          if (l.from <= last) continue;
          last = l.from;
          const n = guideLevel(view.state.doc, l.number);
          if (n > 0) out.push(guideDeco(n).range(l.from));
        }
      }
      return Decoration.set(out);
    }
  },
  { decorations: (v) => v.decorations },
);

// ---------------------------------------------------------------- focus decorations
const dimLine = Decoration.line({ class: "fw-dim" });
const focusPlugin = ViewPlugin.fromClass(
  class {
    decorations: DecorationSet;
    constructor(view: EditorView) {
      this.decorations = this.build(view);
    }
    update(u: ViewUpdate) {
      if (u.docChanged || u.selectionSet || u.viewportChanged) this.decorations = this.build(u.view);
    }
    build(view: EditorView): DecorationSet {
      const doc = view.state.doc;
      const active = focusActiveLines(doc, view.state.selection.ranges);
      const out = [];
      let last = -1;
      for (const { from, to } of view.visibleRanges) {
        for (let pos = from; pos <= to; ) {
          const l = doc.lineAt(pos);
          pos = l.to + 1;
          if (l.from <= last) continue;
          last = l.from;
          if (!active.some(([a, b]) => l.number >= a && l.number <= b)) out.push(dimLine.range(l.from));
        }
      }
      return Decoration.set(out);
    }
  },
  { decorations: (v) => v.decorations },
);

// ---------------------------------------------------------------- extension
/** Dispatched by `Session` (modes.ts) with the parsed `setAppearance` patch and by `setMode` with `{ mode }`. */
export const appearanceEffect = StateEffect.define<Partial<AppearanceState>>();

const appearanceField = StateField.define<AppearanceState>({
  create: () => INITIAL_APPEARANCE,
  update(value, tr) {
    for (const e of tr.effects) if (e.is(appearanceEffect)) value = reduceAppearance(value, e.value);
    return value;
  },
});

const wrapCompartment = new Compartment();
const codeCompartment = new Compartment();
const hangCompartment = new Compartment();
const focusCompartment = new Compartment();

const wrapExt = (on: boolean): Extension => (on ? EditorView.lineWrapping : []);
const codeExt = (on: boolean): Extension =>
  on
    ? [
        lineNumbers(),
        guidesPlugin,
        EditorState.tabSize.of(CODE_INDENT),
        indentUnit.of(" ".repeat(CODE_INDENT)),
        // Beats the list-aware Tab handler in modes.ts: code appearance always indents.
        Prec.highest(keymap.of([indentWithTab])),
      ]
    : [];
const hangExt = (on: boolean): Extension => (on ? hangingPlugin : []);
const focusExt = (on: boolean): Extension => (on ? focusPlugin : []);

/** Reconfigure the compartments in the same transaction that changes the resolved state. */
const reconfigurer = EditorState.transactionExtender.of((tr) => {
  const prev = tr.startState.field(appearanceField, false);
  if (!prev) return null;
  let next = prev;
  for (const e of tr.effects) if (e.is(appearanceEffect)) next = reduceAppearance(next, e.value);
  if (next === prev) return null;
  const a = planFor(prev), b = planFor(next);
  const effects: StateEffect<unknown>[] = [];
  if (a.wrap !== b.wrap) effects.push(wrapCompartment.reconfigure(wrapExt(b.wrap)));
  if (a.code !== b.code) effects.push(codeCompartment.reconfigure(codeExt(b.code)));
  if (a.hanging !== b.hanging) effects.push(hangCompartment.reconfigure(hangExt(b.hanging)));
  if (a.focus !== b.focus) effects.push(focusCompartment.reconfigure(focusExt(b.focus)));
  return effects.length ? { effects } : null;
});

function applyClasses(view: EditorView, s: AppearanceState) {
  const write = view.dom.closest?.("#write");
  if (!write) return;
  const want = new Set(effectiveAppearanceClasses(s.mode, s));
  for (const c of [...APPEARANCE_CLASSES, "fw-focus", "fw-typewriter"]) write.classList.toggle(c, want.has(c));
}

const hostPlugin = ViewPlugin.fromClass(
  class {
    pending = false;
    constructor(readonly view: EditorView) {
      applyClasses(view, view.state.field(appearanceField));
    }
    update(u: ViewUpdate) {
      const s = u.state.field(appearanceField), p = u.startState.field(appearanceField);
      if (s !== p) applyClasses(u.view, s);
      const head = u.state.selection.main.head;
      if (
        shouldRecenter({
          typewriter: s.typewriter,
          justEnabled: s.typewriter && !p.typewriter,
          docChanged: u.docChanged,
          selectionSet: u.selectionSet,
          userDriven: u.transactions.some(isCaretDriven),
          head,
          prevHead: u.startState.selection.main.head,
        })
      )
        this.recenter();
    }
    recenter() {
      if (this.pending) return;
      this.pending = true;
      // Cannot dispatch from inside an update.
      queueMicrotask(() => {
        this.pending = false;
        try {
          this.view.dispatch({
            effects: EditorView.scrollIntoView(this.view.state.selection.main.head, { y: "center" }),
          });
        } catch {
          /* view destroyed */
        }
      });
    }
  },
);

/** Register once in the Session's extra extensions (main.ts). */
export function appearanceExtension(): Extension {
  const init = planFor(INITIAL_APPEARANCE);
  return [
    appearanceField,
    reconfigurer,
    wrapCompartment.of(wrapExt(init.wrap)),
    codeCompartment.of(codeExt(init.code)),
    hangCompartment.of(hangExt(init.hanging)),
    focusCompartment.of(focusExt(init.focus)),
    hostPlugin,
  ];
}

/** Current resolved state (INITIAL when the extension is not installed). */
export function getAppearanceState(state: EditorState): AppearanceState {
  return state.field(appearanceField, false) ?? INITIAL_APPEARANCE;
}
