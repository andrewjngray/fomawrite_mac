// Word-style formatting marks (Show/Hide ¶): spaces as centred dots, tabs as arrows, a ¶ after every line.
// Presentation only: decorations over the visible ranges, never part of the document, never reported to the host.
// The host switches it with the appearance key `showMarks` (Session.setAppearance -> showMarksEffect).
import { StateEffect, StateField, Extension, Range } from "@codemirror/state";
import { Decoration, DecorationSet, EditorView, ViewPlugin, ViewUpdate, WidgetType } from "@codemirror/view";

export const showMarksEffect = StateEffect.define<boolean>();

export const showMarksField = StateField.define<boolean>({
  create: () => false,
  update(value, tr) {
    for (const e of tr.effects) if (e.is(showMarksEffect)) value = e.value;
    return value;
  },
});

/** `showMarks` from a `setAppearance` payload: a boolean, or undefined when absent / not a boolean. */
export function parseShowMarks(raw: unknown): boolean | undefined {
  if (typeof raw !== "object" || raw === null) return undefined;
  const v = (raw as Record<string, unknown>).showMarks;
  return typeof v === "boolean" ? v : undefined;
}

class EolWidget extends WidgetType {
  eq() { return true; }
  toDOM() {
    const el = document.createElement("span");
    el.className = "fw-ws-eol";
    el.setAttribute("aria-hidden", "true");
    return el;
  }
  ignoreEvent() { return true; }
}
const eol = Decoration.widget({ widget: new EolWidget(), side: 1 });
const spaceMark = Decoration.mark({ class: "fw-ws-space" });
const tabMark = Decoration.mark({ class: "fw-ws-tab" });

/** Decorations for the given document ranges (exported for tests; viewport-limited by the caller). */
export function marksFor(view: Pick<EditorView, "state">, ranges: readonly { from: number; to: number }[]): DecorationSet {
  const doc = view.state.doc;
  const out: Range<Decoration>[] = [];
  let lastLine = -1;
  for (const r of ranges) {
    let pos = r.from;
    while (pos <= r.to) {
      const line = doc.lineAt(pos);
      if (line.number > lastLine) {
        lastLine = line.number;
        const text = line.text;
        for (let i = 0; i < text.length; i++) {
          const c = text.charCodeAt(i);
          if (c === 32 || c === 0xa0) out.push(spaceMark.range(line.from + i, line.from + i + 1));
          else if (c === 9) out.push(tabMark.range(line.from + i, line.from + i + 1));
        }
        out.push(eol.range(line.to));
      }
      if (line.to >= r.to) break;
      pos = line.to + 1;
    }
  }
  return Decoration.set(out, true);
}

const marksPlugin = ViewPlugin.fromClass(
  class {
    decorations: DecorationSet;
    constructor(view: EditorView) { this.decorations = this.build(view); }
    update(u: ViewUpdate) {
      const toggled = u.startState.field(showMarksField) !== u.state.field(showMarksField);
      if (toggled || u.docChanged || u.viewportChanged) this.decorations = this.build(u.view);
    }
    build(view: EditorView): DecorationSet {
      if (!view.state.field(showMarksField)) return Decoration.none;
      return marksFor(view, view.visibleRanges);
    }
  },
  { decorations: (v) => v.decorations },
);

export function marksExtension(): Extension {
  return [showMarksField, marksPlugin];
}
