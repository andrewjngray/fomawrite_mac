// Marker chips in Live (Cycle 164): a PDF notes marker line
// `<!-- fomawrite:pdf KIND page=N … -->` is drawn as a small chip ("Highlight · p. 3",
// "Note · p. 3") and the header line `<!-- fomawrite:pdf-notes 1 -->` as a quiet
// "PDF notes" chip. Rendering only: the document is never changed. Like the other
// Live widgets, a chip gives way to the raw marker while the selection touches its
// line; Source mode never draws chips. Markers inside code are not comment blocks
// in the syntax tree, so they stay text (as in the notes parser).
import { EditorSelection, EditorState, Range } from "@codemirror/state";
import { Decoration, DecorationSet, EditorView, ViewPlugin, ViewUpdate, WidgetType } from "@codemirror/view";
import { ensureSyntaxTree, syntaxTree } from "@codemirror/language";
import { IterMode } from "@lezer/common";
import { liveExtension, TextRange } from "./live";

// The same patterns as src/pdfnotes.cpp (a marker is a whole line).
const HEADER = /^ {0,3}(<!--[ \t]*fomawrite:pdf-notes(?:[ \t]+\d+)?[ \t]*-->)[ \t]*$/;
const MARKER = /^ {0,3}(<!--[ \t]*fomawrite:pdf[ \t]+([A-Za-z][A-Za-z0-9_-]*)((?:[ \t]+[^\s=]+=\S*)*)[ \t]*-->)[ \t]*$/;
const KEY = /([^\s=]+)=(\S*)/g;

// Preview's colours, as in the PDF pane.
export const MARK_COLOURS: Record<string, string> = {
  yellow: "#ffd60a", green: "#34c759", blue: "#3d9df5", pink: "#ff5c9d", purple: "#a970ff", red: "#ff3b30",
};
const KIND_NAMES: Record<string, string> = { highlight: "Highlight", underline: "Underline", strike: "Strikethrough", note: "Note" };

export interface ChipInfo {
  /** "header", "highlight", "underline", "strike", "note", or another kind. */
  kind: string;
  label: string;
  /** A CSS colour for text marks; "" otherwise. */
  colour: string;
}

/** What a marker line says, or null when the line is not a marker. */
export function parseChip(line: string): ChipInfo | null {
  if (HEADER.test(line)) return { kind: "header", label: "PDF notes", colour: "" };
  const m = MARKER.exec(line);
  if (!m) return null;
  const kind = m[2].toLowerCase();
  const keys = new Map<string, string>();
  for (const k of m[3].matchAll(KEY)) if (!keys.has(k[1])) keys.set(k[1], k[2]);
  const name = KIND_NAMES[kind] ?? kind.charAt(0).toUpperCase() + kind.slice(1);
  const page = /^\d+$/.test(keys.get("page") ?? "") ? Number(keys.get("page")) : 0;
  const text = kind === "highlight" || kind === "underline" || kind === "strike";
  const wanted = keys.get("colour") ?? "";
  const colour = text ? MARK_COLOURS[wanted] ?? MARK_COLOURS[kind === "highlight" ? "yellow" : "red"] : "";
  return { kind, label: page > 0 ? `${name} · p. ${page}` : name, colour };
}

const PIN_PATH = "M12 2a6 6 0 0 0-6 6c0 4.5 6 12 6 12s6-7.5 6-12a6 6 0 0 0-6-6z";

export class PdfChipWidget extends WidgetType {
  constructor(readonly info: ChipInfo) {
    super();
  }
  eq(o: PdfChipWidget) {
    return o.info.kind === this.info.kind && o.info.label === this.info.label && o.info.colour === this.info.colour;
  }
  get estimatedHeight() {
    return -1;
  }
  toDOM() {
    const s = document.createElement("span");
    s.className = "fw-pdf-chip fw-pdf-chip-" + (/^[a-z0-9_-]+$/.test(this.info.kind) ? this.info.kind : "other");
    s.setAttribute("data-pdf-chip", this.info.kind);
    if (this.info.kind === "note") {
      const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
      svg.setAttribute("viewBox", "0 0 24 24");
      svg.setAttribute("class", "fw-pdf-chip-pin");
      const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
      path.setAttribute("d", PIN_PATH);
      svg.appendChild(path);
      s.appendChild(svg);
    } else if (this.info.colour) {
      const dot = document.createElement("span");
      dot.className = "fw-pdf-chip-dot";
      dot.style.background = this.info.colour;
      s.appendChild(dot);
    }
    s.appendChild(document.createTextNode(this.info.label));
    // A click puts the caret on the marker line, which reveals the raw marker (and the host shows the item on the page).
    s.addEventListener("mousedown", (e) => {
      e.preventDefault();
      const view = EditorView.findFromDOM(s);
      if (!view) return;
      view.dispatch({ selection: { anchor: Math.min(view.state.doc.length, view.posAtDOM(s)) }, scrollIntoView: true });
      view.focus();
    });
    return s;
  }
  ignoreEvent() {
    return true;
  }
}

