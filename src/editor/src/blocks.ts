// Live mode phase 2: block-level rendering (images, task checkboxes, horizontal rules,
// fenced-code labels, table / nested-quote line classes) plus fenced-code syntax highlighting.
//
// Same structure as live.ts: a pure builder (`buildBlocks`) that walks the Lezer tree for the
// given ranges only, and a ViewPlugin around it. Reveal rule: when any selection range touches a
// node (inclusive of both ends; for task markers the node's *line*), the raw markdown is shown
// instead of the widget.
//
// Widgets are inline replace decorations (a ViewPlugin may not provide block decorations); the
// ones that stand alone on a line are styled `display: block` from blocks.css.
import { EditorSelection, EditorState, Extension, Range } from "@codemirror/state";
import { Decoration, DecorationSet, EditorView, ViewPlugin, ViewUpdate, WidgetType } from "@codemirror/view";
import { ensureSyntaxTree, syntaxHighlighting, syntaxTree } from "@codemirror/language";
import { fenceLanguages as curatedLanguages } from "./languages";
import { IterMode, NodeType } from "@lezer/common";
import { tagHighlighter, tags as t } from "@lezer/highlight";
import { liveExtension } from "./live";
import type { TextRange } from "./live";

// ---------------------------------------------------------------- code languages
/** Pass to `markdown({ codeLanguages })`: fenced blocks get the language's parser, see languages.ts. */
export const fenceLanguages = curatedLanguages;

/** Token classes for fenced-code highlighting. `scope` skips the markdown tree itself (only nested languages). */
export const blocksHighlight = tagHighlighter(
  [
    { tag: [t.keyword, t.modifier, t.controlKeyword, t.operatorKeyword, t.definitionKeyword, t.moduleKeyword], class: "fw-tok-keyword" },
    { tag: [t.string, t.special(t.string), t.character, t.regexp], class: "fw-tok-string" },
    { tag: [t.number, t.integer, t.float], class: "fw-tok-number" },
    { tag: [t.bool, t.null, t.atom, t.self], class: "fw-tok-atom" },
    { tag: [t.comment, t.lineComment, t.blockComment, t.docComment], class: "fw-tok-comment" },
    { tag: [t.function(t.variableName), t.function(t.propertyName), t.definition(t.function(t.variableName))], class: "fw-tok-function" },
    { tag: [t.typeName, t.className, t.namespace, t.tagName], class: "fw-tok-type" },
    { tag: [t.propertyName, t.attributeName, t.definition(t.propertyName)], class: "fw-tok-property" },
    { tag: [t.operator, t.punctuation, t.bracket, t.angleBracket, t.separator], class: "fw-tok-punct" },
    { tag: [t.meta, t.processingInstruction, t.annotation, t.labelName], class: "fw-tok-meta" },
    { tag: [t.variableName, t.definition(t.variableName)], class: "fw-tok-variable" },
    { tag: t.invalid, class: "fw-tok-invalid" },
  ],
  { scope: (type: NodeType) => type.name !== "Document" },
);

// ---------------------------------------------------------------- image store (host round trip)
export interface ImageEntry {
  status: "pending" | "ready" | "error";
  dataUrl?: string;
  error?: string;
  /** Natural size once the <img> has loaded (used for estimatedHeight). */
  width?: number;
  height?: number;
  /** ms timestamp of the last reply (errors are retried after ERROR_RETRY_MS). */
  at: number;
  listeners: Set<() => void>;
}

const ERROR_RETRY_MS = 5000;

/** Resolves `src` through the host (`bridge.requestImage` / `bridge.imageReply`), one request per `src`. */
export class ImageStore {
  readonly cache = new Map<string, ImageEntry>();
  private tokens = new Map<number, string>();
  private nextToken = 1;
  /** Sends a request to the host; null until a bridge is connected (entries then stay pending). */
  sink: ((token: number, src: string) => void) | null = null;

  /** The entry for `src`, requesting it from the host if it was never asked for (or failed a while ago). */
  ensure(src: string): ImageEntry {
    let e = this.cache.get(src);
    if (e && !(e.status === "error" && Date.now() - e.at > ERROR_RETRY_MS)) return e;
    const listeners = e ? e.listeners : new Set<() => void>();
    e = { status: "pending", at: Date.now(), listeners };
    this.cache.set(src, e);
    if (this.sink) {
      const token = this.nextToken++;
      this.tokens.set(token, src);
      try {
        this.sink(token, src);
      } catch (err) {
        this.settle(src, "", err instanceof Error ? err.message : String(err));
      }
    }
    return e;
  }

