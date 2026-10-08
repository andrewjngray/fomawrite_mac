// Typora conventions in Live mode: footnotes, `[toc]`, YAML front matter and `==highlight==`.
//
// Three parts, all in this file (same shape as math.ts):
//   1. `extrasMarkdown`  - a @lezer/markdown extension: FootnoteRef `[^id]` (inline), FootnoteDef `[^id]: text` with
//                          indented continuation lines (block), TocBlock `[toc]` alone on a line (block) and
//                          FrontMatter (a `---` ... `---` block at the very start of the document) and
//                          Highlight (`==text==`, two `=` delimiters like GFM's `~~`; live.ts decorates it).
//   2. `buildExtras`     - pure decoration builder. Reveal rule as everywhere: a selection range touching a node
//                          (inclusive) shows its raw text; otherwise widgets replace it.
//   3. `extrasExtension` - ViewPlugin + atomic ranges. Live mode only.
//
// Derived data (footnote numbering, heading list) is computed from the syntax tree and cached per tree object,
// so it is rebuilt only when the document (or the parse) changes.
import { EditorSelection, EditorState, Extension, Range } from "@codemirror/state";
import { Decoration, DecorationSet, EditorView, ViewPlugin, ViewUpdate, WidgetType } from "@codemirror/view";
import { ensureSyntaxTree, syntaxTree } from "@codemirror/language";
import type { BlockContext, InlineContext, Line, MarkdownConfig } from "@lezer/markdown";
import { IterMode } from "@lezer/common";
import type { Input, Tree } from "@lezer/common";
import { liveExtension } from "./live";
import type { TextRange } from "./live";

// ---------------------------------------------------------------- parsing
const LBRACKET = 91, CARET = 94, RBRACKET = 93;
const isWs = (c: number) => c === 32 || c === 9 || c === 10 || c === 13;

/** Inline `[^id]`: id is one or more characters other than whitespace and brackets. */
function parseFootnoteRef(cx: InlineContext, next: number, pos: number): number {
  if (next !== LBRACKET || cx.char(pos + 1) !== CARET) return -1;
  for (let i = pos + 2; ; i++) {
    const c = cx.char(i);
    if (c < 0 || isWs(c) || c === LBRACKET) return -1;
    if (c === RBRACKET) {
      if (i === pos + 2) return -1;
      return cx.addElement(cx.elt("FootnoteRef", pos, i + 1));
    }
  }
}

const DEF_RE = /^\[\^([^\s\[\]]+)\]:/;
const TOC_RE = /^\[toc\]\s*$/i;
const FM_FENCE_RE = /^---\s*$/;
const FM_CLOSE_RE = /^(?:---|\.\.\.)\s*$/;

const inputOf = (cx: BlockContext): Input => (cx as unknown as { input: Input }).input; // not in the public typings

/** Text of the lines following the current one (up to `max` characters), for look-ahead. */
function peekLines(cx: BlockContext, line: Line, max: number): string[] {
  const input = inputOf(cx);
  const start = cx.lineStart + line.text.length + 1;
  if (start >= input.length) return [];
  return input.read(start, Math.min(input.length, start + max)).split("\n");
}

/** After a blank line: is the next non-blank line indented by 4+ columns (a further paragraph of the footnote)? */
function blankThenIndented(cx: BlockContext, line: Line): boolean {
  for (const text of peekLines(cx, line, 2000)) {
    if (/^\s*$/.test(text)) continue;
    return /^(?: {4}|\t)/.test(text);
  }
  return false;
}

