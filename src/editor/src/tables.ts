// Live mode: GFM tables rendered as a real grid.
//
// Reveal rule (same as live.ts / blocks.ts): when any selection range touches a top-level `Table`
// node (inclusive of both ends) the raw markdown lines stay visible (blocks.ts styles them with
// `fw-table-line` / `fw-table-delim-line`); otherwise the whole table - every line, line breaks
// included - is replaced by ONE block widget holding `<table class="fw-table">`.
//
// Multi-line replacement. CodeMirror refuses block decorations and line-break-spanning replaces
// from anything computed per view (a ViewPlugin, or a `(view) => DecorationSet` facet function:
// "Block decorations may not be specified via plugins"); that is why live.ts splits hidden ranges
// per line and why blocks.ts uses inline widgets. A *StateField* feeding `EditorView.decorations`
// is exempt, so tables.ts emits a single `Decoration.replace({ widget, block: true })` over
// [first line start, last line end] - no per-line splitting, no zero-width filler replacements.
// Being static it also takes part in CM's height map (hence `estimatedHeight`), and is registered
// as an atomic range so the caret skips over the grid instead of landing inside hidden text.
// The one catch: a StateField cannot see whether Live mode is on (modes.ts keeps that in a
// compartment, and live.ts is a ViewPlugin). A tiny ViewPlugin therefore mirrors
// `view.plugin(liveExtension)` into a StateField through a `setLive` effect dispatched from a
// microtask (before the next paint), and the table field is empty while that flag is false.
//
// Layering: `parseTableText` / `splitRow` / `parseInline` are pure (strings in, plain data out) and
// unit-tested in node. `findTables` / `buildTableDecorations` need only an EditorState. Everything
// that touches the DOM (`buildTableDom`, `TableWidget.toDOM`, the Escape command) is kept apart
// and is not covered by the node tests.
import { EditorSelection, EditorState, Extension, Prec, Range, StateEffect, StateField, Text } from "@codemirror/state";
import { Decoration, DecorationSet, EditorView, ViewPlugin, ViewUpdate, WidgetType, keymap } from "@codemirror/view";
import { syntaxTree } from "@codemirror/language";
import type { Tree } from "@lezer/common";
import { liveExtension } from "./live";

// ---------------------------------------------------------------- pure: table text -> cells
export type Align = "left" | "center" | "right" | null;

export interface TableCellData {
  /** Trimmed cell source (escapes such as `\|` are still in it; `parseInline` unescapes). */
  text: string;
  /** Offsets of the trimmed text, relative to the `base` given to the parser. */
  from: number;
  to: number;
  /** Where a click on the rendered cell puts the caret (start of the cell's text). */
  at: number;
}

export interface ParsedTable {
  aligns: Align[];
  header: TableCellData[];
  rows: TableCellData[][];
}

/** True when the pipe at `i` is preceded by an odd number of backslashes. */
function escapedAt(s: string, i: number): boolean {
  let n = 0;
  while (i - 1 - n >= 0 && s[i - 1 - n] === "\\") n++;
  return n % 2 === 1;
}

/** Split one table line into cells at unescaped `|`; a leading and a trailing pipe do not make empty cells. */
export function splitRow(line: string, base = 0): TableCellData[] {
  const pipes: number[] = [];
  for (let i = 0; i < line.length; i++) if (line[i] === "|" && !escapedAt(line, i)) pipes.push(i);
  let start = 0;
  while (start < line.length && /\s/.test(line[start])) start++;
  let end = line.length;
  while (end > start && /\s/.test(line[end - 1])) end--;
  if (start >= end) return [];
  if (line[start] === "|" && pipes[0] === start) start++;
  if (end > start && line[end - 1] === "|" && pipes[pipes.length - 1] === end - 1) end--;
  const cells: TableCellData[] = [];
  let segFrom = start;
  const push = (segTo: number) => {
    const seg = line.slice(segFrom, segTo);
    const lead = seg.length - seg.trimStart().length;
    const text = seg.trim();
    const from = segFrom + lead;
    cells.push({
      text,
      from: base + from,
      to: base + from + text.length,
      at: base + (text ? from : segFrom + Math.min(1, seg.length)),
    });
  };
  for (const p of pipes) {
    if (p < start || p >= end) continue;
    push(p);
    segFrom = p + 1;
  }
  push(end);
  return cells;
}

