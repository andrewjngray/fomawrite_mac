// In-page link editor (Typora-style "Insert Link"). A CodeMirror panel (the same mechanism as the search panel:
// `showPanel` driven by a StateField) with Label / URL / Title fields. Opened by the host command "link" and by
// Mod-k. Everything that decides what text is written is a pure function over `EditorState`, so it is unit-tested
// without a DOM; the panel itself is a thin shell around them. See README "Links".
import { EditorSelection, EditorState, Extension, Prec, StateEffect, StateField, TransactionSpec } from "@codemirror/state";
import { EditorView, Panel, keymap, showPanel } from "@codemirror/view";
import { ensureSyntaxTree, syntaxTree } from "@codemirror/language";
import { encodeImagePath } from "./images";

// ---------------------------------------------------------------- text helpers (pure)

/** A URL that starts with a scheme (`https:`, `mailto:`, `tel:` ...) is an absolute link: kept as typed apart from spaces/parentheses. */
export function hasScheme(url: string): boolean {
  return /^[A-Za-z][A-Za-z0-9+.-]*:/.test(url);
}

/**
 * The link destination as written into the Markdown. Whitespace and parentheses are always percent-encoded (they would end
 * the destination); a URL with a scheme is otherwise kept as typed (so `%20` already in it is not double-encoded), while a
 * relative path is encoded exactly like an image path (`%` becomes `%25` too).
 */
export function encodeLinkUrl(url: string): string {
  const u = url.trim();
  if (!hasScheme(u)) return encodeImagePath(u);
  return u.replace(/[()\s]/g, (c) => (c.charCodeAt(0) < 128 ? "%" + c.charCodeAt(0).toString(16).toUpperCase().padStart(2, "0") : encodeURIComponent(c)));
}

/** Inverse of `encodeLinkUrl` as far as the field is concerned: shows `%20 %28 %29` (and `%25` for relative paths) as typed characters. */
export function decodeLinkUrl(raw: string): string {
  let u = raw.trim();
  if (u.startsWith("<") && u.endsWith(">") && u.length >= 2) u = u.slice(1, -1);
  const decode = hasScheme(u) ? /%(20|28|29)/g : /%(20|28|29|25)/g;
  return u.replace(decode, (_m, h: string) => String.fromCharCode(parseInt(h, 16)));
}

/** Brackets in a label would end the link text early: backslash-escape them. Line breaks become spaces. */
export function escapeLabel(label: string): string {
  return label.replace(/\s*[\r\n]+\s*/g, " ").replace(/[[\]]/g, (c) => "\\" + c);
}
export function unescapeLabel(raw: string): string {
  return raw.replace(/\\([[\]])/g, "$1");
}

