// Selector mapping for the Live theme filter (README "Live theme filter", docs/live-appearance-design.md).
//
// Typora-style themes style real elements: `#write h1 { ... }`, `#write blockquote { ... }`, `#write code { ... }`.
// Live text lines are `.cm-line` divs carrying classes (`fw-h1`, `fw-quote-line`, `fw-list-line`) with inline marks
// as spans (`fw-code`, `fw-live-link`, `fw-strong`, `fw-em`, `fw-strike`), so those rules cannot match. This module
// reads the theme text and emits a *mapped copy* of every rule that can be translated, scoped to Live
// (`#write.fw-mode-live`, MAPPED_SCOPE: filtered AND exact, so "Live follows theme exactly" keeps the theme's
// heading / quote / link typography on Live lines), keeping only typography-level properties. It is pure (no DOM)
// and tolerant: anything it does not understand is skipped, never thrown on.
//
//   h1..h6 -> .cm-line.fw-h1..6      blockquote -> .cm-line.fw-quote-line     li -> .cm-line.fw-list-line
//   ul li -> .fw-list-line.fw-list-ul     ol li -> .fw-list-line.fw-list-ol     (bare li: both kinds)
//   p -> .cm-line (not headings, list, code, table, front matter, math, footnote lines)
//   code (not under pre) -> .fw-code     a -> .fw-live-link     strong, b -> .fw-strong
//   em, i -> .fw-em     del, s, strike -> .fw-strike
//
// Absolute font sizes (px, pt, ...) become `em` relative to the theme's own `#write` size (16px when it sets none),
// so the theme's scale follows the host text size. `blockquote::before/::after` glyphs are deliberately not mapped.
//
// Left alone (the real elements already exist in Live widgets, so the theme reaches them directly): pre, table, img,
// hr, and anything with classes, ids, attributes or structural pseudo-classes on a mapped element.

/** Scope of the editing overlay (livetheme.ts): filtered Live only. */
export const LIVE_SCOPE = "#write.fw-mode-live:not(.fw-live-exact)";
/** Scope of the mapped theme copy: every Live, filtered or exact. */
export const MAPPED_SCOPE = "#write.fw-mode-live";

// ---------------------------------------------------------------- CSS rule splitter
type Node =
  | { kind: "rule"; selector: string; body: string }
  | { kind: "at"; name: string; prelude: string; body: string | null; raw: string };

/** Replace comments with a space; string literals are respected (a `/*` inside quotes is text). */
export function stripComments(css: string): string {
  let out = "";
  for (let i = 0; i < css.length; ) {
    const c = css[i];
    if (c === '"' || c === "'") {
      const end = skipString(css, i);
      out += css.slice(i, end);
      i = end;
    } else if (c === "/" && css[i + 1] === "*") {
      const end = css.indexOf("*/", i + 2);
      out += " ";
      i = end < 0 ? css.length : end + 2;
    } else {
      out += c;
      i++;
    }
  }
  return out;
}

/** Index just after the string literal opening at `i` (an unterminated string runs to the end). */
function skipString(s: string, i: number): number {
  const q = s[i];
  let j = i + 1;
  while (j < s.length && s[j] !== q) j += s[j] === "\\" ? 2 : 1;
  return Math.min(s.length, j + 1);
}

/** Index of the `}` matching the `{` at `open` (or s.length when unbalanced). */
function matchBrace(s: string, open: number): number {
  let depth = 0;
  for (let i = open; i < s.length; i++) {
    const c = s[i];
    if (c === '"' || c === "'") i = skipString(s, i) - 1;
    else if (c === "\\") i++;
    else if (c === "{") depth++;
    else if (c === "}" && --depth === 0) return i;
  }
  return s.length;
}

