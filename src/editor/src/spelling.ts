// Spelling and grammar in the Live page. The host owns the checker (macOS system checker, see src/spellcheck.h); the page
// decides which stretches of the document are prose, asks the host what is wrong in them, and draws a red wavy underline
// for a misspelled word (`.fw-misspelled`) and a blue one for a grammar finding (`.fw-grammar`). A right-click on a red word
// offers suggestions, Learn Spelling and Ignore Spelling; on a blue range the checker's message, its corrections and
// Ignore Grammar Issue (page-local, per document).
//
// Layers, so most of it is testable in node without a DOM:
//   - proseSegments()        pure over EditorState: prose text (code, URLs, HTML, front matter, math blanked) per block
//   - spellField             the underline marks (DecorationSet), driven by effects, mapped through edits
//   - SpellEngine            tokens, the one request in flight (mapped through edits), stale-reply rules, caret-word rule
//   - spellingExtension()    the ViewPlugin (debounce, viewport), the context menu (plain DOM), the bridge glue
// See README "Spelling".
import { ChangeDesc, ChangeSet, EditorState, Extension, StateEffect, StateField, TransactionSpec } from "@codemirror/state";
import { Decoration, DecorationSet, EditorView, ViewPlugin, ViewUpdate } from "@codemirror/view";
import { ensureSyntaxTree, syntaxTree } from "@codemirror/language";
import type { Tree } from "@lezer/common";

export const DEBOUNCE_MS = 300;
/** Extra characters checked beyond the rendered viewport (which CodeMirror already pads), so short scrolls need no new request. */
export const MARGIN_CHARS = 2000;
/** One segment carries at most this many characters of prose (a very long paragraph is split at line ends). */
const MAX_SEGMENT_CHARS = 4000;
const SUGGESTION_TIMEOUT_MS = 500;
export const MAX_SUGGESTIONS = 5;

// ---------------------------------------------------------------- which text is prose (pure)

/** Syntax nodes whose whole extent is not prose: code, URLs, raw HTML, front matter, math, reference ids, footnote refs. */
const EXCLUDED_NODES = new Set([
  "FencedCode", "CodeBlock", "InlineCode", "CodeText", "CodeInfo",
  "URL", "Autolink",
  "HTMLTag", "HTMLBlock", "CommentBlock", "Comment", "ProcessingInstructionBlock", "ProcessingInstruction",
  "Entity",
  "FrontMatter",
  "InlineMath", "MathBlock",
  "LinkLabel", "LinkReference", "FootnoteRef", "FootnoteLabel", "TocBlock",
]);
/** Syntax characters: blanked so `_word_` and `**word**` reach the checker as plain words. */
const MARKUP_NODES = new Set([
  "EmphasisMark", "StrikethroughMark", "HighlightMark", "HeaderMark", "QuoteMark", "ListMark", "LinkMark", "TaskMarker", "TableDelimiter",
]);

export interface Segment { from: number; to: number; text: string }
export interface DocRange { from: number; to: number }
/** The checker's finding behind a grammar mark: its message and its corrections (up to `MAX_SUGGESTIONS`). */
export interface GrammarInfo { message: string; suggestions: string[] }
/** A finding in the document: spelling (no `grammar`) or grammar (with the checker's message and corrections). */
export interface SpellRange extends DocRange { grammar?: GrammarInfo }

/** The syntax tree for `state`, parsed up to `upto` if that can be done quickly; `complete` says whether it was. */
function treeFor(state: EditorState, upto: number): { tree: Tree; complete: boolean } {
  const full = ensureSyntaxTree(state, Math.min(state.doc.length, upto), 50);
  return full ? { tree: full, complete: true } : { tree: syntaxTree(state), complete: false };
}

const HAS_LETTER = /\p{L}/u;

/**
 * Prose of `from..to` (extended to whole lines), as segments the host can check. Every segment's `text` is exactly the
 * document text of `from..to` with the characters that are not prose (code, URLs, raw HTML, front matter, math, reference
 * ids, Markdown syntax characters) replaced by spaces, so the length matches `to - from` and an offset into `text` plus
 * `from` is a document offset. Consecutive lines that hold a letter are one segment (a paragraph); a segment never exceeds
 * about 4000 characters.
 */
