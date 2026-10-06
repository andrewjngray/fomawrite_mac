// Math (Typora convention): `$tex$` inline and `$$ ... $$` blocks, rendered with KaTeX in Live mode.
//
// Three parts, all in this file:
//   1. `mathMarkdown`  - a @lezer/markdown extension adding InlineMath / MathBlock (+ MathMark delimiters).
//   2. `buildMath`     - pure decoration builder (same shape as blocks.ts): when no selection range touches a
//                        node, InlineMath becomes an inline KaTeX widget and MathBlock a display widget on its
//                        first line with the remaining lines collapsed; otherwise the raw TeX stays visible.
//   3. `mathExtension` - ViewPlugin + atomic ranges + `.fw-math` highlighting (visible in Source mode too).
//
// KaTeX's CSS and fonts are copied to dist/ by esbuild.mjs (dist/katex.css, dist/fonts/*.woff2).
import { EditorSelection, EditorState, Extension, Range } from "@codemirror/state";
import { Decoration, DecorationSet, EditorView, ViewPlugin, ViewUpdate, WidgetType } from "@codemirror/view";
import { HighlightStyle, ensureSyntaxTree, syntaxHighlighting, syntaxTree } from "@codemirror/language";
import type { BlockContext, InlineContext, Line, MarkdownConfig } from "@lezer/markdown";
import { IterMode } from "@lezer/common";
import type { Input } from "@lezer/common";
import { Tag, tags as t } from "@lezer/highlight";
import katex from "katex";
import { liveExtension } from "./live";
import type { TextRange } from "./live";

// ---------------------------------------------------------------- parsing
const DOLLAR = 36, BACKSLASH = 92, NEWLINE = 10;
const isSpace = (c: number) => c === 32 || c === 9 || c === 10 || c === 13;
const isDigit = (c: number) => c >= 48 && c <= 57;

/**
 * Inline math `$tex$`. Rules (Pandoc's, plus "no newline"):
 *  - the opening `$` is not followed by whitespace, and neither side of it is another `$`;
 *  - the first unescaped `$` after it must close: it follows a non-space character and is not followed by a digit,
 *    otherwise the opening `$` is plain text (so "costs $5 and $10" stays text);
 *  - no line break inside; `\$` is an escape and never closes.
 */
function parseInlineMath(cx: InlineContext, next: number, pos: number): number {
  if (next !== DOLLAR) return -1;
  const after = cx.char(pos + 1);
  if (after < 0 || after === DOLLAR || isSpace(after) || cx.char(pos - 1) === DOLLAR) return -1;
  for (let i = pos + 1; ; i++) {
    const c = cx.char(i);
    if (c < 0 || c === NEWLINE) return -1;
    if (c === BACKSLASH) {
      i++; // skip the escaped character (it may be `$` or a line break; the loop re-checks the one after)
      if (cx.char(i) === NEWLINE) return -1;
      continue;
    }
    if (c !== DOLLAR) continue;
    // First unescaped `$`: closer or give up.
    if (isSpace(cx.char(i - 1)) || isDigit(cx.char(i + 1))) return -1;
    const end = i + 1;
    return cx.addElement(
      cx.elt("InlineMath", pos, end, [cx.elt("MathMark", pos, pos + 1), cx.elt("MathMark", i, end)]),
    );
  }
}

const OPEN_LINE = /^\$\$\s*$/;
const ONE_LINE = /^\$\$(?!\$)(.*\S.*?)\$\$\s*$/;

/** Is there a closing `$$` line below, before the next blank line? (An unclosed `$$` must not swallow the document.) */
function hasClosingLine(cx: BlockContext, line: Line): boolean {
  const input: Input = (cx as unknown as { input: Input }).input; // not in the public typings
  const start = cx.lineStart + line.text.length + 1;
  if (start >= input.length) return false;
  // A math block longer than this without a blank line is not a realistic equation.
  const lines = input.read(start, Math.min(input.length, start + 20000)).split("\n");
  for (const text of lines) {
    const bare = text.replace(/^[\s>]*/, ""); // container prefixes (quote marks, indentation)
    if (bare === "") return false;
    if (/^\$\$\s*$/.test(bare)) return true;
  }
  return false;
}