  /** Host answer. Replies for unknown tokens (e.g. after a document switch) are ignored. */
  reply(token: number, dataUrl: string, error: string): void {
    const src = this.tokens.get(token);
    if (src === undefined) return;
    this.tokens.delete(token);
    this.settle(src, dataUrl, error);
  }

  private settle(src: string, dataUrl: string, error: string) {
    const e = this.cache.get(src);
    if (!e) return;
    if (!error && /^data:image\//i.test(dataUrl)) {
      e.status = "ready";
      e.dataUrl = dataUrl;
      e.error = undefined;
    } else {
      e.status = "error";
      e.dataUrl = undefined;
      e.error = error || "host returned no image";
    }
    e.at = Date.now();
    for (const l of [...e.listeners]) l();
  }

  /** Forget everything (document switch: relative `src` values mean something else now). */
  clear(): void {
    this.cache.clear();
    this.tokens.clear();
  }
}

/** Module-level store used by the editor. `imageCache` is its `Map` keyed by `src`. */
export const imageStore = new ImageStore();
export const imageCache = imageStore.cache;

/** The slice of the bridge that this module uses (all optional: older hosts lack image support). */
export interface ImageBridge {
  requestImage?: (token: number, src: string) => void;
  imageReply?: { connect(fn: (token: number, dataUrl: string, error: string) => void): void };
}

export function connectImageBridge(bridge: ImageBridge, store: ImageStore = imageStore): void {
  if (typeof bridge.requestImage === "function") store.sink = (token, src) => bridge.requestImage!(token, src);
  else store.sink = (token) => queueMicrotask(() => store.reply(token, "", "host does not support images"));
  bridge.imageReply?.connect?.((token, dataUrl, error) => store.reply(token, dataUrl, error));
}

// ---------------------------------------------------------------- widgets
const el = <K extends keyof HTMLElementTagNameMap>(tag: K, cls: string): HTMLElementTagNameMap[K] => {
  const e = document.createElement(tag);
  e.className = cls;
  return e;
};

/** Put the caret at the start of the replaced node (revealing the markdown). */
function revealAt(dom: HTMLElement, e: Event) {
  e.preventDefault();
  const view = EditorView.findFromDOM(dom);
  if (!view) return;
  const pos = view.posAtDOM(dom);
  view.dispatch({ selection: { anchor: pos }, scrollIntoView: true });
  view.focus();
}

export class ImageWidget extends WidgetType {
  private subs = new WeakMap<HTMLElement, () => void>();
  constructor(
    readonly src: string,
    readonly alt: string,
    readonly title: string,
    /** The image is alone on its line: render as a block. */
    readonly block: boolean,
    private store: ImageStore = imageStore,
  ) {
    super();
  }
  eq(o: ImageWidget) {
    return o.src === this.src && o.alt === this.alt && o.title === this.title && o.block === this.block;
  }
  get estimatedHeight() {
    const e = this.store.cache.get(this.src);
    return e && e.height ? Math.min(e.height, 600) : this.block ? 120 : -1;
  }
  toDOM(view: EditorView) {
    const wrap = el("span", "fw-image" + (this.block ? " fw-image-block" : ""));
    const entry = this.store.ensure(this.src);
    const render = () => {
      const e = this.store.cache.get(this.src) ?? entry;
      wrap.textContent = "";
      wrap.classList.toggle("fw-image-failed", e.status === "error");
      if (e.status === "ready" && e.dataUrl) {
        const img = el("img", "fw-image-img");
        img.alt = this.alt;
        if (this.title) img.title = this.title;
        img.draggable = false;
        img.addEventListener("load", () => {
          e.width = img.naturalWidth;
          e.height = img.naturalHeight;
          view.requestMeasure();
        });
        img.src = e.dataUrl;
        wrap.appendChild(img);
      } else {
        const ph = el("span", "fw-image-placeholder");
        ph.textContent = this.alt || "image";
        ph.title = e.status === "error" ? `${this.src}: ${e.error ?? "failed to load"}` : this.src;
        wrap.appendChild(ph);
      }
    };
    render();
    entry.listeners.add(render);
    this.subs.set(wrap, () => entry.listeners.delete(render));
    wrap.addEventListener("mousedown", (e) => revealAt(wrap, e));
    return wrap;
  }
  destroy(dom: HTMLElement) {
    this.subs.get(dom)?.();
  }
  ignoreEvent() {
    return true;
  }
}