export function proseSegments(state: EditorState, from: number, to: number, tree?: Tree): Segment[] {
  const doc = state.doc;
  const start = doc.lineAt(Math.max(0, Math.min(from, doc.length))).from;
  const end = doc.lineAt(Math.max(0, Math.min(to, doc.length))).to;
  if (end <= start) return [];
  const blank: [number, number][] = [];
  (tree ?? treeFor(state, end).tree).iterate({
    from: start,
    to: end,
    enter(n) {
      if (EXCLUDED_NODES.has(n.name) || MARKUP_NODES.has(n.name)) {
        blank.push([Math.max(n.from, start), Math.min(n.to, end)]);
        return false;
      }
      return undefined;
    },
  });
  blank.sort((a, b) => a[0] - b[0]);
  const segments: Segment[] = [];
  let group: { from: number; to: number; lines: string[]; size: number } | null = null;
  const flush = () => {
    if (group) segments.push({ from: group.from, to: group.to, text: group.lines.join("\n") });
    group = null;
  };
  for (let n = doc.lineAt(start).number; n <= doc.lineAt(end).number; n++) {
    const line = doc.line(n);
    let text = line.text;
    for (const [bf, bt] of blank) {
      if (bt <= line.from) continue;
      if (bf >= line.to) break;
      const a = Math.max(bf, line.from) - line.from, b = Math.min(bt, line.to) - line.from;
      if (b > a) text = text.slice(0, a) + " ".repeat(b - a) + text.slice(b);
    }
    if (!HAS_LETTER.test(text)) { flush(); continue; }
    if (group && group.size + text.length > MAX_SEGMENT_CHARS) flush();
    if (!group) group = { from: line.from, to: line.to, lines: [], size: 0 };
    group.lines.push(text);
    group.to = line.to;
    group.size += text.length + 1;
  }
  flush();
  return segments;
}

/** `from..to` plus a margin, aligned to whole lines. */
export function windowAround(state: EditorState, from: number, to: number, margin = MARGIN_CHARS): DocRange {
  const doc = state.doc;
  return {
    from: doc.lineAt(Math.max(0, from - margin)).from,
    to: doc.lineAt(Math.min(doc.length, to + margin)).to,
  };
}

// ---------------------------------------------------------------- the marks

/** `ranges` replace the marks inside `from..to` (all marks when `all`). An empty `ranges` clears that part. */
export interface SpellPatch { from: number; to: number; ranges: SpellRange[]; all: boolean }
export const setSpellEffect = StateEffect.define<SpellPatch>();
export const clearSpellEffect = StateEffect.define<null>();