function startsMathBlock(cx: BlockContext, line: Line): "one" | "multi" | null {
  if (line.indent - line.baseIndent >= 4) return null;
  const rest = line.text.slice(line.pos);
  if (!rest.startsWith("$$")) return null;
  if (ONE_LINE.test(rest)) return "one";
  if (OPEN_LINE.test(rest) && hasClosingLine(cx, line)) return "multi";
  return null;
}

export const mathTag = Tag.define();

/** `@lezer/markdown` extension: InlineMath, MathBlock, MathMark. */
export const mathMarkdown: MarkdownConfig = {
  defineNodes: [
    { name: "InlineMath", style: mathTag },
    { name: "MathBlock", block: true, style: mathTag },
    { name: "MathMark", style: t.processingInstruction },
  ],
  parseInline: [{ name: "InlineMath", parse: parseInlineMath, before: "Emphasis" }],
  parseBlock: [
    {
      name: "MathBlock",
      before: "FencedCode",
      parse(cx, line) {
        const kind = startsMathBlock(cx, line);
        if (!kind) return false;
        const from = cx.lineStart + line.pos;
        if (kind === "one") {
          const to = cx.lineStart + line.text.trimEnd().length;
          cx.nextLine();
          cx.addElement(cx.elt("MathBlock", from, to, [cx.elt("MathMark", from, from + 2), cx.elt("MathMark", to - 2, to)]));
          return true;
        }
        const children = [cx.elt("MathMark", from, from + 2)];
        let to = from + 2;
        while (cx.nextLine() && (line as unknown as { depth: number }).depth >= (cx as unknown as { stack: unknown[] }).stack.length) {
          const bare = line.text.slice(line.pos);
          if (/^\$\$\s*$/.test(bare) && line.indent - line.baseIndent < 4) {
            const at = cx.lineStart + line.pos;
            children.push(cx.elt("MathMark", at, at + 2));
            to = at + 2;
            cx.nextLine();
            break;
          }
          to = cx.lineStart + line.text.length;
        }
        cx.addElement(cx.elt("MathBlock", from, to, children));
        return true;
      },
      endLeaf(cx, line) {
        return startsMathBlock(cx, line) !== null;
      },
    },
  ],
};

// ---------------------------------------------------------------- rendering + cache
export interface MathRender {
  html: string;
  error?: string;
}

/** Rendered HTML per TeX string (and display mode). Cleared whenever a new editor state is created (setDocument). */
export const mathCache = new Map<string, MathRender>();
export function clearMathCache(): void {
  mathCache.clear();
}

const escapeHtml = (s: string) => s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");

/** Render `tex` to an HTML string (cached). Errors yield a `.fw-math-error` span carrying the message as `title`. */
export function renderMath(tex: string, display: boolean): MathRender {
  const key = (display ? "D:" : "I:") + tex;
  let r = mathCache.get(key);
  if (r) return r;
  try {
    // throwOnError: true so the ParseError message is available for the title (false would draw an inline red error instead).
    r = { html: katex.renderToString(tex, { throwOnError: true, displayMode: display, output: "html" }) };
  } catch (e) {
    const error = e instanceof Error ? e.message : String(e);
    r = { html: `<span class="fw-math-error" title="${escapeHtml(error)}">${escapeHtml(tex)}</span>`, error };
  }
  mathCache.set(key, r);
  return r;
}

const el = (tag: string, cls: string): HTMLElement => {
  const e = document.createElement(tag);
  e.className = cls;
  return e;
};