const DELIM_CELL = /^:?-+:?$/;

/** Alignments from a delimiter row (`|:--|:-:|--:|`), or null when the line is not one. */
export function parseAlignments(line: string): Align[] | null {
  const cells = splitRow(line);
  if (!cells.length || !cells.every((c) => DELIM_CELL.test(c.text))) return null;
  return cells.map((c) => {
    const l = c.text.startsWith(":"), r = c.text.endsWith(":");
    return l && r ? "center" : r ? "right" : l ? "left" : null;
  });
}

/**
 * Parse the source of a whole table (header line, delimiter line, body lines; `\n` separated).
 * Rows are normalised to the delimiter row's column count: short rows are padded with empty
 * cells, extra cells are dropped (GFM). Returns null when there is no valid delimiter row.
 * Never throws.
 */
export function parseTableText(text: string, base = 0): ParsedTable | null {
  try {
    const lines: { text: string; at: number }[] = [];
    let at = 0;
    for (const t of text.split("\n")) {
      lines.push({ text: t, at: base + at });
      at += t.length + 1;
    }
    if (lines.length < 2) return null;
    const aligns = parseAlignments(lines[1].text);
    if (!aligns) return null;
    const n = aligns.length;
    const norm = (l: { text: string; at: number }): TableCellData[] => {
      const cells = splitRow(l.text, l.at).slice(0, n);
      while (cells.length < n) {
        const e = l.at + l.text.length;
        cells.push({ text: "", from: e, to: e, at: e });
      }
      return cells;
    };
    const header = norm(lines[0]);
    if (!lines[0].text.trim()) return null;
    const rows: TableCellData[][] = [];
    for (let i = 2; i < lines.length; i++) if (lines[i].text.trim()) rows.push(norm(lines[i]));
    return { aligns, header, rows };
  } catch {
    return null;
  }
}

// ---------------------------------------------------------------- pure: minimal inline markdown
export type Inline =
  | { k: "text"; v: string }
  | { k: "code"; v: string }
  | { k: "br" }
  | { k: "em" | "strong" | "del"; c: Inline[] }
  | { k: "link"; href: string; title: string; c: Inline[] };