/** Split already comment-free CSS into top-level rules and at-rules. */
function splitNodes(css: string): Node[] {
  const nodes: Node[] = [];
  let i = 0;
  const n = css.length;
  while (i < n) {
    while (i < n && /\s/.test(css[i])) i++;
    if (i >= n) break;
    if (css[i] === "}") { i++; continue; } // stray closing brace
    const start = i;
    let paren = 0;
    let stop = -1;
    for (; i < n; i++) {
      const c = css[i];
      if (c === '"' || c === "'") i = skipString(css, i) - 1;
      else if (c === "\\") i++;
      else if (c === "(" || c === "[") paren++;
      else if (c === ")" || c === "]") paren = Math.max(0, paren - 1);
      else if (paren === 0 && (c === "{" || c === ";")) { stop = i; break; }
    }
    if (stop < 0) break; // trailing garbage without a block
    const prelude = css.slice(start, stop).trim();
    if (css[stop] === ";") {
      i = stop + 1;
      if (prelude.startsWith("@")) nodes.push({ kind: "at", name: atName(prelude), prelude, body: null, raw: css.slice(start, i) });
      continue;
    }
    const close = matchBrace(css, stop);
    const body = css.slice(stop + 1, close);
    i = Math.min(n, close + 1);
    if (prelude.startsWith("@")) nodes.push({ kind: "at", name: atName(prelude), prelude, body, raw: css.slice(start, i) });
    else if (prelude) nodes.push({ kind: "rule", selector: prelude, body });
  }
  return nodes;
}

function atName(prelude: string): string {
  const m = /^@([\w-]+)/.exec(prelude);
  return m ? m[1].toLowerCase() : "";
}

/** Split on `sep` outside (), [] and strings. */
function splitTop(s: string, sep: string): string[] {
  const out: string[] = [];
  let depth = 0, from = 0;
  for (let i = 0; i < s.length; i++) {
    const c = s[i];
    if (c === '"' || c === "'") i = skipString(s, i) - 1;
    else if (c === "\\") i++;
    else if (c === "(" || c === "[") depth++;
    else if (c === ")" || c === "]") depth = Math.max(0, depth - 1);
    else if (c === sep && depth === 0) { out.push(s.slice(from, i)); from = i + 1; }
  }
  out.push(s.slice(from));
  return out;
}

interface Decl {
  name: string;
  value: string;
  important: boolean;
}

function parseDecls(body: string): Decl[] {
  const out: Decl[] = [];
  for (const piece of splitTop(body, ";")) {
    const colon = piece.indexOf(":");
    if (colon < 1) continue;
    const name = piece.slice(0, colon).trim().toLowerCase();
    let value = piece.slice(colon + 1).trim();
    const important = /\s*!\s*important\s*$/i.test(value);
    if (important) value = value.replace(/\s*!\s*important\s*$/i, "").trim();
    if (!name || !value || /[\s{}]/.test(name)) continue;
    out.push({ name, value, important });
  }
  return out;
}

// ---------------------------------------------------------------- selector mapping
interface Compound {
  tag: string;
  ids: string[];
  classes: string[];
  attrs: string[];
  pseudos: string[]; // pseudo-classes, lower-case, with their argument text
  elements: string[]; // pseudo-elements
}

const IDENT = /^(?:\\.|[\w-]|[^\x00-\x7f])+/;

function parseCompound(raw: string): Compound | null {
  const c: Compound = { tag: "", ids: [], classes: [], attrs: [], pseudos: [], elements: [] };
  let i = 0;
  const tag = /^(?:\*|[A-Za-z][\w-]*)/.exec(raw);
  if (tag) { c.tag = tag[0].toLowerCase(); i = tag[0].length; }
  while (i < raw.length) {
    const ch = raw[i];
    if (ch === "#" || ch === ".") {
      const m = IDENT.exec(raw.slice(i + 1));
      if (!m) return null;
      (ch === "#" ? c.ids : c.classes).push(m[0]);
      i += 1 + m[0].length;
    } else if (ch === "[") {
      let j = i + 1;
      for (; j < raw.length && raw[j] !== "]"; j++) if (raw[j] === '"' || raw[j] === "'") j = skipString(raw, j) - 1;
      if (j >= raw.length) return null;
      c.attrs.push(raw.slice(i, j + 1));
      i = j + 1;
    } else if (ch === ":") {
      const element = raw[i + 1] === ":";
      const m = IDENT.exec(raw.slice(i + (element ? 2 : 1)));
      if (!m) return null;
      let end = i + (element ? 2 : 1) + m[0].length;
      let text = m[0].toLowerCase();
      if (raw[end] === "(") {
        let depth = 0, j = end;
        for (; j < raw.length; j++) {
          if (raw[j] === '"' || raw[j] === "'") j = skipString(raw, j) - 1;
          else if (raw[j] === "(") depth++;
          else if (raw[j] === ")" && --depth === 0) break;
        }
        if (j >= raw.length) return null;
        text += raw.slice(end, j + 1);
        end = j + 1;
      }
      (element ? c.elements : c.pseudos).push(text);
      i = end;
    } else return null;
  }
  return c;
}

