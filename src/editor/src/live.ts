// Live-preview decorations (Typora-style). Pure builder + ViewPlugin.
//
// The builder walks the Lezer markdown tree for the given ranges only and emits:
//   - zero-width Decoration.replace({}) for syntax markers that are not "revealed"
//   - mark decorations for inline styling (fw-em, fw-strong, fw-code, fw-strike, fw-live-link)
//   - fw-revealed marks on nodes the selection touches (their markers stay visible)
//   - line decorations for block context (fw-h1..6, fw-quote-line, fw-list-line + fw-list-ul / fw-list-ol, fw-code-line)
import { EditorSelection, EditorState, Range } from "@codemirror/state";
import { Decoration, DecorationSet, EditorView, ViewPlugin, ViewUpdate } from "@codemirror/view";
import { ensureSyntaxTree, syntaxTree } from "@codemirror/language";
import { IterMode } from "@lezer/common";

export interface TextRange {
  from: number;
  to: number;
}

/** Marker used on hidden-range decorations so tests/tools can recognise them. */
export const HIDDEN_SPEC_KEY = "fwHidden";
const HIDE = Decoration.replace({ [HIDDEN_SPEC_KEY]: true } as any);

export function isHiddenDecoration(d: Decoration): boolean {
  return !!(d.spec && (d.spec as any)[HIDDEN_SPEC_KEY]);
}

const mk = (cls: string) => Decoration.mark({ class: cls });
const MARKS = {
  em: mk("fw-em"),
  strong: mk("fw-strong"),
  code: mk("fw-code"),
  strike: mk("fw-strike"),
  link: mk("fw-live-link"),
  revealed: mk("fw-revealed"),
  listMark: mk("fw-list-mark"),
};
const lineDeco = new Map<string, Decoration>();
function line(cls: string): Decoration {
  let d = lineDeco.get(cls);
  if (!d) lineDeco.set(cls, (d = Decoration.line({ class: cls })));
  return d;
}

export interface LiveResult {
  /** All decorations (hidden ranges + marks + line decorations). */
  decorations: DecorationSet;
  /** Only the hidden (replace) ranges: used for atomicRanges. */
  hidden: DecorationSet;
}

const HEADING_RE = /^ATXHeading([1-6])$/;