const rawLine = Decoration.line({ class: "fw-pdf-marker-src" });

export interface ChipResult {
  decorations: DecorationSet;
  /** The replaced ranges, for atomicRanges. */
  widgets: DecorationSet;
}

export function buildPdfChips(
  state: EditorState,
  ranges?: readonly TextRange[],
  selection: EditorSelection = state.selection,
): ChipResult {
  const doc = state.doc;
  const vis = ranges && ranges.length ? ranges : [{ from: 0, to: doc.length }];
  const decos: Range<Decoration>[] = [];
  const widgets: Range<Decoration>[] = [];
  let maxTo = 0;
  for (const r of vis) maxTo = Math.max(maxTo, r.to);
  const tree = ensureSyntaxTree(state, Math.min(doc.length, maxTo), 50) ?? syntaxTree(state);
  const seen = new Set<number>();
  for (const range of vis) {
    tree.iterate({
      from: Math.max(0, range.from),
      to: Math.min(doc.length, range.to),
      mode: IterMode.IgnoreMounts,
      enter: (node) => {
        if (node.name !== "CommentBlock") return;
        try {
          const first = doc.lineAt(node.from);
          if (seen.has(first.from) || doc.lineAt(node.to).number !== first.number) return false;
          seen.add(first.from);
          const info = parseChip(first.text);
          if (!info) return false;
          if (selection.ranges.some((r) => r.from <= first.to && r.to >= first.from)) {
            decos.push(rawLine.range(first.from));
            return false;
          }
          const from = first.from + (first.text.length - first.text.trimStart().length);
          const to = first.from + first.text.trimEnd().length;
          const r = Decoration.replace({ widget: new PdfChipWidget(info) }).range(from, to);
          decos.push(r);
          widgets.push(r);
        } catch {
          // Never throw out of a decoration builder.
        }
        return false;
      },
    });
  }
  return { decorations: Decoration.set(decos, true), widgets: Decoration.set(widgets, true) };
}

/** The chips for a view: only while Live is on. */
export function chipsForView(view: Pick<EditorView, "state" | "visibleRanges" | "plugin">): ChipResult {
  if (!view.plugin(liveExtension)) return { decorations: Decoration.none, widgets: Decoration.none };
  return buildPdfChips(view.state, view.visibleRanges);
}

class PdfChipsPlugin {
  decorations: DecorationSet = Decoration.none;
  widgets: DecorationSet = Decoration.none;
  private tree: unknown;
  private active = false;
  constructor(view: EditorView) {
    this.rebuild(view);
  }
  update(u: ViewUpdate) {
    const active = !!u.view.plugin(liveExtension);
    const tree = syntaxTree(u.state);
    if (active !== this.active || (active && (u.docChanged || u.viewportChanged || u.selectionSet || tree !== this.tree)))
      this.rebuild(u.view);
  }
  private rebuild(view: EditorView) {
    this.active = !!view.plugin(liveExtension);
    this.tree = syntaxTree(view.state);
    try {
      const r = chipsForView(view);
      this.decorations = r.decorations;
      this.widgets = r.widgets;
    } catch {
      this.decorations = this.widgets = Decoration.none;
    }
  }
}

const pdfChipsPlugin = ViewPlugin.fromClass(PdfChipsPlugin, {
  decorations: (v) => v.decorations,
  provide: (p) => EditorView.atomicRanges.of((view) => view.plugin(p)?.widgets ?? Decoration.none),
});

export function pdfChipsExtension() {
  return pdfChipsPlugin;
}