const misspelledMark = Decoration.mark({ class: "fw-misspelled" });
/** One grammar mark per finding: the message and corrections ride on the spec, so the menu needs no host round trip. */
const grammarMark = (info: GrammarInfo) => Decoration.mark({ class: "fw-grammar", grammar: info });
const WORD_CHAR = /[\p{L}\p{N}\p{M}'’_]/u;

/**
 * A mark survives an edit unless the edit changed the word: it overlaps the mark, or it touches a mark edge and leaves a word
 * character next to it (typing a letter at either end, deleting the space that separated it from the next word). A space or
 * a line break typed next to the word leaves the mark in place.
 */
function editedWord(doc: { length: number; sliceString(a: number, b: number): string }, changed: DocRange[], from: number, to: number): boolean {
  for (const c of changed) {
    if (c.to < from || c.from > to) continue;
    if (c.from < to && c.to > from) return true; // overlaps the word
    if (c.to === from && from > 0 && WORD_CHAR.test(doc.sliceString(from - 1, from))) return true;
    if (c.from === to && to < doc.length && WORD_CHAR.test(doc.sliceString(to, to + 1))) return true;
  }
  return false;
}

export const spellField = StateField.define<DecorationSet>({
  create: () => Decoration.none,
  update(value, tr) {
    if (tr.docChanged) {
      const changed: DocRange[] = [];
      tr.changes.iterChangedRanges((_a, _b, fromB, toB) => { changed.push({ from: fromB, to: toB }); });
      const doc = tr.newDoc;
      value = value.map(tr.changes).update({ filter: (f, t) => !editedWord(doc, changed, f, t) });
    }
    for (const e of tr.effects) {
      if (e.is(clearSpellEffect)) value = Decoration.none;
      else if (e.is(setSpellEffect)) {
        const { from, to, ranges, all } = e.value;
        const len = tr.newDoc.length;
        const add = ranges
          .filter((r) => r.from >= 0 && r.to <= len && r.to > r.from)
          .sort((a, b) => a.from - b.from)
          .map((r) => (r.grammar ? grammarMark(r.grammar) : misspelledMark).range(r.from, r.to));
        value = value.update({ filter: all ? () => false : (f, t) => t <= from || f >= to, add, sort: true });
      }
    }
    return value;
  },
  provide: (f) => EditorView.decorations.from(f),
});

/** Every mark (spelling and grammar), in order; grammar ones carry their finding. */
export function markedRanges(state: EditorState): SpellRange[] {
  const out: SpellRange[] = [];
  const set = state.field(spellField, false);
  set?.between(0, state.doc.length, (from, to, value) => {
    const grammar = value.spec.grammar as GrammarInfo | undefined;
    out.push(grammar ? { from, to, grammar } : { from, to });
  });
  return out;
}

/** The spelling marks, in order. */
export function misspelledRanges(state: EditorState): DocRange[] {
  return markedRanges(state).filter((r) => !r.grammar).map(({ from, to }) => ({ from, to }));
}

/** The grammar marks, in order, with their message and corrections. */
export function grammarRanges(state: EditorState): (DocRange & GrammarInfo)[] {
  return markedRanges(state).flatMap((r) => (r.grammar ? [{ from: r.from, to: r.to, ...r.grammar }] : []));
}

/** The spelling mark containing `pos` (both ends inclusive), or null. */
export function misspelledAt(state: EditorState, pos: number): DocRange | null {
  return misspelledRanges(state).find((r) => r.from <= pos && pos <= r.to) ?? null;
}

/** The grammar mark containing `pos` (one that holds it strictly wins over one that only ends or starts there), or null. */
export function grammarAt(state: EditorState, pos: number): (DocRange & GrammarInfo) | null {
  const all = grammarRanges(state);
  return all.find((r) => r.from < pos && pos < r.to) ?? all.find((r) => r.from <= pos && pos <= r.to) ?? null;
}

/**
 * Reply JSON -> valid findings, ordered, of `[0, length]`; anything malformed is dropped (never throws). An entry without a
 * category, or with "Spelling", is a misspelled word; "Grammar" carries the checker's `message` and `suggestions`; any other
 * category is not drawn.
 */
export function parseSpellRanges(json: string, length: number): SpellRange[] {
  let raw: unknown;
  try { raw = JSON.parse(json); } catch { return []; }
  if (!Array.isArray(raw)) return [];
  const out: SpellRange[] = [];
  for (const r of raw) {
    const e = r as { from?: unknown; to?: unknown; category?: unknown; message?: unknown; suggestions?: unknown } | null;
    const from = e?.from, to = e?.to;
    if (!(typeof from === "number" && typeof to === "number" && Number.isInteger(from) && Number.isInteger(to) && from >= 0 && to > from && to <= length)) continue;
    const category = e?.category;
    if (category === undefined || category === "Spelling") out.push({ from, to });
    else if (category === "Grammar")
      out.push({
        from,
        to,
        grammar: {
          message: typeof e?.message === "string" ? e.message : "",
          suggestions: Array.isArray(e?.suggestions) ? parseSuggestions(JSON.stringify(e.suggestions)) : [],
        },
      });
  }
  return out.sort((a, b) => a.from - b.from);
}

/** Reply JSON -> up to `MAX_SUGGESTIONS` distinct non-empty words. */
export function parseSuggestions(json: string): string[] {
  let raw: unknown;
  try { raw = JSON.parse(json); } catch { return []; }
  if (!Array.isArray(raw)) return [];
  const out: string[] = [];
  for (const w of raw) if (typeof w === "string" && w.trim() !== "" && !out.includes(w)) out.push(w);
  return out.slice(0, MAX_SUGGESTIONS);
}

/**
 * The word the caret is still typing is not underlined: a range (spelling or grammar) that ends at an empty caret is withheld
 * (the writer has not finished it) and `withheldAt` says where, so it can be checked once the caret leaves.
 */
export function withholdCaretWord<R extends DocRange>(ranges: R[], selection: { empty: boolean; head: number }): { keep: R[]; withheldAt: number | null } {
  if (!selection.empty) return { keep: ranges, withheldAt: null };
  const keep = ranges.filter((r) => r.to !== selection.head);
  return { keep, withheldAt: keep.length === ranges.length ? null : selection.head };
}

// ---------------------------------------------------------------- the engine (tokens, stale replies, mapping)

export interface SpellingBridge {
  log?: (message: string) => void;
  checkSpelling?: (token: number, segmentsJson: string) => void;
  spellingSuggestions?: (token: number, word: string) => void;
  learnWord?: (word: string) => void;
  ignoreWord?: (word: string) => void;
  ignoreGrammar?: (text: string, message: string) => void;
  spellingReply?: { connect(fn: (token: number, rangesJson: string) => void): void };
  suggestionsReply?: { connect(fn: (token: number, wordsJson: string) => void): void };
  setSpellCheck?: { connect(fn: (enabled: boolean) => void): void };
}

interface InFlight { token: number; from: number; to: number; all: boolean; changes: ChangeDesc | null }

export class SpellEngine {
  /** The host's setting (`setSpellCheck`). On until told otherwise: a host that never says is asked. */
  enabled = true;
  /** Where a word was withheld because the caret was at its end (null: nothing withheld). */
  withheldAt: number | null = null;
  /** The part of the document the marks are known to be current for (mapped through edits). */
  checked: DocRange | null = null;
  /** The last request was made on a syntax tree that was not parsed that far; check again when the tree grows. */
  treeIncomplete = false;
  /** Texts of grammar findings the writer chose "Ignore Grammar Issue" for; per document (cleared by `clearIgnored`). */
  readonly ignoredGrammar = new Set<string>();
  private nextToken = 1;
  private inflight: InFlight | null = null;
  private suggest: { token: number; done: (words: string[]) => void; timer: ReturnType<typeof setTimeout> } | null = null;

  constructor(private bridge: SpellingBridge) {}

  /** Checking is on and the host can do it. */
  get active(): boolean {
    return this.enabled && typeof this.bridge.checkSpelling === "function";
  }

  log(message: string): void {
    try { this.bridge.log?.(message); } catch { /* ignore */ }
  }

  /** "Ignore Grammar Issue": remember `text`, drop every grammar mark spelling it, and filter it from later replies. */
  ignoreGrammar(state: EditorState, text: string): TransactionSpec {
    this.ignoredGrammar.add(text);
    return dropGrammarSpec(state, text);
  }

  /** A new document starts with no ignored grammar issues. */
  clearIgnored(): void {
    this.ignoredGrammar.clear();
  }

  /** Forget everything in flight (document replaced, checking switched off). */
  reset(): void {
    this.inflight = null;
    this.checked = null;
    this.withheldAt = null;
    this.cancelSuggestions();
  }

  /** Carry the request in flight, the checked window and the withheld caret through an edit. */
  noteChanges(changes: ChangeSet | ChangeDesc): void {
    const desc = "desc" in changes ? changes.desc : changes;
    if (this.inflight) this.inflight.changes = this.inflight.changes ? this.inflight.changes.composeDesc(desc) : desc;
    if (this.checked) {
      const from = desc.mapPos(this.checked.from, 1), to = desc.mapPos(this.checked.to, -1);
      this.checked = to > from ? { from, to } : null;
    }
    this.withheldAt = null; // an edit schedules a new check anyway
  }

  covers(win: DocRange): boolean {
    return !!this.checked && this.checked.from <= win.from && win.to <= this.checked.to;
  }

  /**
   * Send the prose of `win` to the host. `all` makes the reply replace every mark (a fresh start), not only those in `win`.
   * Returns the transaction to apply when there is nothing to ask (the window holds no prose: its marks are just cleared),
   * or null when a request went out (its reply comes through `reply`) or checking is off. A newer request makes the reply to
   * the older one stale.
   */
  request(state: EditorState, win: DocRange, all = false): TransactionSpec | null {
    if (!this.active) return null;
    const { tree, complete } = treeFor(state, win.to);
    this.treeIncomplete = !complete;
    const segments = proseSegments(state, win.from, win.to, tree);
    if (segments.length === 0) {
      this.inflight = null;
      this.checked = { ...win };
      return { effects: setSpellEffect.of({ from: win.from, to: win.to, ranges: [], all }) };
    }
    const token = this.nextToken++;
    this.inflight = { token, from: win.from, to: win.to, all, changes: null };
    try {
      this.bridge.checkSpelling!(token, JSON.stringify(segments));
    } catch (e) {
      this.inflight = null;
      this.log(`checkSpelling failed: ${e instanceof Error ? e.message : String(e)}`);
    }
    return null;
  }

  /**
   * The host's answer. Returns the transaction that applies it, or null when the reply is stale (a newer request or a reset
   * happened) or checking is off. Ranges are in the document as it was at the request; edits since are accounted for, and a
   * range an edit touched is dropped (the next check covers it).
   */
  reply(state: EditorState, token: number, rangesJson: string): TransactionSpec | null {
    const req = this.inflight;
    if (!req || req.token !== token || !this.enabled) return null;
    this.inflight = null;
    const desc = req.changes;
    const ranges: SpellRange[] = [];
    for (const r of parseSpellRanges(rangesJson, desc ? desc.length : state.doc.length)) {
      if (r.from < req.from || r.to > req.to) continue; // outside what was asked
      let from = r.from, to = r.to;
      if (desc) {
        if (desc.touchesRange(r.from, r.to)) continue;
        from = desc.mapPos(r.from, 1);
        to = desc.mapPos(r.to, -1);
        if (to <= from) continue;
      }
      if (r.grammar && this.ignoredGrammar.has(state.doc.sliceString(from, to))) continue; // "Ignore Grammar Issue"
      ranges.push(r.grammar ? { from, to, grammar: r.grammar } : { from, to });
    }
    let from = req.from, to = req.to;
    if (desc) { from = desc.mapPos(from, 1); to = desc.mapPos(to, -1); }
    if (to <= from) return null;
    const { keep, withheldAt } = withholdCaretWord(ranges, state.selection.main);
    this.withheldAt = withheldAt;
    this.checked = { from, to };
    return { effects: setSpellEffect.of({ from, to, ranges: keep, all: req.all }) };
  }

  /** `setSpellCheck(enabled)` from the host. Returns the transaction that clears the marks when it is off. */
  setEnabled(enabled: boolean): TransactionSpec | null {
    this.enabled = enabled;
    this.reset();
    return enabled ? null : { effects: clearSpellEffect.of(null) };
  }

  // ---- suggestions (one outstanding request; a newer one, or a timeout, makes a late reply stale)

  cancelSuggestions(): void {
    if (this.suggest) clearTimeout(this.suggest.timer);
    this.suggest = null;
  }

  /** Ask for suggestions for `word`; `done` gets them (or [] after 500 ms without an answer, or when the host cannot answer). */
  requestSuggestions(word: string, done: (words: string[]) => void, timeoutMs = SUGGESTION_TIMEOUT_MS): number {
    this.cancelSuggestions();
    const token = this.nextToken++;
    if (typeof this.bridge.spellingSuggestions !== "function") { done([]); return token; }
    const timer = setTimeout(() => {
      if (this.suggest?.token !== token) return;
      this.suggest = null;
      done([]);
    }, timeoutMs);
    this.suggest = { token, done, timer };
    try {
      this.bridge.spellingSuggestions(token, word);
    } catch (e) {
      this.log(`spellingSuggestions failed: ${e instanceof Error ? e.message : String(e)}`);
      this.cancelSuggestions();
      done([]);
    }
    return token;
  }

  /** The host's answer to `spellingSuggestions`; false when it is stale and was ignored. */
  suggestionsReply(token: number, wordsJson: string): boolean {
    const s = this.suggest;
    if (!s || s.token !== token) return false;
    clearTimeout(s.timer);
    this.suggest = null;
    s.done(parseSuggestions(wordsJson));
    return true;
  }
}

// ---------------------------------------------------------------- the context menu (plain DOM)

export type SpellChoice =
  | { kind: "suggestion"; word: string }
  | { kind: "grammarSuggestion"; word: string }
  | { kind: "learn" }
  | { kind: "ignore" }
  | { kind: "ignoreGrammar" };

/**
 * The transaction for a correction: replaces the word (or the grammar range) as an ordinary user change (reported to the
 * host, undoable; `input.spelling` or `input.grammar`). Null for Learn / Ignore (no text changes) and when the document no
 * longer holds `word` at `range`.
 */
export function choiceSpec(state: EditorState, range: DocRange, word: string, choice: SpellChoice): TransactionSpec | null {
  if (state.doc.sliceString(range.from, range.to) !== word) return null;
  if (choice.kind !== "suggestion" && choice.kind !== "grammarSuggestion") return null;
  return {
    changes: { from: range.from, to: range.to, insert: choice.word },
    selection: { anchor: range.from + choice.word.length },
    scrollIntoView: true,
    userEvent: choice.kind === "grammarSuggestion" ? "input.grammar" : "input.spelling",
  };
}

/** After Learn / Ignore: every spelling mark that spells exactly `word` goes at once (the host's re-check settles the rest). Grammar marks stay. */
export function dropWordSpec(state: EditorState, word: string): TransactionSpec {
  const keep = markedRanges(state).filter((r) => r.grammar || state.doc.sliceString(r.from, r.to) !== word);
  return { effects: setSpellEffect.of({ from: 0, to: state.doc.length, ranges: keep, all: true }) };
}

/** After Ignore Grammar Issue: every grammar mark whose text is exactly `text` goes (spelling marks stay). */
export function dropGrammarSpec(state: EditorState, text: string): TransactionSpec {
  const keep = markedRanges(state).filter((r) => !r.grammar || state.doc.sliceString(r.from, r.to) !== text);
  return { effects: setSpellEffect.of({ from: 0, to: state.doc.length, ranges: keep, all: true }) };
}

/** What a context menu shows: an optional non-clickable first line, corrections, then the fixed items after a separator. */
export interface MenuModel {
  label: string;
  /** The checker's message (grammar); a disabled first line. */
  header?: string;
  corrections: { text: string; choice: SpellChoice }[];
  /** Shown, disabled, when there are no corrections (spelling: "No Guesses Found"). */
  none?: string;
  footer: { cls: string; text: string; choice: SpellChoice }[];
}

export function spellingMenuModel(suggestions: string[]): MenuModel {
  return {
    label: "Spelling",
    corrections: suggestions.slice(0, MAX_SUGGESTIONS).map((word) => ({ text: word, choice: { kind: "suggestion", word } })),
    none: "No Guesses Found",
    footer: [
      { cls: "fw-spell-learn", text: "Learn Spelling", choice: { kind: "learn" } },
      { cls: "fw-spell-ignore", text: "Ignore Spelling", choice: { kind: "ignore" } },
    ],
  };
}

export function grammarMenuModel(info: GrammarInfo): MenuModel {
  return {
    label: "Grammar",
    header: info.message.trim() || "Possible grammar issue",
    corrections: info.suggestions.slice(0, MAX_SUGGESTIONS).map((word) => ({ text: word, choice: { kind: "grammarSuggestion", word } })),
    footer: [{ cls: "fw-spell-ignore-grammar", text: "Ignore Grammar Issue", choice: { kind: "ignoreGrammar" } }],
  };
}

class SpellMenu {
  readonly dom: HTMLElement;
  private cleanup: (() => void)[] = [];
  private closed = false;

  constructor(
    private view: EditorView,
    model: MenuModel,
    x: number,
    y: number,
    private onChoice: (choice: SpellChoice) => void,
    private onClose: () => void,
  ) {
    const doc = view.dom.ownerDocument;
    const dom = (this.dom = doc.createElement("div"));
    dom.className = "fw-spell-menu";
    dom.setAttribute("role", "menu");
    dom.setAttribute("aria-label", model.label);
    const item = (cls: string, text: string, choice: SpellChoice | null) => {
      const b = doc.createElement("button");
      b.type = "button";
      b.className = "fw-spell-item " + cls;
      b.setAttribute("role", "menuitem");
      b.textContent = text;
      if (choice) b.addEventListener("click", () => this.choose(choice));
      else b.disabled = true;
      dom.append(b);
    };
    if (model.header) item("fw-spell-message", model.header, null);
    for (const c of model.corrections) item("fw-spell-suggestion", c.text, c.choice);
    if (model.corrections.length === 0 && model.none) item("fw-spell-none", model.none, null);
    const sep = doc.createElement("div");
    sep.className = "fw-spell-separator";
    sep.setAttribute("role", "separator");
    dom.append(sep);
    for (const f of model.footer) item(f.cls, f.text, f.choice);

    dom.style.left = x + "px";
    dom.style.top = y + "px";
    doc.body.append(dom);
    // Keep it on screen.
    const box = dom.getBoundingClientRect(), win = doc.defaultView;
    if (win) {
      if (box.right > win.innerWidth - 4) dom.style.left = Math.max(4, win.innerWidth - box.width - 4) + "px";
      if (box.bottom > win.innerHeight - 4) dom.style.top = Math.max(4, y - box.height) + "px";
    }

    dom.addEventListener("mousedown", (e) => e.preventDefault()); // keep the editor's focus and selection
    dom.addEventListener("contextmenu", (e) => e.preventDefault());
    const buttons = () => Array.from(dom.querySelectorAll<HTMLButtonElement>("button:not(:disabled)"));
    const listen = (t: EventTarget, type: string, fn: (e: any) => void, capture = false) => {
      t.addEventListener(type, fn, capture);
      this.cleanup.push(() => t.removeEventListener(type, fn, capture));
    };
    listen(doc, "mousedown", (e: MouseEvent) => { if (!dom.contains(e.target as Node)) this.close(); }, true);
    listen(doc, "keydown", (e: KeyboardEvent) => {
      if (e.key === "Escape") { e.preventDefault(); e.stopPropagation(); this.close(); return; }
      const list = buttons();
      const at = list.indexOf(doc.activeElement as HTMLButtonElement);
      if (e.key === "ArrowDown") { e.preventDefault(); list[(at + 1) % list.length]?.focus(); }
      else if (e.key === "ArrowUp") { e.preventDefault(); list[(at <= 0 ? list.length : at) - 1]?.focus(); }
      else if (e.key === "Tab") { e.preventDefault(); this.close(); }
    }, true);
    if (win) {
      listen(win, "blur", () => this.close());
      listen(win, "resize", () => this.close());
    }
    listen(view.scrollDOM, "scroll", () => this.close());
    buttons()[0]?.focus();
  }

  private choose(choice: SpellChoice): void {
    this.close();
    this.onChoice(choice);
  }

  close(): void {
    if (this.closed) return;
    this.closed = true;
    for (const c of this.cleanup.splice(0)) c();
    this.dom.remove();
    this.onClose();
    this.view.focus();
  }
}

// ---------------------------------------------------------------- the extension

export function spellingExtension(bridge: SpellingBridge): Extension {
  const engine = new SpellEngine(bridge);
  let plugin: SpellPlugin | null = null;

  class SpellPlugin {
    private timer: ReturnType<typeof setTimeout> | null = null;
    private menu: SpellMenu | null = null;
    private menuGrammarMessage = "";

    constructor(private view: EditorView) {
      plugin = this;
      engine.reset(); // a replaced document (setDocument) forgets everything in flight
      engine.clearIgnored(); // and the grammar issues the writer ignored in the old one
      this.schedule(50);
    }

    update(u: ViewUpdate): void {
      if (u.docChanged) {
        engine.noteChanges(u.changes);
        this.closeMenu();
        this.schedule(DEBOUNCE_MS);
      } else if (u.viewportChanged) {
        if (!engine.covers(this.window())) this.schedule(150);
      } else if (u.selectionSet && engine.withheldAt !== null && (!u.state.selection.main.empty || u.state.selection.main.head !== engine.withheldAt)) {
        engine.withheldAt = null;
        this.schedule(50); // the caret left the word it was typing: check it now
      } else if (engine.treeIncomplete && syntaxTree(u.startState) !== syntaxTree(u.state)) {
        this.schedule(100); // the parse caught up: code and links are excluded properly now
      }
    }

    destroy(): void {
      if (this.timer !== null) clearTimeout(this.timer);
      this.closeMenu();
      if (plugin === this) plugin = null;
    }

    private window(): DocRange {
      return windowAround(this.view.state, this.view.viewport.from, this.view.viewport.to);
    }

    schedule(ms: number, all = false): void {
      if (this.timer !== null) clearTimeout(this.timer);
      this.timer = null;
      if (!engine.active) return;
      this.timer = setTimeout(() => {
        this.timer = null;
        this.run(all);
      }, ms);
    }

    private run(all: boolean): void {
      const spec = engine.request(this.view.state, this.window(), all);
      if (spec) this.view.dispatch(spec);
    }

    onReply(token: number, json: string): void {
      const spec = engine.reply(this.view.state, token, json);
      if (spec) this.view.dispatch(spec);
    }

    onSetSpellCheck(enabled: boolean): void {
      const spec = engine.setEnabled(enabled);
      if (this.timer !== null) { clearTimeout(this.timer); this.timer = null; }
      this.closeMenu();
      if (spec) this.view.dispatch(spec);
      else this.schedule(0, true); // on again, or words/language changed: check afresh, replacing every mark
    }

    // ---- context menu
    openMenu(range: DocRange, x: number, y: number): void {
      this.closeMenu();
      this.menuGrammarMessage = "";
      const word = this.view.state.doc.sliceString(range.from, range.to);
      engine.requestSuggestions(word, (words) => {
        if (plugin !== this || this.view.state.doc.sliceString(range.from, range.to) !== word) return; // edited meanwhile
        this.show(spellingMenuModel(words), range, word, x, y);
      });
    }

    /** The grammar menu needs no host round trip: the message and corrections came with the reply. */
    openGrammarMenu(range: DocRange, info: GrammarInfo, x: number, y: number): void {
      this.closeMenu();
      this.menuGrammarMessage = info.message;
      this.show(grammarMenuModel(info), range, this.view.state.doc.sliceString(range.from, range.to), x, y);
    }

    private show(model: MenuModel, range: DocRange, word: string, x: number, y: number): void {
      this.menu = new SpellMenu(this.view, model, x, y, (choice) => this.choose(range, word, choice), () => { this.menu = null; });
    }

    private choose(range: DocRange, word: string, choice: SpellChoice): void {
      const state = this.view.state;
      if (choice.kind === "suggestion" || choice.kind === "grammarSuggestion") {
        const spec = choiceSpec(state, range, word, choice);
        if (spec) this.view.dispatch(spec);
        return;
      }
      if (choice.kind === "ignoreGrammar") {
        if (state.doc.sliceString(range.from, range.to) !== word) return;
        this.view.dispatch(engine.ignoreGrammar(state, word));
        // The host owns the ignore list the pane and the Source editor read.
        try { bridge.ignoreGrammar?.(word, this.menuGrammarMessage); } catch (e) { engine.log(`ignoreGrammar failed: ${e instanceof Error ? e.message : String(e)}`); }
        return;
      }
      try {
        if (choice.kind === "learn") bridge.learnWord?.(word);
        else bridge.ignoreWord?.(word);
      } catch (e) {
        engine.log(`${choice.kind === "learn" ? "learnWord" : "ignoreWord"} failed: ${e instanceof Error ? e.message : String(e)}`);
      }
      this.view.dispatch(dropWordSpec(state, word));
    }

    closeMenu(): void {
      engine.cancelSuggestions();
      this.menu?.close();
      this.menu = null;
    }
  }

  bridge.spellingReply?.connect?.((token, json) => plugin?.onReply(token, json));
  bridge.suggestionsReply?.connect?.((token, json) => { engine.suggestionsReply(token, json); });
  bridge.setSpellCheck?.connect?.((enabled) => {
    if (plugin) plugin.onSetSpellCheck(!!enabled);
    else engine.setEnabled(!!enabled);
  });

  return [
    spellField,
    ViewPlugin.fromClass(SpellPlugin),
    EditorView.domEventHandlers({
      contextmenu(event, view) {
        if (!plugin || !engine.active) return false;
        const target = event.target as Element | null;
        // Where both apply, the spelling menu wins (the red underline is the one drawn).
        const span = target?.closest?.(".fw-misspelled");
        const gspan = span ? null : target?.closest?.(".fw-grammar");
        if (!span && !gspan) return false;
        const pos = view.posAtCoords({ x: event.clientX, y: event.clientY }, false);
        if (span) {
          const range = misspelledAt(view.state, pos) ?? misspelledAt(view.state, view.posAtDOM(span, 0));
          if (!range) return false;
          event.preventDefault();
          plugin.openMenu(range, event.clientX, event.clientY);
          return true;
        }
        const grammar = grammarAt(view.state, pos) ?? grammarAt(view.state, view.posAtDOM(gspan!, 0));
        if (!grammar) return false;
        event.preventDefault();
        plugin.openGrammarMenu({ from: grammar.from, to: grammar.to }, grammar, event.clientX, event.clientY);
        return true;
      },
    }),
  ];
}