/** Descendant / child / sibling combinators: [{comb, raw}] where `comb` is "" for the first compound. */
function splitCompounds(sel: string): { comb: string; raw: string }[] | null {
  const parts: { comb: string; raw: string }[] = [];
  let i = 0, pending = "";
  const n = sel.length;
  while (i < n) {
    const ws = i;
    while (i < n && /\s/.test(sel[i])) i++;
    const sawSpace = i > ws;
    if (i >= n) break;
    if (">+~".includes(sel[i])) {
      if (!parts.length || pending) return null;
      pending = sel[i++];
      continue;
    }
    if (sawSpace && parts.length && !pending) pending = " ";
    const start = i;
    let depth = 0;
    for (; i < n; i++) {
      const c = sel[i];
      if (c === '"' || c === "'") i = skipString(sel, i) - 1;
      else if (c === "\\") i++;
      else if (c === "(" || c === "[") depth++;
      else if (c === ")" || c === "]") depth = Math.max(0, depth - 1);
      else if (depth === 0 && (/\s/.test(c) || ">+~".includes(c))) break;
    }
    parts.push({ comb: parts.length ? pending || " " : "", raw: sel.slice(start, i) });
    pending = "";
  }
  if (pending || !parts.length) return null;
  return parts;
}

type Kind = "heading" | "quote" | "list" | "text" | "inline" | "code";

const BLOCK_TAGS: Record<string, { cls: string; kind: Kind }> = {
  h1: { cls: "fw-h1", kind: "heading" },
  h2: { cls: "fw-h2", kind: "heading" },
  h3: { cls: "fw-h3", kind: "heading" },
  h4: { cls: "fw-h4", kind: "heading" },
  h5: { cls: "fw-h5", kind: "heading" },
  h6: { cls: "fw-h6", kind: "heading" },
  blockquote: { cls: "fw-quote-line", kind: "quote" },
  li: { cls: "fw-list-line", kind: "list" },
  p: { cls: "", kind: "text" },
};
const INLINE_TAGS: Record<string, { cls: string; kind: Kind }> = {
  code: { cls: "fw-code", kind: "code" },
  a: { cls: "fw-live-link", kind: "inline" },
  strong: { cls: "fw-strong", kind: "inline" },
  b: { cls: "fw-strong", kind: "inline" },
  em: { cls: "fw-em", kind: "inline" },
  i: { cls: "fw-em", kind: "inline" },
  del: { cls: "fw-strike", kind: "inline" },
  s: { cls: "fw-strike", kind: "inline" },
  strike: { cls: "fw-strike", kind: "inline" },
};
/** Lines `p` must not reach: they are not paragraph text in the rendered document. */
const NOT_PARAGRAPH =
  ":not(:where(.fw-h1,.fw-h2,.fw-h3,.fw-h4,.fw-h5,.fw-h6,.fw-list-line,.fw-code-line,.fw-table-line," +
  ".fw-front-matter-line,.fw-math-src-line,.fw-footnote-def))";

const STATE_PSEUDOS = new Set(["hover", "focus", "active", "focus-visible", "focus-within"]);

interface Mapped {
  kind: Kind;
  css: string;
  /** Quote lines only: the same selector restricted to depth-1 quotes (for padding-left, see mapRule). */
  cssDepth1?: string;
  /** Quote lines only: the selector restricted to depth 1, 2, 3 and 4 quotes (`.fw-quote-d1` .. `d4`). */
  cssDepths?: string[];
}