/** Put the caret just inside the replaced node (reveals the raw TeX). */
function revealAt(dom: HTMLElement, e: Event, offset: number) {
  e.preventDefault();
  const view = EditorView.findFromDOM(dom);
  if (!view) return;
  const pos = view.posAtDOM(dom) + offset;
  view.dispatch({ selection: { anchor: Math.min(pos, view.state.doc.length) }, scrollIntoView: true });
  view.focus();
}

export class MathWidget extends WidgetType {
  constructor(readonly tex: string, readonly display: boolean) {
    super();
  }
  eq(o: MathWidget) {
    return o.tex === this.tex && o.display === this.display;
  }
  get estimatedHeight() {
    return -1;
  }
  toDOM() {
    const wrap = el("span", "fw-math-render " + (this.display ? "fw-math-display" : "fw-math-inline"));
    const r = renderMath(this.tex, this.display);
    wrap.innerHTML = r.html;
    if (r.error) wrap.classList.add("fw-math-failed");
    wrap.addEventListener("mousedown", (e) => revealAt(wrap, e, this.display ? 2 : 1));
    return wrap;
  }
  ignoreEvent() {
    return true;
  }
}

// ---------------------------------------------------------------- builder
const COLLAPSED_LINE = Decoration.line({ class: "fw-math-collapsed" });
const SRC_LINE = Decoration.line({ class: "fw-math-src-line" });
const SRC_MARK = Decoration.mark({ class: "fw-math-src" });
const HIDE = Decoration.replace({});

export interface MathResult {
  /** Widgets, hidden lines, line classes. */
  decorations: DecorationSet;
  /** Only the replace ranges: used for atomicRanges. */
  widgets: DecorationSet;
}

export function buildMath(
  state: EditorState,
  ranges?: readonly TextRange[],
  selection: EditorSelection = state.selection,
): MathResult {
  const doc = state.doc;
  const vis = ranges && ranges.length ? ranges : [{ from: 0, to: doc.length }];
  const decos: Range<Decoration>[] = [];
  const widgets: Range<Decoration>[] = [];

  const touched = (from: number, to: number) => selection.ranges.some((r) => r.from <= to && r.to >= from);
  const replace = (d: Decoration, from: number, to: number) => {
    const r = d.range(from, to);
    decos.push(r);
    widgets.push(r);
  };

  let maxTo = 0;
  for (const r of vis) maxTo = Math.max(maxTo, r.to);
  const tree = ensureSyntaxTree(state, Math.min(doc.length, maxTo), 50) ?? syntaxTree(state);

  for (const range of vis) {
    tree.iterate({
      from: Math.max(0, range.from),
      to: Math.min(doc.length, range.to),
      mode: IterMode.IgnoreMounts,
      enter: (node) => {
        const name = node.name;
        if (name !== "InlineMath" && name !== "MathBlock") return;
        const nf = node.from, nt = node.to;
        try {
          const marks: { from: number; to: number }[] = [];
          for (let c = node.node.firstChild; c; c = c.nextSibling) if (c.name === "MathMark") marks.push({ from: c.from, to: c.to });
          const closed = marks.length >= 2;
          const tex = closed ? doc.sliceString(marks[0].to, marks[marks.length - 1].from).trim() : "";
          const first = doc.lineAt(nf), last = doc.lineAt(nt);
          if (name === "InlineMath") {
            if (touched(nf, nt)) decos.push(SRC_MARK.range(nf, nt));
            else if (closed && tex) replace(Decoration.replace({ widget: new MathWidget(tex, false) }), nf, nt);
            return false;
          }
          // MathBlock
          if (touched(nf, nt) || !closed || !tex) {
            for (let n = first.number; n <= last.number; n++) decos.push(SRC_LINE.range(doc.line(n).from));
            return false;
          }
          replace(Decoration.replace({ widget: new MathWidget(tex, true) }), nf, first.to);
          // Remaining lines: text hidden, line collapsed to zero height (a replace may not span a line break).
          for (let n = first.number + 1; n <= last.number; n++) {
            const l = doc.line(n);
            decos.push(COLLAPSED_LINE.range(l.from));
            if (l.to > l.from) replace(HIDE, l.from, l.to);
          }
          return false;
        } catch {
          // Never throw out of the decoration builder: partial/invalid markdown must not break typing.
          return false;
        }
      },
    });
  }

  return { decorations: Decoration.set(decos, true), widgets: Decoration.set(widgets, true) };
}