export class TaskWidget extends WidgetType {
  constructor(readonly checked: boolean) {
    super();
  }
  eq(o: TaskWidget) {
    return o.checked === this.checked;
  }
  get estimatedHeight() {
    return -1;
  }
  toDOM() {
    const box = el("input", "fw-task");
    box.type = "checkbox";
    box.checked = this.checked;
    box.setAttribute("aria-label", this.checked ? "Completed task" : "Open task");
    box.addEventListener("mousedown", (e) => {
      e.preventDefault(); // keep the caret where it is and do not move focus
      const view = EditorView.findFromDOM(box);
      if (!view) return;
      const pos = view.posAtDOM(box);
      if (!/^\[[ xX]\]$/.test(view.state.sliceDoc(pos, pos + 3))) return;
      const done = view.state.sliceDoc(pos + 1, pos + 2) !== " ";
      view.dispatch({ changes: { from: pos + 1, to: pos + 2, insert: done ? " " : "x" }, userEvent: "input.toggle" });
    });
    box.addEventListener("click", (e) => e.preventDefault()); // the document is the source of truth
    return box;
  }
  ignoreEvent() {
    return true;
  }
}

export class HrWidget extends WidgetType {
  eq() {
    return true;
  }
  get estimatedHeight() {
    return 24;
  }
  toDOM() {
    const wrap = el("span", "fw-hr-wrap");
    wrap.appendChild(el("hr", "fw-hr"));
    wrap.addEventListener("mousedown", (e) => revealAt(wrap, e));
    return wrap;
  }
  ignoreEvent() {
    return true;
  }
}

/** Replaces an opening fence line ("```js") with the language label. */
export class FenceLabelWidget extends WidgetType {
  constructor(readonly label: string) {
    super();
  }
  eq(o: FenceLabelWidget) {
    return o.label === this.label;
  }
  get estimatedHeight() {
    return -1;
  }
  toDOM() {
    const s = el("span", "fw-code-label");
    s.textContent = this.label;
    return s;
  }
  ignoreEvent() {
    return false;
  }
}

/** Replaces a closing fence ("```") with an empty, line-height-preserving widget. */
export class FenceEndWidget extends WidgetType {
  eq() {
    return true;
  }
  get estimatedHeight() {
    return -1;
  }
  toDOM() {
    return el("span", "fw-fence-end");
  }
  ignoreEvent() {
    return false;
  }
}

// ---------------------------------------------------------------- builder
const lineDeco = new Map<string, Decoration>();
function line(cls: string): Decoration {
  let d = lineDeco.get(cls);
  if (!d) lineDeco.set(cls, (d = Decoration.line({ class: cls })));
  return d;
}
const markDim = Decoration.mark({ class: "fw-table-delim" });
const HR = Decoration.replace({ widget: new HrWidget() });
const FENCE_END = Decoration.replace({ widget: new FenceEndWidget() });
const TASK_OPEN = Decoration.replace({ widget: new TaskWidget(false) });
const TASK_DONE = Decoration.replace({ widget: new TaskWidget(true) });

export interface BlocksResult {
  /** Widgets + line classes. */
  decorations: DecorationSet;
  /** Only the replace (widget) ranges: used for atomicRanges. */
  widgets: DecorationSet;
}

function cleanSrc(raw: string): string {
  raw = raw.trim();
  return raw.startsWith("<") && raw.endsWith(">") ? raw.slice(1, -1) : raw;
}