/** Map one complex selector to its Live equivalent, or null when it cannot (or should not) be mapped. */
export function mapSelector(selector: string): Mapped | null {
  const parts = splitCompounds(selector.trim());
  if (!parts) return null;
  const compounds: Compound[] = [];
  for (const p of parts) {
    const c = parseCompound(p.raw);
    if (!c) return null;
    compounds.push(c);
  }
  // Only descendant / child combinators survive; a child step to a mapped element is looser as a descendant.
  if (parts.some((p) => p.comb === "+" || p.comb === "~")) return null;

  let scope = MAPPED_SCOPE;
  let first = 0;
  const head = compounds[0];
  if (head.tag === "" && head.ids.length === 1 && head.ids[0] === "write") {
    if (head.attrs.length || head.pseudos.length || head.elements.length) return null;
    scope += head.classes.map((c) => "." + c).join("");
    first = 1;
  }
  const rest = compounds.slice(first);
  if (!rest.length) return null; // a bare `#write` rule already applies

  const blockClasses: string[] = [];
  let sawP = false, sawBlock = false;
  const inline: { cls: string; kind: Kind }[] = [];
  let subjectKind: Kind = "text";
  let subjectPseudos: string[] = [];
  let listType = ""; // "ul" | "ol": the nearest list ancestor named before an `li`

  for (let k = 0; k < rest.length; k++) {
    const c = rest[k];
    const last = k === rest.length - 1;
    if (c.ids.length || c.classes.length || c.attrs.length || c.elements.length) return null;
    const keep: string[] = [];
    for (const ps of c.pseudos) {
      if (!last) return null; // state on an ancestor (`a:hover code`) has no Live meaning
      if (STATE_PSEUDOS.has(ps)) keep.push(ps);
      else if (ps === "link" || ps === "any-link") { if (c.tag !== "a") return null; } // every Live link is a link
      else return null; // :visited, :first-child, :not(...), :nth-*, ...
    }
    if (c.tag === "ul" || c.tag === "ol") {
      if (last || sawBlock || inline.length || keep.length) return null;
      listType = c.tag;
      continue;
    }
    const block = BLOCK_TAGS[c.tag];
    const inl = INLINE_TAGS[c.tag];
    if (block) {
      if (inline.length) return null; // a block inside an inline element
      sawBlock = true;
      if (block.cls) { if (!blockClasses.includes(block.cls)) blockClasses.push(block.cls); } else sawP = true;
      if (c.tag === "li" && listType && !blockClasses.includes("fw-list-" + listType)) blockClasses.push("fw-list-" + listType);
      if (last) { subjectKind = block.kind; subjectPseudos = keep; }
    } else if (inl) {
      inline.push(inl);
      if (last) { subjectKind = inl.kind; subjectPseudos = keep; }
    } else return null; // pre, table, img, hr, div, span, *, ...
  }
  if (inline.length && !sawBlock && rest.some((c) => c.tag === "ul" || c.tag === "ol")) return null;

  const build = (extra: string) => {
    let out = scope;
    if (sawBlock) {
      out += " .cm-line" + blockClasses.map((c) => "." + c).join("") + extra;
      if (!blockClasses.length && sawP) out += NOT_PARAGRAPH;
      if (!inline.length) out += subjectPseudos.map((p) => ":" + p).join("");
    }
    if (inline.length) {
      // pseudo-classes attach to the last inline compound
      const spans = inline.map((x) => "." + x.cls);
      spans[spans.length - 1] += subjectPseudos.map((p) => ":" + p).join("");
      out += " " + spans.join(" ");
    }
    return out;
  };
  const mapped: Mapped = { kind: subjectKind, css: build("") };
  if (subjectKind === "quote") {
    mapped.cssDepths = [1, 2, 3, 4].map((n) => build(".fw-quote-d" + n));
    mapped.cssDepth1 = mapped.cssDepths[0];
  }
  return mapped;
}

// ---------------------------------------------------------------- declaration filter
const TYPOGRAPHY = /^(?:font(?:-[a-z-]+)?|color|line-height|letter-spacing|word-spacing|text-decoration(?:-[a-z-]+)?|text-transform|text-underline-offset|text-underline-position)$/;
const BORDER_LEFT = /^border-left(?:-(?:width|style|color))?$/;
const BORDER_SIDES = /^border-(?:left|bottom)(?:-(?:width|style|color))?$/;
const ABSOLUTE_LENGTH = /\d\s*(?:px|pt|pc|cm|mm|in|q)\b/i;
const ABS_UNITS = "px|pt|pc|in|cm|mm|q";
const SIMPLE_ABSOLUTE = new RegExp(`^(\\d*\\.?\\d+)(${ABS_UNITS})$`, "i");
const PX_PER: Record<string, number> = { px: 1, pt: 4 / 3, pc: 16, in: 96, cm: 96 / 2.54, mm: 96 / 25.4, q: 96 / 101.6 };
/** The size the theme's absolute lengths are relative to when its `#write` sets none (a browser's default). */
export const DEFAULT_BASE_PX = 16;