/** Pure builder: decoration set for the given ranges (default: whole doc) and selection. */
export function buildMathDecorations(
  state: EditorState,
  ranges?: readonly TextRange[],
  selection?: EditorSelection,
): DecorationSet {
  return buildMath(state, ranges, selection).decorations;
}

// ---------------------------------------------------------------- plugin
class MathPlugin {
  decorations: DecorationSet = Decoration.none;
  widgets: DecorationSet = Decoration.none;
  private tree: unknown;
  private active = false;
  constructor(view: EditorView) {
    // A new plugin instance means a new EditorState (setDocument): start with an empty render cache.
    clearMathCache();
    this.rebuild(view);
  }
  update(u: ViewUpdate) {
    const active = !!u.view.plugin(liveExtension); // math only renders in live mode
    const tree = syntaxTree(u.state);
    if (active !== this.active || (active && (u.docChanged || u.viewportChanged || u.selectionSet || tree !== this.tree)))
      this.rebuild(u.view);
  }
  private rebuild(view: EditorView) {
    this.active = !!view.plugin(liveExtension);
    this.tree = syntaxTree(view.state);
    if (!this.active) {
      this.decorations = this.widgets = Decoration.none;
      return;
    }
    try {
      const r = buildMath(view.state, view.visibleRanges);
      this.decorations = r.decorations;
      this.widgets = r.widgets;
    } catch {
      this.decorations = this.widgets = Decoration.none;
    }
  }
}

const mathPlugin = ViewPlugin.fromClass(MathPlugin, {
  decorations: (v) => v.decorations,
  provide: (p) => EditorView.atomicRanges.of((view) => view.plugin(p)?.widgets ?? Decoration.none),
});

/** Source-mode (and revealed Live-mode) highlighting of math spans. */
const mathHighlight = HighlightStyle.define([{ tag: mathTag, class: "fw-math" }]);

// Styles live here (not in a .css file) so this module stays self-contained; colours fall back to the editor's variables.
const mathTheme = EditorView.baseTheme({
  ".fw-math": { color: "var(--fw-math-source, #6f42c1)" },
  ".fw-math-render": { cursor: "pointer" },
  ".fw-math-inline": { display: "inline-block", verticalAlign: "baseline", maxWidth: "100%" },
  ".fw-math-display": { display: "inline-block", width: "100%", verticalAlign: "top", overflowX: "auto", overflowY: "hidden", textAlign: "center" },
  ".fw-math-display .katex-display": { margin: "0.4em 0" },
  ".fw-math-collapsed": { height: "0", lineHeight: "0", fontSize: "0", padding: "0 !important", margin: "0", overflow: "hidden" },
  ".fw-math-error": { color: "var(--fw-math-error, #cc0000)", fontFamily: "var(--fw-mono, Menlo, monospace)", fontSize: "0.9em", borderBottom: "1px dotted currentColor" },
  ".fw-math-src-line": { color: "var(--fw-math-source, #6f42c1)" },
  "&dark .fw-math, &dark .fw-math-src-line": { color: "var(--fw-math-source, #b392f0)" },
});

/**
 * Register once from main.ts (`new Session([..., mathExtension()])`). The Lezer extension itself is added
 * to the `markdown({ extensions })` call in modes.ts so both modes parse math.
 */
export function mathExtension(): Extension {
  return [mathPlugin, syntaxHighlighting(mathHighlight), mathTheme];
}
