// CM6 ChangeSet <-> JSON contract. Pure: no DOM, no view.
//
// Wire format: JSON array of {from, to, insert}. `from`/`to` are UTF-16 offsets into
// the PRE-change document, ascending by `from`, non-overlapping. A consumer may apply
// them in reverse order with plain slice/concat and needs no offset adjustment.
import type { ChangeSet, ChangeSpec, Text } from "@codemirror/state";

export interface WireChange {
  from: number;
  to: number;
  insert: string;
}

export function changesToList(changes: ChangeSet): WireChange[] {
  const out: WireChange[] = [];
  changes.iterChanges((fromA: number, toA: number, _fromB: number, _toB: number, inserted: Text) => {
    out.push({ from: fromA, to: toA, insert: inserted.toString() });
  });
  return out;
}

export function changesToJson(changes: ChangeSet): string {
  return JSON.stringify(changesToList(changes));
}

/** Parse + validate a changes JSON string against a document length. Throws on bad input. */
export function parseChanges(json: string, docLength: number): WireChange[] {
  const raw = JSON.parse(json);
  if (!Array.isArray(raw)) throw new Error("changes must be an array");
  let prevTo = 0;
  const out: WireChange[] = [];
  for (const c of raw) {
    const from = c?.from, to = c?.to;
    const insert = c?.insert ?? "";
    if (!Number.isInteger(from) || !Number.isInteger(to) || typeof insert !== "string")
      throw new Error("malformed change: " + JSON.stringify(c));
    if (from < 0 || to < from || to > docLength)
      throw new Error(`change out of range: ${from}-${to} (doc length ${docLength})`);
    if (from < prevTo) throw new Error("changes must be ascending and non-overlapping");
    prevTo = to;
    out.push({ from, to, insert });
  }
  return out;
}

/** JSON -> CM6 ChangeSpec[] (all positions are in the pre-change document, as CM6 expects). */
export function jsonToChangeSpecs(json: string, docLength: number): ChangeSpec[] {
  return parseChanges(json, docLength).map((c) => ({ from: c.from, to: c.to, insert: c.insert }));
}

/** Reference applier used by tests and documentation: reverse order, slice/concat. */
export function applyWireChanges(text: string, changes: WireChange[]): string {
  let out = text;
  for (let i = changes.length - 1; i >= 0; i--) {
    const c = changes[i];
    out = out.slice(0, c.from) + c.insert + out.slice(c.to);
  }
  return out;
}