export function buildBlocks(
  state: EditorState,
  ranges?: readonly TextRange[],
  selection: EditorSelection = state.selection,
): BlocksResult {
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
  const lines = (from: number, to: number, cls: string, clipFrom: number, clipTo: number) => {
    let pos = Math.max(from, clipFrom);
    const end = Math.min(to, clipTo);
    while (pos <= end && pos <= doc.length) {
      lineClass(pos, cls);
      pos = doc.lineAt(pos).to + 1;
    }
  };

  let maxTo = 0;
  for (const r of vis) maxTo = Math.max(maxTo, r.to);
  const tree = ensureSyntaxTree(state, Math.min(doc.length, maxTo), 50) ?? syntaxTree(state);

  for (const range of vis) {
    const cFrom = Math.max(0, range.from);
    const cTo = Math.min(doc.length, range.to);
    tree.iterate({
      from: cFrom,
      to: cTo,
      mode: IterMode.IgnoreMounts, // never look inside nested-language (fenced code) trees
      enter: (node) => {
        const name = node.name;
        const nf = node.from, nt = node.to;
        try {
          switch (name) {
            case "Image": {
              let url: { from: number; to: number } | null = null, title = "";
              const marks: number[][] = [];
              for (let c = node.node.firstChild; c; c = c.nextSibling) {
                if (c.name === "LinkMark") marks.push([c.from, c.to]);
                else if (c.name === "URL" && !url) url = { from: c.from, to: c.to };
                else if (c.name === "LinkTitle") title = doc.sliceString(c.from + 1, c.to - 1);
              }
              const l = doc.lineAt(nf);
              // Reference images / unclosed syntax have no URL; multi-line images stay raw.
              if (url && marks.length >= 2 && nt <= l.to && !touched(nf, nt)) {
                const src = cleanSrc(doc.sliceString(url.from, url.to));
                const alt = doc.sliceString(marks[0][1], marks[1][0]);
                const alone = doc.sliceString(l.from, l.to).trim() === doc.sliceString(nf, nt);
                if (src) replace(Decoration.replace({ widget: new ImageWidget(src, alt, title, alone) }), nf, nt);
              }
              return false;
            }
            case "TaskMarker": {
              const l = doc.lineAt(nf);
              if (!touched(l.from, l.to)) replace(doc.sliceString(nf + 1, nf + 2) === " " ? TASK_OPEN : TASK_DONE, nf, nt);
              break;
            }
            case "HorizontalRule":
              if (!touched(nf, nt) && doc.lineAt(nf).to >= nt) replace(HR, nf, nt);
              break;
            case "FencedCode": {
              const marks: { from: number; to: number }[] = [];
              let info: { from: number; to: number } | null = null;
              for (let c = node.node.firstChild; c; c = c.nextSibling) {
                if (c.name === "CodeMark") marks.push({ from: c.from, to: c.to });
                else if (c.name === "CodeInfo" && !info) info = { from: c.from, to: c.to };
              }
              const hide = !touched(nf, nt);
              if (marks.length) {
                const open = doc.lineAt(marks[0].from);
                lineClass(open.from, "fw-code-open");
                if (hide) {
                  lineClass(open.from, "fw-fence-hidden");
                  const label = info ? doc.sliceString(info.from, info.to).trim() : "";
                  replace(Decoration.replace({ widget: new FenceLabelWidget(label) }), marks[0].from, open.to);
                }
              }
              if (marks.length > 1) {
                const close = doc.lineAt(marks[1].from);
                if (close.from !== doc.lineAt(marks[0].from).from) {
                  lineClass(close.from, "fw-code-close");
                  if (hide) {
                    lineClass(close.from, "fw-fence-hidden");
                    replace(FENCE_END, marks[1].from, close.to);
                  }
                }
              }
              return false; // the body is code, nothing inside is markdown
            }
            case "Table":
              lines(nf, nt, "fw-table-line", cFrom, cTo);
              break;
            case "TableDelimiter":
              // The delimiter row (|---|:-:|) is one long node; cell separators are single "|".
              if (nt - nf > 1) {
                decos.push(markDim.range(nf, nt));
                lineClass(nf, "fw-table-delim-line");
              }
              break;
            case "Blockquote": {
              let depth = 0;
              for (let p: typeof node.node | null = node.node; p; p = p.parent) if (p.name === "Blockquote") depth++;
              lines(nf, nt, "fw-quote-d" + Math.min(depth, 4), cFrom, cTo);
              break;
            }
          }
        } catch {
          // Never throw out of the decoration builder: partial/invalid markdown must not break typing.
        }
      },
    });
  }

  return { decorations: Decoration.set(decos, true), widgets: Decoration.set(widgets, true) };
}

/** Pure builder: decoration set for the given ranges (default: whole doc) and selection. */
export function buildBlockDecorations(
  state: EditorState,
  ranges?: readonly TextRange[],
  selection?: EditorSelection,
): DecorationSet {
  return buildBlocks(state, ranges, selection).decorations;
}

// ---------------------------------------------------------------- plugin
class BlocksPlugin {
  decorations: DecorationSet = Decoration.none;
  widgets: DecorationSet = Decoration.none;
  private tree: unknown;
  private active = false;
  constructor(view: EditorView) {
    // A new plugin instance means a new EditorState (setDocument): relative image paths may now mean something else.
    imageStore.clear();
    this.rebuild(view);
  }
  update(u: ViewUpdate) {
    const active = !!u.view.plugin(liveExtension); // blocks only render in live mode
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
      const r = buildBlocks(view.state, view.visibleRanges);
      this.decorations = r.decorations;
      this.widgets = r.widgets;
    } catch {
      this.decorations = this.widgets = Decoration.none;
    }
  }
}

const blocksPlugin = ViewPlugin.fromClass(BlocksPlugin, {
  decorations: (v) => v.decorations,
  provide: (p) => EditorView.atomicRanges.of((view) => view.plugin(p)?.widgets ?? Decoration.none),
});

/**
 * Live-mode block rendering. Pass the bridge so images can be resolved by the host
 * (`requestImage` / `imageReply`); without it, images stay as placeholders.
 */
export function blocksExtension(bridge?: ImageBridge): Extension {
  if (bridge) connectImageBridge(bridge);
  return [blocksPlugin, syntaxHighlighting(blocksHighlight)];
}