const ASCII_PUNCT = /[!-/:-@[-`{-~]/;
const isWs = (c: string | undefined) => c === undefined || /\s/.test(c);
const isAlnum = (c: string | undefined) => c !== undefined && /[\p{L}\p{N}]/u.test(c);
const MAX_DEPTH = 6;

/** Only http(s), mailto, tel and relative/fragment URLs become links. */
export function safeHref(raw: string): string | null {
  const u = raw.replace(/[\u0000-\u001f\u007f\s]+/g, "");
  if (!u) return null;
  const m = /^([a-z][a-z0-9+.-]*):/i.exec(u);
  if (m && !/^(https?|mailto|tel)$/i.test(m[1])) return null;
  return u;
}

/** Index of a backtick run of exactly `n` at or after `from`, or -1. */
function findRun(s: string, ch: string, n: number, from: number): number {
  for (let i = from; i < s.length; i++) {
    if (s[i] !== ch) continue;
    let r = 1;
    while (s[i + r] === ch) r++;
    if (r === n) return i;
    i += r - 1;
  }
  return -1;
}

/** Index just past the end of a code span starting at `i`, or `i` when it is not closed. */
function skipCode(s: string, i: number): number {
  let n = 1;
  while (s[i + n] === "`") n++;
  const c = findRun(s, "`", n, i + n);
  return c < 0 ? i + n : c + n;
}

/** Index of the matching `close` for the bracket at `i` (nesting, escapes and code spans respected), or -1. */
function matchBracket(s: string, i: number, open: string, close: string): number {
  let depth = 0;
  for (let j = i; j < s.length; j++) {
    const c = s[j];
    if (c === "\\") j++;
    else if (c === "`") j = skipCode(s, j) - 1;
    else if (c === open) depth++;
    else if (c === close && --depth === 0) return j;
  }
  return -1;
}

/** Closing delimiter `d` ("*", "**", "_", "~~") for an opener whose content starts at `from`, or -1. */
function findClose(s: string, d: string, from: number): number {
  const ch = d[0];
  for (let j = from; j < s.length; j++) {
    const c = s[j];
    if (c === "\\") j++;
    else if (c === "`") j = skipCode(s, j) - 1;
    else if (c === ch) {
      let r = 1;
      while (s[j + r] === ch) r++;
      if (r < d.length || (d.length === 1 && r > 1 && ch !== "_")) {
        j += r - 1;
        continue;
      }
      const at = ch === "~" ? j : j + (r - d.length); // close with the last delimiter chars of the run
      if (at > from && !isWs(s[at - 1]) && !(ch === "_" && isAlnum(s[at + d.length]))) return at;
      j += r - 1;
    }
  }
  return -1;
}

/** Parse `[text](dest "title")` starting at the `[` at `i`; returns the node and the end index. */
function parseLink(s: string, i: number, depth: number): { end: number; text: string; href: string; title: string } | null {
  const rb = matchBracket(s, i, "[", "]");
  if (rb < 0 || s[rb + 1] !== "(") return null;
  const rp = matchBracket(s, rb + 1, "(", ")");
  if (rp < 0) return null;
  let dest = s.slice(rb + 2, rp).trim(), title = "";
  const t = /^(<[^>]*>|\S*)\s*(?:"([^"]*)"|'([^']*)')?$/.exec(dest);
  if (!t) return null;
  dest = t[1].startsWith("<") ? t[1].slice(1, -1) : t[1];
  title = t[2] ?? t[3] ?? "";
  return { end: rp + 1, text: s.slice(i + 1, rb), href: dest, title };
}