const EQUALS = 61;
const HighlightDelim = { resolve: "Highlight", mark: "HighlightMark" };
const PUNCTUATION = /[!-\/:-@\[-`{-~\u00a1-\u00bf\u2010-\u2027\u2030-\u205e]/;

/** Inline `==text==`: exactly two `=` (not part of a longer run), flanked like GFM's `~~` so `a == b == c` stays text. */
function parseHighlight(cx: InlineContext, next: number, pos: number): number {
  if (next !== EQUALS || cx.char(pos + 1) !== EQUALS || cx.char(pos + 2) === EQUALS || cx.char(pos - 1) === EQUALS) return -1;
  const before = cx.slice(pos - 1, pos), after = cx.slice(pos + 2, pos + 3);
  const spaceBefore = /\s|^$/.test(before), spaceAfter = /\s|^$/.test(after);
  const punctBefore = PUNCTUATION.test(before), punctAfter = PUNCTUATION.test(after);
  return cx.addDelimiter(
    HighlightDelim,
    pos,
    pos + 2,
    !spaceAfter && (!punctAfter || spaceBefore || punctBefore),
    !spaceBefore && (!punctBefore || spaceAfter || punctAfter),
  );
}

/** `@lezer/markdown` extension: FootnoteRef, FootnoteDef (+ FootnoteLabel), TocBlock, FrontMatter (+ FrontMatterMark), Highlight (+ HighlightMark). */
export const extrasMarkdown: MarkdownConfig = {
  defineNodes: [
    { name: "FootnoteRef" },
    { name: "FootnoteLabel" },
    { name: "FootnoteDef", block: true },
    { name: "TocBlock", block: true },
    { name: "FrontMatter", block: true },
    { name: "FrontMatterMark" },
    { name: "Highlight" },
    { name: "HighlightMark" },
  ],
  parseInline: [
    { name: "FootnoteRef", parse: parseFootnoteRef, before: "Link" },
    { name: "Highlight", parse: parseHighlight, after: "Emphasis" },
  ],
  parseBlock: [
    {
      // Only at the very start of the document, and only when a closing `---` (or `...`) line exists.
      name: "FrontMatter",
      before: "HorizontalRule",
      parse(cx, line) {
        if (cx.lineStart !== 0 || cx.depth !== 1 || !FM_FENCE_RE.test(line.text)) return false;
        const rest = peekLines(cx, line, 50000);
        let at = cx.lineStart + line.text.length + 1;
        let closeStart = -1, closeLen = 0;
        for (const text of rest) {
          if (FM_CLOSE_RE.test(text)) {
            closeStart = at;
            closeLen = text.trimEnd().length;
            break;
          }
          at += text.length + 1;
        }
        if (closeStart < 0) return false;
        const children = [cx.elt("FrontMatterMark", 0, 3)];
        while (cx.lineStart < closeStart && cx.nextLine());
        children.push(cx.elt("FrontMatterMark", closeStart, closeStart + closeLen));
        const to = closeStart + closeLen;
        cx.nextLine();
        cx.addElement(cx.elt("FrontMatter", 0, to, children));
        return true;
      },
    },
    {
      name: "TocBlock",
      before: "LinkReference",
      parse(cx, line) {
        if (line.indent - line.baseIndent >= 4) return false;
        const rest = line.text.slice(line.pos);
        if (!TOC_RE.test(rest)) return false;
        const from = cx.lineStart + line.pos;
        const to = cx.lineStart + line.text.trimEnd().length;
        cx.nextLine();
        cx.addElement(cx.elt("TocBlock", from, to));
        return true;
      },
    },
    {
      // `[^id]: text` at the start of a (top-level) line; continuation = following lines indented 2+ columns,
      // and indented (4+) paragraphs after blank lines.
      name: "FootnoteDef",
      before: "LinkReference",
      parse(cx, line) {
        if (cx.depth !== 1 || line.indent >= 4) return false;
        const rest = line.text.slice(line.pos);
        const m = DEF_RE.exec(rest);
        if (!m) return false;
        const from = cx.lineStart + line.pos;
        const labelEnd = from + m[0].length;
        const children = [cx.elt("FootnoteLabel", from, labelEnd)];
        const addInline = (text: string, offset: number) => {
          if (text.trim()) for (const e of cx.parser.parseInline(text, offset)) children.push(e);
        };
        let skip = 0;
        while (rest[m[0].length + skip] === " " || rest[m[0].length + skip] === "\t") skip++;
        addInline(rest.slice(m[0].length + skip).trimEnd(), labelEnd + skip);
        let to = cx.lineStart + line.text.trimEnd().length;
        while (cx.nextLine()) {
          const text = line.text;
          if (/^\s*$/.test(text)) {
            if (!blankThenIndented(cx, line)) break;
            continue;
          }
          if (line.indent < 2) break;
          const at = cx.lineStart + line.pos;
          addInline(text.slice(line.pos).trimEnd(), at);
          to = cx.lineStart + text.trimEnd().length;
        }
        cx.addElement(cx.elt("FootnoteDef", from, to, children));
        return true;
      },
      endLeaf(_cx, line) {
        return line.indent < 4 && DEF_RE.test(line.text.slice(line.pos));
      },
    },
  ],
};

// ---------------------------------------------------------------- derived data (cached per syntax tree)
export interface FootnoteInfo {
  /** id -> number, in order of first reference (unreferenced definitions follow, in document order). */
  numbers: Map<string, number>;
  /** ids that have a definition. */
  defined: Set<string>;
}

const footnoteCache = new WeakMap<Tree, FootnoteInfo>();

export function footnoteInfo(state: EditorState): FootnoteInfo {
  const tree = ensureSyntaxTree(state, state.doc.length, 50) ?? syntaxTree(state);
  let info = footnoteCache.get(tree);
  if (info) return info;
  const numbers = new Map<string, number>();
  const defined = new Set<string>();
  const defOrder: string[] = [];
  tree.iterate({
    mode: IterMode.IgnoreMounts,
    enter: (n) => {
      if (n.name === "FootnoteRef") {
        const id = state.doc.sliceString(n.from + 2, n.to - 1);
        if (!numbers.has(id)) numbers.set(id, numbers.size + 1);
        return false;
      }
      if (n.name === "FootnoteLabel") {
        const id = state.doc.sliceString(n.from + 2, n.to - 2);
        defined.add(id);
        defOrder.push(id);
        return false;
      }
    },
  });
  for (const id of defOrder) if (!numbers.has(id)) numbers.set(id, numbers.size + 1);
  info = { numbers, defined };
  footnoteCache.set(tree, info);
  return info;
}

export interface Heading {
  level: number;
  text: string;
  /** Start of the heading's `#` mark. */
  from: number;
  /** Start of the heading's text (where the caret goes when an entry is clicked). */
  textFrom: number;
}

/** Strip the most common inline markdown from heading text (`**`, `` ` ``, `[text](url)`, ...). */
export function plainHeadingText(s: string): string {
  return s
    .replace(/!?\[([^\]]*)\]\([^)]*\)/g, "$1")
    .replace(/[*_~`]+/g, "")
    .replace(/\s+/g, " ")
    .trim();
}

const headingCache = new WeakMap<Tree, Heading[]>();

/** ATX headings of levels 1-3 in document order (cached per syntax tree). */
export function collectHeadings(state: EditorState): Heading[] {
  const tree = ensureSyntaxTree(state, state.doc.length, 50) ?? syntaxTree(state);
  let list = headingCache.get(tree);
  if (list) return list;
  const out: Heading[] = (list = []);
  const doc = state.doc;
  tree.iterate({
    mode: IterMode.IgnoreMounts,
    enter: (n) => {
      const m = /^ATXHeading([1-6])$/.exec(n.name);
      if (!m) {
        // Only containers can hold headings; skip everything else (fenced code, tables, paragraphs, ...).
        return n.name === "Document" || n.name === "Blockquote" || n.name === "BulletList" || n.name === "OrderedList" || n.name === "ListItem";
      }
      const level = Number(m[1]);
      if (level <= 3) {
        let from = n.from, to = n.to;
        const marks: { from: number; to: number }[] = [];
        for (let c = n.node.firstChild; c; c = c.nextSibling) if (c.name === "HeaderMark") marks.push({ from: c.from, to: c.to });
        if (marks.length) {
          from = marks[0].to;
          if (marks.length > 1) to = marks[marks.length - 1].from;
        }
        const raw = doc.sliceString(from, to);
        const lead = raw.length - raw.trimStart().length;
        out.push({ level, text: plainHeadingText(raw), from: n.from, textFrom: from + lead });
      }
      return false;
    },
  });
  headingCache.set(tree, out);
  return out;
}

export interface TocNode {
  heading: Heading;
  /** Index into the flat heading list (stable identity for click handling). */
  index: number;
  children: TocNode[];
}

/** Nest a flat heading list by level (a heading deeper than its predecessor becomes its child, however big the jump). */
export function buildTocTree(headings: readonly Heading[]): TocNode[] {
  const roots: TocNode[] = [];
  const stack: TocNode[] = [];
  headings.forEach((heading, index) => {
    const node: TocNode = { heading, index, children: [] };
    while (stack.length && stack[stack.length - 1].heading.level >= heading.level) stack.pop();
    (stack.length ? stack[stack.length - 1].children : roots).push(node);
    stack.push(node);
  });
  return roots;
}

/** Number of top-level `key:` entries in the YAML between the `---` fences (`text` excludes the fences). */
export function countFrontMatterFields(text: string): number {
  let n = 0;
  for (const line of text.split("\n")) {
    if (/^(?:[A-Za-z0-9_$][^:#"'\n]*|"[^"\n]*"|'[^'\n]*'):(?:\s|$)/.test(line)) n++;
  }
  return n;
}

// ---------------------------------------------------------------- widgets
const el = <K extends keyof HTMLElementTagNameMap>(tag: K, cls: string): HTMLElementTagNameMap[K] => {
  const e = document.createElement(tag);
  e.className = cls;
  return e;
};

/** Put the caret `offset` characters after the start of the replaced node (reveals the raw text). */
function revealAt(dom: HTMLElement, e: Event, offset: number) {
  e.preventDefault();
  const view = EditorView.findFromDOM(dom);
  if (!view) return;
  const pos = Math.min(view.state.doc.length, view.posAtDOM(dom) + offset);
  view.dispatch({ selection: { anchor: pos }, scrollIntoView: true });
  view.focus();
}

export class FootnoteRefWidget extends WidgetType {
  constructor(readonly n: number, readonly id: string, readonly missing: boolean) {
    super();
  }
  eq(o: FootnoteRefWidget) {
    return o.n === this.n && o.id === this.id && o.missing === this.missing;
  }
  get estimatedHeight() {
    return -1;
  }
  toDOM() {
    const sup = el("sup", "fw-footnote-ref" + (this.missing ? " fw-footnote-missing" : ""));
    sup.textContent = String(this.n);
    sup.title = this.missing ? `Footnote "${this.id}" has no definition` : this.id;
    sup.addEventListener("mousedown", (e) => revealAt(sup, e, 2));
    return sup;
  }
  ignoreEvent() {
    return true;
  }
}

export class FootnoteLabelWidget extends WidgetType {
  constructor(readonly n: number) {
    super();
  }
  eq(o: FootnoteLabelWidget) {
    return o.n === this.n;
  }
  get estimatedHeight() {
    return -1;
  }
  toDOM() {
    const s = el("span", "fw-footnote-label");
    s.textContent = this.n + ".";
    s.addEventListener("mousedown", (e) => revealAt(s, e, 2));
    return s;
  }
  ignoreEvent() {
    return true;
  }
}

function buildTocList(nodes: TocNode[], doc: Document): HTMLUListElement {
  const ul = doc.createElement("ul");
  ul.className = "fw-toc-list";
  for (const n of nodes) {
    const li = doc.createElement("li");
    li.className = "fw-toc-li fw-toc-l" + n.heading.level;
    const a = doc.createElement("a");
    a.className = "fw-toc-item";
    a.setAttribute("data-index", String(n.index));
    a.textContent = n.heading.text || "(untitled)";
    li.appendChild(a);
    if (n.children.length) li.appendChild(buildTocList(n.children, doc));
    ul.appendChild(li);
  }
  return ul;
}

export class TocWidget extends WidgetType {
  /** Identity: levels and texts (positions are re-read from the live state on click, so edits above do not rebuild the DOM). */
  readonly key: string;
  constructor(readonly headings: readonly Heading[]) {
    super();
    this.key = headings.map((h) => h.level + ":" + h.text).join("\n");
  }
  eq(o: TocWidget) {
    return o.key === this.key;
  }
  get estimatedHeight() {
    return Math.max(1, this.headings.length) * 24 + 16;
  }
  toDOM(view: EditorView) {
    const doc = view.dom.ownerDocument;
    const wrap = doc.createElement("div");
    wrap.className = "fw-toc";
    if (!this.headings.length) {
      const empty = doc.createElement("span");
      empty.className = "fw-toc-empty";
      empty.textContent = "Table of contents (no headings yet)";
      wrap.appendChild(empty);
    } else {
      wrap.appendChild(buildTocList(buildTocTree(this.headings), doc));
    }
    wrap.addEventListener("mousedown", (e) => {
      e.preventDefault();
      if (e.button !== 0) return;
      const v = EditorView.findFromDOM(wrap);
      if (!v) return;
      const a = (e.target as HTMLElement | null)?.closest?.("[data-index]");
      const idx = a ? Number(a.getAttribute("data-index")) : NaN;
      const h = collectHeadings(v.state)[idx];
      if (h) {
        v.dispatch({ selection: { anchor: h.textFrom }, effects: EditorView.scrollIntoView(h.textFrom, { y: "start", yMargin: 8 }) });
        v.focus();
      } else {
        // Clicked outside an entry: reveal the raw `[toc]`.
        v.dispatch({ selection: { anchor: v.posAtDOM(wrap) }, scrollIntoView: true });
        v.focus();
      }
    });
    return wrap;
  }
  ignoreEvent() {
    return true;
  }
}

export class FrontMatterWidget extends WidgetType {
  constructor(readonly fields: number) {
    super();
  }
  eq(o: FrontMatterWidget) {
    return o.fields === this.fields;
  }
  get estimatedHeight() {
    return -1;
  }
  toDOM() {
    const s = el("span", "fw-front-matter");
    s.textContent = `front matter · ${this.fields} ${this.fields === 1 ? "field" : "fields"}`;
    s.addEventListener("mousedown", (e) => {
      // Caret to the start of the second line: inside the block (reveals it) but not on the fence.
      const view = EditorView.findFromDOM(s);
      const second = view && view.state.doc.lines > 1 ? view.state.doc.line(2).from : 0;
      revealAt(s, e, second);
    });
    return s;
  }
  ignoreEvent() {
    return true;
  }
}

// ---------------------------------------------------------------- builder
const lineDeco = new Map<string, Decoration>();
function line(cls: string): Decoration {
  let d = lineDeco.get(cls);
  if (!d) lineDeco.set(cls, (d = Decoration.line({ class: cls })));
  return d;
}
const HIDE = Decoration.replace({});

export interface ExtrasResult {
  /** Widgets, hidden lines, line classes. */
  decorations: DecorationSet;
  /** Only the replace ranges: used for atomicRanges. */
  widgets: DecorationSet;
}

export function buildExtras(
  state: EditorState,
  ranges?: readonly TextRange[],
  selection: EditorSelection = state.selection,
): ExtrasResult {
  const doc = state.doc;
  const vis = ranges && ranges.length ? ranges : [{ from: 0, to: doc.length }];
  const decos: Range<Decoration>[] = [];
  const widgets: Range<Decoration>[] = [];
  const lineSeen = new Set<string>();

  const touched = (from: number, to: number) => selection.ranges.some((r) => r.from <= to && r.to >= from);
  const replace = (d: Decoration, from: number, to: number) => {
    const r = d.range(from, to);
    decos.push(r);
    widgets.push(r);
  };
  const lineClass = (pos: number, cls: string) => {
    const l = doc.lineAt(pos);
    const key = l.from + ":" + cls;
    if (lineSeen.has(key)) return;
    lineSeen.add(key);
    decos.push(line(cls).range(l.from));
  };
  const lines = (from: number, to: number, cls: string) => {
    for (let pos = from; pos <= to && pos <= doc.length; pos = doc.lineAt(pos).to + 1) lineClass(pos, cls);
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
        const nf = node.from, nt = node.to;
        try {
          switch (name) {
            case "FootnoteRef": {
              if (touched(nf, nt)) {
                decos.push(Decoration.mark({ class: "fw-footnote-src" }).range(nf, nt));
                return false;
              }
              const id = doc.sliceString(nf + 2, nt - 1);
              const info = footnoteInfo(state);
              const n = info.numbers.get(id);
              if (n !== undefined) replace(Decoration.replace({ widget: new FootnoteRefWidget(n, id, !info.defined.has(id)) }), nf, nt);
              return false;
            }
            case "FootnoteDef": {
              lines(nf, nt, "fw-footnote-def");
              const label = node.node.firstChild;
              if (label && label.name === "FootnoteLabel") {
                const first = doc.lineAt(nf);
                if (!touched(first.from, first.to)) {
                  const id = doc.sliceString(label.from + 2, label.to - 2);
                  const n = footnoteInfo(state).numbers.get(id);
                  let end = label.to;
                  while (end < first.to && /[ \t]/.test(doc.sliceString(end, end + 1))) end++;
                  if (n !== undefined) replace(Decoration.replace({ widget: new FootnoteLabelWidget(n) }), label.from, end);
                }
              }
              break; // the definition text may contain further refs
            }
            case "TocBlock": {
              if (touched(nf, nt)) {
                decos.push(Decoration.mark({ class: "fw-toc-src" }).range(nf, nt));
                return false;
              }
              replace(Decoration.replace({ widget: new TocWidget(collectHeadings(state)) }), nf, nt);
              return false;
            }
            case "FrontMatter": {
              const first = doc.lineAt(nf), last = doc.lineAt(nt);
              if (touched(nf, nt)) {
                for (let n = first.number; n <= last.number; n++) decos.push(line("fw-front-matter-line").range(doc.line(n).from));
                return false;
              }
              const text = doc.sliceString(first.to + 1, Math.max(first.to + 1, last.from));
              replace(Decoration.replace({ widget: new FrontMatterWidget(countFrontMatterFields(text)) }), nf, first.to);
              // Remaining lines: text hidden, line collapsed to zero height (a replace may not span a line break).
              for (let n = first.number + 1; n <= last.number; n++) {
                const l = doc.line(n);
                decos.push(line("fw-extras-collapsed").range(l.from));
                if (l.to > l.from) replace(HIDE, l.from, l.to);
              }
              return false;
            }
          }
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
export function buildExtrasDecorations(
  state: EditorState,
  ranges?: readonly TextRange[],
  selection?: EditorSelection,
): DecorationSet {
  return buildExtras(state, ranges, selection).decorations;
}

// ---------------------------------------------------------------- plugin
class ExtrasPlugin {
  decorations: DecorationSet = Decoration.none;
  widgets: DecorationSet = Decoration.none;
  private tree: unknown;
  private active = false;
  constructor(view: EditorView) {
    this.rebuild(view);
  }
  update(u: ViewUpdate) {
    const active = !!u.view.plugin(liveExtension); // only in live mode
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
      const r = buildExtras(view.state, view.visibleRanges);
      this.decorations = r.decorations;
      this.widgets = r.widgets;
    } catch {
      this.decorations = this.widgets = Decoration.none;
    }
  }
}

const extrasPlugin = ViewPlugin.fromClass(ExtrasPlugin, {
  decorations: (v) => v.decorations,
  provide: (p) => EditorView.atomicRanges.of((view) => view.plugin(p)?.widgets ?? Decoration.none),
});

/**
 * Register once from main.ts (`new Session([..., extrasExtension()])`). The Lezer extension (`extrasMarkdown`)
 * is added to the `markdown({ extensions })` call in modes.ts so both modes parse these constructs.
 */
export function extrasExtension(): Extension {
  return extrasPlugin;
}