export function buildLive(
  state: EditorState,
  ranges?: readonly TextRange[],
  selection: EditorSelection = state.selection,
): LiveResult {
  const doc = state.doc;
  const vis = ranges && ranges.length ? ranges : [{ from: 0, to: doc.length }];
  const decos: Range<Decoration>[] = [];
  const hiddenList: Range<Decoration>[] = [];
  const lineSeen = new Set<string>();

  const touched = (from: number, to: number) => selection.ranges.some((r) => r.from <= to && r.to >= from);

  /** Hide [from,to), split at line breaks (plugins may not replace line breaks). */
  const hide = (from: number, to: number) => {
    from = Math.max(0, from);
    to = Math.min(doc.length, to);
    while (from < to) {
      const l = doc.lineAt(from);
      const end = Math.min(to, l.to);
      if (end > from) {
        const r = HIDE.range(from, end);
        decos.push(r);
        hiddenList.push(r);
      }
      from = l.to + 1;
    }
  };
  const mark = (d: Decoration, from: number, to: number) => {
    if (to > from) decos.push(d.range(from, to));
  };
  const lines = (from: number, to: number, cls: string, clipFrom: number, clipTo: number) => {
    const d = line(cls);
    let pos = Math.max(from, clipFrom);
    const end = Math.min(to, clipTo);
    while (pos <= end && pos <= doc.length) {
      const l = doc.lineAt(pos);
      const key = l.from + ":" + cls;
      if (!lineSeen.has(key)) {
        lineSeen.add(key);
        decos.push(d.range(l.from));
      }
      pos = l.to + 1;
    }
  };

  // Line start -> "ul" | "ol" for list lines. A ListItem covers its nested items' lines too; the walk is pre-order, so
  // the deepest (nearest) list's item writes last and wins.
  const listKind = new Map<number, string>();
  const listLines = (from: number, to: number, kind: string, clipFrom: number, clipTo: number) => {
    let pos = Math.max(from, clipFrom);
    const end = Math.min(to, clipTo);
    while (pos <= end && pos <= doc.length) {
      const l = doc.lineAt(pos);
      listKind.set(l.from, kind);
      pos = l.to + 1;
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
      mode: IterMode.IgnoreMounts, // nested-language trees (fenced code) are not markdown
      enter: (node) => {
        const name = node.name;
        const nf = node.from, nt = node.to;
        try {
          let m: RegExpExecArray | null;
          if ((m = HEADING_RE.exec(name))) {
            lines(nf, nt, "fw-h" + m[1], cFrom, cTo);
            if (touched(nf, nt)) {
              mark(MARKS.revealed, nf, nt);
            } else {
              let first = true;
              for (let c = node.node.firstChild; c; c = c.nextSibling) {
                if (c.name !== "HeaderMark") continue;
                let from = c.from, to = c.to;
                if (first) {
                  if (doc.sliceString(to, to + 1) === " ") to++;
                } else {
                  while (from > nf && doc.sliceString(from - 1, from) === " ") from--;
                }
                first = false;
                hide(from, to);
              }
            }
            return;
          }
          switch (name) {
            case "Emphasis":
            case "StrongEmphasis":
            case "InlineCode":
            case "Strikethrough": {
              const deco =
                name === "Emphasis" ? MARKS.em : name === "StrongEmphasis" ? MARKS.strong : name === "InlineCode" ? MARKS.code : MARKS.strike;
              const markName = name === "InlineCode" ? "CodeMark" : name === "Strikethrough" ? "StrikethroughMark" : "EmphasisMark";
              mark(deco, nf, nt);
              if (touched(nf, nt)) {
                mark(MARKS.revealed, nf, nt);
              } else {
                for (let c = node.node.firstChild; c; c = c.nextSibling) if (c.name === markName) hide(c.from, c.to);
              }
              break;
            }
            case "Link": {
              const marks: { from: number; to: number }[] = [];
              for (let c = node.node.firstChild; c; c = c.nextSibling) if (c.name === "LinkMark") marks.push({ from: c.from, to: c.to });
              if (marks.length < 2) break;
              const open = marks[0], close = marks[1];
              mark(MARKS.link, open.to, close.from);
              if (touched(nf, nt)) {
                mark(MARKS.revealed, nf, nt);
              } else {
                hide(open.from, open.to);
                hide(close.from, nt); // "](url "title")" or "][ref]" up to the end of the link
              }
              break;
            }
            case "Image":
              return false; // rendered/handled by blocks.ts; keep its raw text untouched when revealed
            case "Blockquote":
              lines(nf, nt, "fw-quote-line", cFrom, cTo);
              break;
            case "QuoteMark": {
              const l = doc.lineAt(nf);
              if (touched(l.from, l.to)) {
                mark(MARKS.revealed, nf, nt);
              } else {
                hide(nf, doc.sliceString(nt, nt + 1) === " " ? nt + 1 : nt);
              }
              break;
            }
            case "ListItem": {
              lines(nf, nt, "fw-list-line", cFrom, cTo);
              const parent = node.node.parent?.name;
              if (parent === "BulletList") listLines(nf, nt, "ul", cFrom, cTo);
              else if (parent === "OrderedList") listLines(nf, nt, "ol", cFrom, cTo);
              break;
            }
            case "ListMark":
              mark(MARKS.listMark, nf, nt);
              break;
            case "FencedCode":
              lines(nf, nt, "fw-code-line", cFrom, cTo);
              break;
          }
        } catch (e) {
          // Never throw out of the decoration builder: partial/invalid markdown must not break typing.
        }
      },
    });
  }

  for (const [from, kind] of listKind) decos.push(line("fw-list-" + kind).range(from));

  return { decorations: Decoration.set(decos, true), hidden: Decoration.set(hiddenList, true) };
}

/** Pure builder: decoration set for the given ranges (default: whole doc) and selection. */
export function buildLiveDecorations(
  state: EditorState,
  ranges?: readonly TextRange[],
  selection?: EditorSelection,
): DecorationSet {
  return buildLive(state, ranges, selection).decorations;
}

let decorateMetric: ((ms: number) => void) | null = null;
/** Install a sink for "decorate" timing samples (rebuild time in ms). */
export function setDecorateMetricSink(fn: ((ms: number) => void) | null): void {
  decorateMetric = fn;
}

class LivePlugin {
  decorations: DecorationSet = Decoration.none;
  hidden: DecorationSet = Decoration.none;
  private tree: unknown;
  constructor(view: EditorView) {
    this.rebuild(view);
  }
  update(u: ViewUpdate) {
    const tree = syntaxTree(u.state);
    if (u.docChanged || u.viewportChanged || u.selectionSet || tree !== this.tree) this.rebuild(u.view);
  }
  private rebuild(view: EditorView) {
    const t0 = performance.now();
    this.tree = syntaxTree(view.state);
    try {
      const r = buildLive(view.state, view.visibleRanges);
      this.decorations = r.decorations;
      this.hidden = r.hidden;
    } catch {
      this.decorations = Decoration.none;
      this.hidden = Decoration.none;
    }
    decorateMetric?.(performance.now() - t0);
  }
}

const livePlugin = ViewPlugin.fromClass(LivePlugin, {
  decorations: (v) => v.decorations,
  provide: (p) => EditorView.atomicRanges.of((view) => view.plugin(p)?.hidden ?? Decoration.none),
});

export const liveExtension = livePlugin;