/** Parse the inline subset used in table cells: `**` `*` `_` `~~` `` ` `` `[text](url)`, `<br>`, backslash escapes. */
export function parseInline(s: string, depth = 0): Inline[] {
  const out: Inline[] = [];
  let buf = "";
  const flush = () => {
    if (buf) out.push({ k: "text", v: buf });
    buf = "";
  };
  const sub = (x: string) => (depth >= MAX_DEPTH ? ([{ k: "text", v: x }] as Inline[]) : parseInline(x, depth + 1));
  let i = 0;
  while (i < s.length) {
    const ch = s[i];
    if (ch === "\\" && i + 1 < s.length && ASCII_PUNCT.test(s[i + 1])) {
      buf += s[i + 1];
      i += 2;
      continue;
    }
    if (ch === "`") {
      let n = 1;
      while (s[i + n] === "`") n++;
      const c = findRun(s, "`", n, i + n);
      if (c >= 0) {
        let v = s.slice(i + n, c);
        if (v.length > 2 && v.startsWith(" ") && v.endsWith(" ") && v.trim()) v = v.slice(1, -1);
        flush();
        out.push({ k: "code", v });
        i = c + n;
      } else {
        buf += "`".repeat(n);
        i += n;
      }
      continue;
    }
    if (ch === "<") {
      const br = /^<br\s*\/?>/i.exec(s.slice(i, i + 8));
      if (br) {
        flush();
        out.push({ k: "br" });
        i += br[0].length;
        continue;
      }
      const au = /^<((?:https?:\/\/|mailto:)[^\s<>]+)>/i.exec(s.slice(i));
      if (au) {
        flush();
        out.push({ k: "link", href: au[1], title: "", c: [{ k: "text", v: au[1] }] });
        i += au[0].length;
        continue;
      }
    }
    if (ch === "[" || (ch === "!" && s[i + 1] === "[")) {
      const img = ch === "!";
      const l = parseLink(s, img ? i + 1 : i, depth);
      if (l) {
        flush();
        if (img) out.push({ k: "text", v: l.text }); // images are not rendered in cells: show the alt text
        else {
          const href = safeHref(l.href);
          if (href) out.push({ k: "link", href, title: l.title, c: sub(l.text) });
          else out.push(...sub(l.text));
        }
        i = l.end;
        continue;
      }
    }
    if (ch === "*" || ch === "_" || ch === "~") {
      let r = 1;
      while (s[i + r] === ch) r++;
      const intra = ch === "_" && isAlnum(s[i - 1]);
      let kind: "em" | "strong" | "del" | null = null;
      let d = "";
      if (ch === "~") {
        if (r === 2) (kind = "del"), (d = "~~");
      } else if (!intra) {
        if (r >= 2) (kind = "strong"), (d = ch + ch);
        else (kind = "em"), (d = ch);
      }
      if (kind && depth < MAX_DEPTH && !isWs(s[i + d.length])) {
        const c = findClose(s, d, i + d.length);
        if (c >= 0) {
          flush();
          out.push({ k: kind, c: sub(s.slice(i + d.length, c)) });
          i = c + d.length;
          continue;
        }
      }
      buf += ch.repeat(r);
      i += r;
      continue;
    }
    // bare http(s) URL (GFM autolink literal), only at a word start
    if ((ch === "h" || ch === "H") && !isAlnum(s[i - 1])) {
      const m = /^https?:\/\/[^\s<]+/i.exec(s.slice(i));
      if (m) {
        const url = m[0].replace(/[?!.,:;*_~'")\]]+$/, "");
        if (url.length > 8) {
          flush();
          out.push({ k: "link", href: url, title: "", c: [{ k: "text", v: url }] });
          i += url.length;
          continue;
        }
      }
    }
    buf += ch;
    i++;
  }
  flush();
  return out;
}

// ---------------------------------------------------------------- tree -> tables (EditorState only)
export interface TableInfo {
  /** Start of the first line / end of the last line of the table (what the widget replaces). */
  from: number;
  to: number;
  text: string;
  /** null when the source does not parse: the table then stays raw. */
  table: ParsedTable | null;
}

const tableCache = new WeakMap<Tree, { doc: Text; tables: TableInfo[] }>();

/** Top-level GFM tables of the (so far) parsed document. Cached per syntax tree. */
export function findTables(state: EditorState): TableInfo[] {
  const tree = syntaxTree(state);
  const hit = tableCache.get(tree);
  if (hit && hit.doc === state.doc) return hit.tables;
  const doc = state.doc;
  const tables: TableInfo[] = [];
  try {
    // Tables nested in quotes / list items are left raw: their lines carry container prefixes.
    for (let n = tree.topNode.firstChild; n; n = n.nextSibling) {
      if (n.name !== "Table") continue;
      const from = doc.lineAt(n.from).from;
      let end = n.to;
      if (end > from && doc.sliceString(end - 1, end) === "\n") end--;
      const to = doc.lineAt(end).to;
      const text = doc.sliceString(from, to);
      tables.push({ from, to, text, table: parseTableText(text) });
    }
  } catch {
    /* partial/invalid markdown must never break typing */
  }
  tableCache.set(tree, { doc, tables });
  return tables;
}

// ---------------------------------------------------------------- widget (DOM)
const ALIGN_CLASS: Record<string, string> = { left: "fw-align-left", center: "fw-align-center", right: "fw-align-right" };

function appendInline(parent: Node, nodes: Inline[], doc: Document) {
  for (const n of nodes) {
    switch (n.k) {
      case "text":
        parent.appendChild(doc.createTextNode(n.v));
        break;
      case "br":
        parent.appendChild(doc.createElement("br"));
        break;
      case "code": {
        const e = doc.createElement("code");
        e.textContent = n.v;
        parent.appendChild(e);
        break;
      }
      case "em":
      case "strong":
      case "del": {
        const e = doc.createElement(n.k === "em" ? "em" : n.k === "strong" ? "strong" : "del");
        appendInline(e, n.c, doc);
        parent.appendChild(e);
        break;
      }
      case "link": {
        const e = doc.createElement("a");
        e.setAttribute("href", n.href);
        if (n.title) e.title = n.title;
        e.rel = "noopener noreferrer";
        appendInline(e, n.c, doc);
        parent.appendChild(e);
        break;
      }
    }
  }
}

/** Build the grid DOM. Cells carry `data-at` = the cell's source offset from the start of the table. */
export function buildTableDom(t: ParsedTable, doc: Document = document): HTMLElement {
  const wrap = doc.createElement("div");
  wrap.className = "fw-table-wrap";
  const table = doc.createElement("table");
  table.className = "fw-table";
  const cell = (tag: "th" | "td", c: TableCellData, i: number) => {
    const e = doc.createElement(tag);
    const a = t.aligns[i];
    if (a) e.className = ALIGN_CLASS[a];
    e.setAttribute("data-at", String(c.at));
    try {
      appendInline(e, parseInline(c.text), doc);
    } catch {
      e.textContent = c.text;
    }
    return e;
  };
  const thead = doc.createElement("thead");
  const hr = doc.createElement("tr");
  t.header.forEach((c, i) => hr.appendChild(cell("th", c, i)));
  thead.appendChild(hr);
  table.appendChild(thead);
  const tbody = doc.createElement("tbody");
  for (const r of t.rows) {
    const tr = doc.createElement("tr");
    r.forEach((c, i) => tr.appendChild(cell("td", c, i)));
    tbody.appendChild(tr);
  }
  table.appendChild(tbody);
  wrap.appendChild(table);
  return wrap;
}

export class TableWidget extends WidgetType {
  constructor(
    /** The table's markdown source: the identity of the widget (offsets inside it are relative to its start). */
    readonly src: string,
    readonly table: ParsedTable,
  ) {
    super();
  }
  eq(o: TableWidget) {
    return o.src === this.src;
  }
  get estimatedHeight() {
    return (1 + this.table.rows.length) * 34 + 20;
  }
  toDOM(view: EditorView) {
    const wrap = buildTableDom(this.table, view.dom.ownerDocument);
    // The caret goes to the start of the clicked cell's text, which touches the table and so reveals the raw lines.
    wrap.addEventListener("mousedown", (e) => {
      e.preventDefault();
      if (e.button !== 0) return;
      const v = EditorView.findFromDOM(wrap);
      if (!v) return;
      const cell = (e.target as HTMLElement | null)?.closest?.("[data-at]");
      const off = cell ? Number(cell.getAttribute("data-at")) : 0;
      const start = v.posAtDOM(wrap);
      v.dispatch({ selection: { anchor: Math.min(v.state.doc.length, start + (Number.isFinite(off) ? off : 0)) }, scrollIntoView: true });
      v.focus();
    });
    wrap.addEventListener("click", (e) => e.preventDefault()); // links inside the grid never navigate
    return wrap;
  }
  ignoreEvent() {
    return true;
  }
}

// ---------------------------------------------------------------- builder
const touches = (selection: EditorSelection, from: number, to: number) => selection.ranges.some((r) => r.from <= to && r.to >= from);

/**
 * Pure builder: one block replace decoration per parsed table that the selection does not touch
 * (inclusive). Tables the selection touches, and tables that do not parse, produce nothing.
 */
export function buildTableDecorations(state: EditorState, selection: EditorSelection = state.selection): DecorationSet {
  const decos: Range<Decoration>[] = [];
  for (const t of findTables(state)) {
    if (!t.table || touches(selection, t.from, t.to)) continue;
    decos.push(Decoration.replace({ widget: new TableWidget(t.text, t.table), block: true }).range(t.from, t.to));
  }
  return Decoration.set(decos);
}

// ---------------------------------------------------------------- extension
/** Whether Live mode is on, as seen from state (modes.ts keeps the mode in a compartment, out of reach of a StateField). */
const setLive = StateEffect.define<boolean>();
const liveField = StateField.define<boolean>({
  create: () => false,
  update(v, tr) {
    for (const e of tr.effects) if (e.is(setLive)) v = e.value;
    return v;
  },
});

/** Mirrors `view.plugin(liveExtension)` into `liveField` (one microtask after a mode switch / new state). */
class LiveWatcher {
  private dead = false;
  constructor(view: EditorView) {
    this.sync(view);
  }
  update(u: ViewUpdate) {
    this.sync(u.view);
  }
  destroy() {
    this.dead = true;
  }
  private sync(view: EditorView) {
    const live = !!view.plugin(liveExtension);
    if (live === view.state.field(liveField, false)) return;
    // Dispatching during a view update is not allowed; a microtask still runs before the next paint.
    queueMicrotask(() => {
      if (this.dead || !!view.plugin(liveExtension) !== live || view.state.field(liveField, false) === live) return;
      view.dispatch({ effects: setLive.of(live) });
    });
  }
}

const tableField = StateField.define<DecorationSet>({
  create: () => Decoration.none,
  update(deco, tr) {
    const live = tr.state.field(liveField, false) ?? false;
    const changed =
      tr.docChanged || tr.selection !== undefined || tr.effects.some((e) => e.is(setLive)) || syntaxTree(tr.startState) !== syntaxTree(tr.state);
    if (!changed) return deco;
    if (!live) return Decoration.none;
    try {
      return buildTableDecorations(tr.state);
    } catch {
      return Decoration.none;
    }
  },
  provide: (f) => [
    EditorView.decorations.from(f), // a StateField (not a plugin) may supply block decorations that replace line breaks
    EditorView.atomicRanges.of((view) => view.state.field(f)),
  ],
});

/** Escape inside a table (shown raw) moves the caret just below it (or above, at the end of the document): the grid returns. */
export function escapeTable(view: EditorView): boolean {
  if (!view.plugin(liveExtension)) return false;
  const doc = view.state.doc;
  const sel = view.state.selection;
  const t = findTables(view.state).find((x) => x.table && touches(sel, x.from, x.to));
  if (!t) return false;
  const target = t.to < doc.length ? t.to + 1 : t.from > 0 ? t.from - 1 : -1;
  if (target < 0) return false;
  view.dispatch({ selection: { anchor: target }, scrollIntoView: true });
  return true;
}

/**
 * Arrow keys step "into" an adjacent rendered grid: CodeMirror treats a block widget as one unit and
 * would jump past it, so from the line above (Down / Right at its end) the caret goes to the first
 * cell, and from the line below (Up / Left at its start) to the end of the table. Either reveals the raw lines.
 */
export function enterTable(forward: boolean, horizontal: boolean) {
  return (view: EditorView): boolean => {
    if (!view.plugin(liveExtension)) return false;
    const sel = view.state.selection;
    if (sel.ranges.length !== 1 || !sel.main.empty) return false;
    const head = sel.main.head;
    const line = view.state.doc.lineAt(head);
    if (horizontal && head !== (forward ? line.to : line.from)) return false;
    for (const t of findTables(view.state)) {
      if (!t.table || touches(sel, t.from, t.to)) continue;
      if (forward ? line.to + 1 === t.from : line.from === t.to + 1) {
        view.dispatch({ selection: { anchor: forward ? t.from + t.table.header[0].at : t.to }, scrollIntoView: true });
        return true;
      }
    }
    return false;
  };
}

export function tablesExtension(): Extension {
  return [
    liveField,
    tableField,
    ViewPlugin.fromClass(LiveWatcher),
    Prec.high(
      keymap.of([
        { key: "Escape", run: escapeTable },
        { key: "ArrowDown", run: enterTable(true, false) },
        { key: "ArrowRight", run: enterTable(true, true) },
        { key: "ArrowUp", run: enterTable(false, false) },
        { key: "ArrowLeft", run: enterTable(false, true) },
      ]),
    ),
  ];
}