/** `"title"` with `"` and `\` escaped; line breaks become spaces. */
export function quoteTitle(title: string): string {
  return '"' + title.replace(/\s*[\r\n]+\s*/g, " ").replace(/[\\"]/g, (c) => "\\" + c) + '"';
}
/** Title as shown in the field, from its raw source (`"t"`, `'t'` or `(t)`). */
export function unquoteTitle(raw: string): string {
  if (raw.length < 2) return raw;
  return raw.slice(1, -1).replace(/\\([\\"'()])/g, "$1");
}

export interface LinkValues { label: string; url: string; title: string }

/** `[label](url "title")`; the title is omitted when empty. An empty label falls back to the URL text. */
export function buildLinkMarkdown(v: LinkValues): string {
  const url = v.url.trim();
  const label = v.label.trim() === "" ? url : v.label;
  const title = v.title.trim();
  return `[${escapeLabel(label)}](${encodeLinkUrl(url)}${title ? " " + quoteTitle(title) : ""})`;
}

// ---------------------------------------------------------------- link detection (pure over EditorState)

/** What the panel is editing. `existing` = replacing a link already in the document (`from..to` is the whole `[..](..)`). */
export interface LinkContext extends LinkValues {
  from: number;
  to: number;
  existing: boolean;
  /** Existing link only: the label exactly as written in the document (what Remove leaves behind). */
  rawLabel: string;
}

export interface LinkNodeInfo extends LinkValues { from: number; to: number; rawLabel: string }

/**
 * The inline Markdown link `[label](url "title")` that the range `from..to` is inside of or touches (both ends inclusive), or
 * null. Reference links (`[a][b]`, `[a]`), images and autolinks are not inline links and are ignored. When several links qualify
 * (a selection spanning two), the one containing the range's head wins, else the first.
 */
export function findLinkAt(state: EditorState, from: number, to: number, head: number = to): LinkNodeInfo | null {
  const doc = state.doc;
  const tree = ensureSyntaxTree(state, Math.min(doc.length, to + 2000), 50) ?? syntaxTree(state);
  const found: LinkNodeInfo[] = [];
  tree.iterate({
    from: Math.max(0, from - 1),
    to: Math.min(doc.length, to + 1),
    enter(n) {
      if (n.name !== "Link" || n.from > to || n.to < from) return;
      const info = readLink(state, n.node);
      if (info) found.push(info);
    },
  });
  return found.find((l) => l.from <= head && head <= l.to) ?? found[0] ?? null;
}

interface LinkSyntaxNode { from: number; to: number; firstChild: LinkSyntaxNode | null; nextSibling: LinkSyntaxNode | null; name: string }

function readLink(state: EditorState, node: LinkSyntaxNode): LinkNodeInfo | null {
  const marks: { from: number; to: number }[] = [];
  let url: { from: number; to: number } | null = null, title: { from: number; to: number } | null = null;
  for (let c = node.firstChild; c; c = c.nextSibling) {
    if (c.name === "LinkMark") marks.push({ from: c.from, to: c.to });
    else if (c.name === "URL") url = { from: c.from, to: c.to };
    else if (c.name === "LinkTitle") title = { from: c.from, to: c.to };
    else if (c.name === "LinkLabel") return null; // reference link
  }
  // `[` `]` `(` `)` - anything else is a shortcut reference (`[a]`) or an unfinished link.
  if (marks.length !== 4) return null;
  const doc = state.doc;
  if (doc.sliceString(marks[0].from, marks[0].to) !== "[" || doc.sliceString(marks[2].from, marks[2].to) !== "(") return null;
  const rawLabel = doc.sliceString(marks[0].to, marks[1].from);
  return {
    from: node.from,
    to: node.to,
    rawLabel,
    label: unescapeLabel(rawLabel),
    url: url ? decodeLinkUrl(doc.sliceString(url.from, url.to)) : "",
    title: title ? unquoteTitle(doc.sliceString(title.from, title.to)) : "",
  };
}

/**
 * What the panel should edit for the main selection: the existing link it is inside/touching (fields prefilled from it), else
 * the selection (Label = selected text when it is on a single line; a multi-line selection is not consumed: the link is inserted
 * at its end with an empty Label, so no text is lost).
 */
export function linkContext(state: EditorState): LinkContext {
  const sel = state.selection.main;
  const link = findLinkAt(state, sel.from, sel.to, sel.head);
  if (link) return { ...link, existing: true };
  const text = state.doc.sliceString(sel.from, sel.to);
  if (text.includes("\n") || text.includes("\r")) return { from: sel.to, to: sel.to, label: "", url: "", title: "", existing: false, rawLabel: "" };
  return { from: sel.from, to: sel.to, label: text, url: "", title: "", existing: false, rawLabel: "" };
}

// ---------------------------------------------------------------- edits (pure)

/** The change a replacement makes and where the caret goes (just behind the new text). */
export interface Replacement { from: number; to: number; insert: string; caret: number }

/** Apply: replace `ctx.from..ctx.to` with the Markdown for `values`. Null when the URL is empty (nothing sensible to write). */
export function applyReplacement(ctx: LinkContext, values: LinkValues): Replacement | null {
  if (values.url.trim() === "") return null;
  const insert = buildLinkMarkdown(values);
  return { from: ctx.from, to: ctx.to, insert, caret: ctx.from + insert.length };
}

/** Remove: replace the whole existing link with its label text. Null when not editing an existing link. */
export function removeReplacement(ctx: LinkContext): Replacement | null {
  if (!ctx.existing) return null;
  return { from: ctx.from, to: ctx.to, insert: ctx.rawLabel, caret: ctx.from + ctx.rawLabel.length };
}

/** An ordinary user transaction (reported to the host, undoable) for a replacement; null when it would change nothing. */
export function replacementSpec(state: EditorState, r: Replacement, userEvent: "input.link" | "delete.link"): TransactionSpec | null {
  if (state.doc.sliceString(r.from, r.to) === r.insert) return null;
  return { changes: { from: r.from, to: r.to, insert: r.insert }, selection: EditorSelection.cursor(r.caret), scrollIntoView: true, userEvent };
}

// ---------------------------------------------------------------- panel state

export const openLinkEffect = StateEffect.define<LinkContext>();
export const closeLinkEffect = StateEffect.define<null>();

export const linkField = StateField.define<LinkContext | null>({
  create: () => null,
  update(value, tr) {
    for (const e of tr.effects) {
      if (e.is(openLinkEffect)) value = e.value;
      else if (e.is(closeLinkEffect)) value = null;
    }
    return value;
  },
  provide: (f) => showPanel.from(f, (v) => (v ? createLinkPanel : null)),
});

export function openLinkPanel(view: EditorView): boolean {
  view.dispatch({ effects: openLinkEffect.of(linkContext(view.state)) });
  return true;
}

export function closeLinkPanel(view: EditorView): boolean {
  if (!view.state.field(linkField, false)) return false;
  view.dispatch({ effects: closeLinkEffect.of(null) });
  view.focus();
  return true;
}

// ---------------------------------------------------------------- the panel (DOM)

function createLinkPanel(view: EditorView): Panel {
  let ctx = view.state.field(linkField)!;
  const dom = document.createElement("div");
  dom.className = "cm-panel fw-link-panel";
  dom.setAttribute("role", "group");
  dom.setAttribute("aria-label", "Edit link");

  const field = (name: string, label: string, placeholder: string) => {
    const wrap = document.createElement("label");
    wrap.className = "fw-link-field fw-link-field-" + name;
    const text = document.createElement("span");
    text.textContent = label;
    const input = document.createElement("input");
    input.className = "cm-textfield";
    input.name = name;
    input.type = "text";
    input.placeholder = placeholder;
    input.spellcheck = false;
    input.autocomplete = "off";
    input.setAttribute("autocapitalize", "off");
    wrap.append(text, input);
    dom.append(wrap);
    return input;
  };
  const label = field("label", "Label", "Link text");
  const url = field("url", "URL", "https://");
  const title = field("title", "Title", "optional");

  const button = (name: string, text: string, onClick: () => void) => {
    const b = document.createElement("button");
    b.className = "cm-button fw-link-" + name;
    b.type = "button";
    b.name = name;
    b.textContent = text;
    b.addEventListener("click", onClick);
    return b;
  };
  const values = (): LinkValues => ({ label: label.value, url: url.value, title: title.value });

  const finish = (spec: TransactionSpec | null) => {
    view.dispatch({ ...(spec ?? {}), effects: closeLinkEffect.of(null) });
    view.focus();
  };
  const apply = () => {
    const r = applyReplacement(ctx, values());
    if (!r) { url.classList.add("fw-invalid"); url.focus(); return; } // a link needs a destination
    finish(replacementSpec(view.state, r, "input.link"));
  };
  const remove = () => {
    const r = removeReplacement(ctx);
    if (r) finish(replacementSpec(view.state, r, "delete.link"));
  };
  const cancel = () => finish(null);

  const applyBtn = button("apply", "Apply", apply);
  const removeBtn = button("remove", "Remove", remove);
  const cancelBtn = button("cancel", "Cancel", cancel);
  dom.append(applyBtn, removeBtn, cancelBtn);

  url.addEventListener("input", () => url.classList.remove("fw-invalid"));
  dom.addEventListener("keydown", (e) => {
    if (e.key === "Escape") { e.preventDefault(); e.stopPropagation(); cancel(); }
    else if (e.key === "Enter" && !e.isComposing && (e.target as HTMLElement).tagName === "INPUT") { e.preventDefault(); apply(); }
  });

  const fill = () => {
    label.value = ctx.label;
    url.value = ctx.url;
    title.value = ctx.title;
    url.classList.remove("fw-invalid");
    removeBtn.hidden = !ctx.existing;
    dom.classList.toggle("fw-link-existing", ctx.existing);
  };
  const focusFirst = () => {
    const el = label.value === "" ? label : url;
    el.focus();
    el.select();
  };
  fill();

  return {
    dom,
    top: true,
    mount: focusFirst,
    update(u) {
      const next = u.state.field(linkField, false);
      if (next && next !== ctx) { ctx = next; fill(); focusFirst(); } // Mod-k again while open: re-read the selection
    },
  };
}

// ---------------------------------------------------------------- the extension

export function linksExtension(): Extension {
  return [
    linkField,
    Prec.high(keymap.of([{ key: "Mod-k", run: openLinkPanel, preventDefault: true }])),
  ];
}