const toPx = (num: string, unit: string) => parseFloat(num) * (PX_PER[unit.toLowerCase()] ?? 1);
const fmtEm = (n: number) => `${parseFloat(n.toFixed(4))}em`;

/** Per-theme facts the declaration filter needs. */
interface Ctx {
  /** The theme's own `#write` font size in px (16 when it sets none or not in absolute units). */
  basePx: number;
}

/** The theme's `#write { font-size }` in px when it is absolute (last declaration wins), else the default. */
function themeBasePx(nodes: Node[]): number {
  let base = DEFAULT_BASE_PX;
  for (const n of nodes) {
    if (n.kind !== "rule" || !splitTop(n.selector, ",").some((s) => s.trim() === "#write")) continue;
    for (const d of parseDecls(n.body)) {
      if (d.name !== "font-size") continue;
      const m = SIMPLE_ABSOLUTE.exec(d.value.trim());
      const px = m ? toPx(m[1], m[2]) : 0;
      base = px > 0 ? px : DEFAULT_BASE_PX;
    }
  }
  return base;
}

/** `font-size` / `font` value with `rem` -> `em` and the absolute size (px, pt, ...) -> `em` against the base. */
function relativeFontSize(name: string, value: string, basePx: number): string {
  value = value.replace(/(-?\d*\.?\d+)rem\b/gi, "$1em");
  if (name === "font-size") {
    const m = SIMPLE_ABSOLUTE.exec(value.trim());
    return m ? fmtEm(toPx(m[1], m[2]) / basePx) : value;
  }
  // `font` shorthand: the first absolute length is the size (weights are unitless; line-height comes after `/`)
  return value.replace(new RegExp(`(^|\\s)(\\d*\\.?\\d+)(${ABS_UNITS})(?=\\/|\\s|$)`, "i"), (_all, pre, n, u) => pre + fmtEm(toPx(n, u) / basePx));
}

/** A non-negative length (or a calc / min / max / clamp of them) usable as a left spacing value. */
function leftLength(v: string, allowVar: boolean): boolean {
  v = v.trim();
  if (/^\+?(?:\d*\.?\d+(?:[a-z]+|%)|0)$/i.test(v)) return true;
  if (/^(?:calc|min|max|clamp)\(.*\)$/i.test(v)) return !/url\(/i.test(v);
  return allowVar && /^var\(.*\)$/i.test(v);
}

/** The left value of a `padding` shorthand (1-4 values, `var()` not understood), or null. */
function paddingShorthandLeft(value: string): string | null {
  if (/var\(/i.test(value)) return null;
  const t = splitTop(value.trim().replace(/\s+/g, " "), " ").filter(Boolean);
  if (!t.length || t.length > 4) return null;
  const left = t.length === 4 ? t[3] : t.length === 1 ? t[0] : t[1];
  return leftLength(left, false) ? left : null;
}

/** `v` plus `n`em, as a CSS value (a zero stays a plain `Nem`). */
function plusEm(v: string, n: number): string {
  return /^\+?0*\.?0*[a-z%]*$/i.test(v.trim()) && parseFloat(v) === 0 ? `${n}em` : `calc(${v} + ${n}em)`;
}

/** The padding-left a quote rule asks for (`padding`, `padding-left`; the last valid declaration wins), or null. */
function quotePaddingLeft(decls: Decl[]): string | null {
  let left: string | null = null;
  for (const d of decls) {
    if (/url\(|image-set\(|expression\(/i.test(d.value)) continue;
    const v = d.name === "padding-left" ? (leftLength(d.value, true) ? d.value.trim() : null) : d.name === "padding" ? paddingShorthandLeft(d.value) : null;
    if (v !== null) left = v;
  }
  return left;
}

/** The declaration as emitted in a mapped copy (`name: value[ !important]`), or null when it is dropped. */
function mapDecl(d: Decl, kind: Kind, ctx: Ctx): string | null {
  let { name, value } = d;
  if (/url\(|image-set\(|expression\(/i.test(value)) return null;
  const emit = (n: string, v: string) => `${n}: ${v}${d.important ? " !important" : ""}`;
  if (name === "font-size" || name === "font") {
    // Relative sizes keep following the host base size: rem would not, so it becomes em, and an absolute size
    // becomes em against the theme's own base size.
    value = relativeFontSize(name, value, ctx.basePx);
    // Whatever absolute length is left (inside a calc(), say) cannot follow the host size on body text: dropped.
    if ((kind === "text" || kind === "list") && ABSOLUTE_LENGTH.test(value)) return null;
  }
  if (TYPOGRAPHY.test(name)) return emit(name, value);
  if (kind === "heading" && BORDER_SIDES.test(name)) return emit(name, value);
  if (kind === "quote" && BORDER_LEFT.test(name)) return emit(name, value);
  if (kind === "quote" && name === "margin-left" && leftLength(value, true)) return emit(name, value.trim());
  if (kind === "code" || kind === "quote") {
    if (name === "background-color") return emit(name, value);
    // `background: <colour>` only (no position / size / repeat / images)
    if (name === "background" && !/gradient\(/i.test(value) && splitTop(value.replace(/\s+/g, " "), " ").length === 1 && !/^(?:none|inherit|initial|unset)$/i.test(value))
      return emit("background-color", value);
  }
  return null;
}

// ---------------------------------------------------------------- rules
const KIND_ORDER: Kind[] = ["heading", "quote", "list", "text", "inline", "code"];

function mapRule(selectorList: string, body: string, ctx: Ctx): string[] {
  if (body.includes("{")) return []; // nested rules (CSS nesting) are not understood: skip
  const mapped: Mapped[] = [];
  for (const sel of splitTop(selectorList, ",")) {
    const m = mapSelector(sel);
    if (m) mapped.push(m);
  }
  if (!mapped.length) return [];
  const decls = parseDecls(body);
  const out: string[] = [];
  for (const kind of KIND_ORDER) {
    const group = mapped.filter((m) => m.kind === kind);
    if (!group.length) continue;
    const props = decls.map((d) => mapDecl(d, kind, ctx)).filter((x): x is string => x !== null);
    const selectors = [...new Set(group.map((m) => m.css))];
    if (props.length) out.push(`${selectors.join(", ")} { ${props.join("; ")}; }`);
    if (kind === "quote") {
      // The editor indents quote lines with padding-left !important (and deeper levels by depth), so a theme's left
      // padding (`padding-left`, or the left value of `padding`) is applied with !important: as is at depth 1 and
      // one em more per deeper level, which keeps nested quotes indented past their parent. Top, right and bottom
      // padding are never taken.
      const left = quotePaddingLeft(decls);
      if (left !== null) {
        for (let depth = 1; depth <= 4; depth++) {
          const sels = [...new Set(group.map((m) => m.cssDepths![depth - 1]))];
          out.push(`${sels.join(", ")} { padding-left: ${depth === 1 ? left : plusEm(left, depth - 1)} !important; }`);
        }
      }
    }
  }
  return out;
}

function mapNodes(nodes: Node[], ctx: Ctx): string[] {
  const out: string[] = [];
  for (const n of nodes) {
    if (n.kind === "rule") {
      out.push(...mapRule(n.selector, n.body, ctx));
    } else if (n.name === "font-face" || n.name === "keyframes" || n.name === "-webkit-keyframes") {
      out.push(n.raw.trim()); // passed through untouched
    } else if ((n.name === "media" || n.name === "supports") && n.body !== null) {
      const inner = mapNodes(splitNodes(n.body), ctx);
      if (inner.length) out.push(`@${n.name} ${n.prelude.slice(n.name.length + 1).trim()} {\n${inner.map((r) => "  " + r).join("\n")}\n}`);
    }
    // @import, @charset, @page, @layer, @namespace, ... : dropped
  }
  return out;
}

/**
 * Mapped, Live-scoped copy of the theme's text rules (see the header comment). Returns "" when nothing maps.
 * Never throws; `@font-face` / `@keyframes` blocks are passed through untouched, `@media` / `@supports` are
 * descended into, everything else at-rule is ignored.
 */
export function mapThemeCss(css: string): string {
  if (!css || typeof css !== "string") return "";
  try {
    const nodes = splitNodes(stripComments(css));
    return mapNodes(nodes, { basePx: themeBasePx(nodes) }).join("\n");
  } catch {
    return "";
  }
}
