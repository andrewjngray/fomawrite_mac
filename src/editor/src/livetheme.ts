// Live mode follows the publishing theme, filtered (docs/live-appearance-design.md in the repo root).
//
// The host pushes the theme CSS (rules under `#write`) with `setTheme`; it is applied unchanged, in both modes.
// In Live this module adds an *editing overlay* stylesheet, `<style id="fomawrite-live-overlay">`, placed AFTER
// the theme element so it wins on precedence. It neutralises the theme's page layout and keeps its typography.
// The mapped copy of the theme's element rules (thememap.ts) is separate and present in exact mode too.
// Everything here is DOM-free at import time; the DOM helpers take a Document so tests can pass a tiny fake.

import { LIVE_SCOPE, mapThemeCss } from "./thememap";

/** The theme element written by `applyThemeDom` (modes.ts). */
export const THEME_ID = "fomawrite-theme";
/** Mapped copy of the theme's element rules for Live text lines (thememap.ts); sits between the theme and the overlay. */
export const MAPPED_ID = "fomawrite-theme-mapped";
export const LIVE_OVERLAY_ID = "fomawrite-live-overlay";
/** Class on <html> while Live is active with the filter on (the overlay's `html.fw-live-filtered body` rule keys on it). */
export const LIVE_HTML_CLASS = "fw-live-filtered";
/** Class on #write once the host has sent a numeric `fontSize` (the overlay lets it beat the theme's `#write { font-size }`). */
export const HOST_SIZE_CLASS = "fw-host-size";
/** Class on #write when the overlay is switched off (`liveThemeFilter: false`, "Live follows theme exactly"). */
export const LIVE_EXACT_CLASS = "fw-live-exact";
export const FILL_BG_CLASS = "fw-live-fill-bg";
export const FILL_FG_CLASS = "fw-live-fill-fg";

/*
 * What survives and what does not (matches the design note's table). All rules are scoped to
 * `#write.fw-mode-live:not(.fw-live-exact)`, so Source is never affected and the overlay is inert when off.
 *
 * KEPT (never mentioned below, so the theme's value stands): font-family, font-size, font-weight, font-style,
 *   color, line-height, letter-spacing, text-decoration, list markers (list-style, li::marker, li::before),
 *   heading styles, code and blockquote colouring and borders, table borders / cell alignment / cell backgrounds,
 *   link colour. NOT kept: margins and padding (a Live line is a `.cm-line`, and thememap.ts never copies margin or
 *   padding, bar the depth-1 quote indent) and block backgrounds (only inline `code` keeps its background-color;
 *   blockquote / pre / paragraph backgrounds are neither mapped nor reach Live lines).
 * NEUTRALISED, with !important:
 *   - #write: position -> static; float -> none; columns -> auto; transform, box-shadow, background-image, filter
 *     -> none; display block, box-sizing border-box, no border / radius / outline, min-height / height auto,
 *     opacity 1, zoom 1, overflow visible; width / max-width / margin / padding -> the editor's own measure (760px
 *     column, 48px 40px 40vh padding, 50vh top/bottom in typewriter mode, i.e. the values in editor.css);
 *     ::before / ::after furniture removed.
 *   - #write > * (theme wrappers, #editor): position static, float none, columns auto, transform / box-shadow /
 *     background-image none, width auto, max-width 100%, no page margin or padding.
 *   - block containers inside (p, ul, ol, blockquote, pre, table, figure, section, ...): float none, columns auto,
 *     transform / box-shadow / background-image none, max-width 100%. `position` is reset on the block-level
 *     furniture (pre, table, figure, img, hr, section, article, aside, header, footer, nav, main, details) but NOT on
 *     li, blockquote, headings or paragraphs, whose ::before/::after are often list markers or quote glyphs
 *     positioned against them. The rendered table widget (.fw-table) is forced to the full measure (width 100%).
 * HOST SIZE: once the host has sent `fontSize` (class fw-host-size on #write), `--fw-font-size` beats a theme's
 *   `#write { font-size }`; the theme's heading sizes stay in em, so its scale still follows. Without a host size the
 *   theme's size stands.
 * FONT / LINE HEIGHT: nothing here. The host's fontFamily / lineHeight are fallbacks declared at zero specificity
 *   on <html> (editor.css, `:where(html.fw-live-root)`), so a face or line height the theme sets on html, body or
 *   #write wins; the host value applies only when the theme sets none.
 * <body>: `html.fw-live-filtered body` (the class is on <html> only in filtered Live) resets margin, padding, width,
 *   max-width, display, position, transform, background-image and columns; colours are left alone.
 * GUARANTEES: the caret follows the text colour; readability (see applyLiveTheme): when the theme sets no
 *   background and/or text colour anywhere #write inherits from or paints on (#write, body, html), the host palette
 *   fills the missing one through --fw-live-bg / --fw-live-fg, chosen so the contrast is at least 4.5:1.
 *
 * Not covered: rules on html/body other than the reset above; use the exact switch then. Element selectors
 *   (`#write h1`, `blockquote`, `code`, `a`, ...) are mapped onto Live's line / mark classes by thememap.ts.
 */
const S = LIVE_SCOPE;
const BLOCKS = "p, ul, ol, blockquote, pre, table, figure, section, article, aside, header, footer, nav, main, details";
const POSITIONED = "pre, table, figure, img, hr, section, article, aside, header, footer, nav, main, details";

export const LIVE_OVERLAY_CSS = `/* Fomawrite live editing overlay: layout neutralised, typography kept. See src/livetheme.ts. */
${S} {
  position: static !important;
  float: none !important;
  columns: auto !important;
  column-count: auto !important;
  transform: none !important;
  box-shadow: none !important;
  background-image: none !important;
  filter: none !important;
  display: block !important;
  box-sizing: border-box !important;
  border: none !important;
  border-radius: 0 !important;
  outline: none !important;
  opacity: 1 !important;
  zoom: 1 !important;
  overflow: visible !important;
  min-height: auto !important;
  height: auto !important;
  width: auto !important;
  max-width: var(--fw-live-measure, 760px) !important;
  margin: 0 auto !important;
  padding: 48px 40px 40vh !important;
}
${S}.fw-typewriter { padding-top: 50vh !important; padding-bottom: 50vh !important; }
${S}::before, ${S}::after { content: none !important; display: none !important; }
${S} > * {
  position: static !important;
  float: none !important;
  columns: auto !important;
  column-count: auto !important;
  transform: none !important;
  box-shadow: none !important;
  background-image: none !important;
  width: auto !important;
  max-width: 100% !important;
  margin: 0 !important;
  padding: 0 !important;
}
${S} :is(${BLOCKS}) {
  float: none !important;
  columns: auto !important;
  column-count: auto !important;
  transform: none !important;
  box-shadow: none !important;
  background-image: none !important;
  max-width: 100% !important;
}
${S} :is(${POSITIONED}) { position: static !important; }
${S} img { float: none !important; max-width: 100% !important; }
${S} .fw-table { width: 100% !important; }
${S}.${HOST_SIZE_CLASS} { font-size: var(--fw-font-size) !important; }
html.${LIVE_HTML_CLASS} body {
  margin: 0 !important;
  padding: 0 !important;
  width: auto !important;
  max-width: none !important;
  display: block !important;
  position: static !important;
  transform: none !important;
  background-image: none !important;
  columns: auto !important;
  column-count: auto !important;
}
${S}.${FILL_BG_CLASS} { background-color: var(--fw-live-bg) !important; }
${S}.${FILL_FG_CLASS} { color: var(--fw-live-fg) !important; }
${S} .cm-content { caret-color: currentColor; }
${S} .cm-cursor, ${S} .cm-dropCursor { border-left-color: currentColor !important; }
`;

// ---------------------------------------------------------------- readability (pure)
export interface Palette {
  background: string;
  text: string;
}

/** `{ background, text }` of non-empty strings, else undefined. */
export function parsePalette(raw: unknown): Palette | undefined {
  if (typeof raw !== "object" || raw === null) return undefined;
  const o = raw as Record<string, unknown>;
  if (typeof o.background !== "string" || typeof o.text !== "string") return undefined;
  const background = o.background.trim(), text = o.text.trim();
  return background && text ? { background, text } : undefined;
}

/** True for an absent colour: empty, `transparent`, or any rgba()/hsla() with zero alpha (getComputedStyle reports
 *  an unset background as `rgba(0, 0, 0, 0)`). */
export function isUnsetColour(c: string | null | undefined): boolean {
  if (c === null || c === undefined) return true;
  const s = c.trim().toLowerCase();
  if (s === "" || s === "transparent") return true;
  const m = /^(?:rgb|hsl)a?\(([^)]*)\)$/.exec(s);
  if (!m) return false;
  const parts = m[1].split(/[\s,/]+/).filter(Boolean);
  return parts.length === 4 && parseFloat(parts[3]) === 0;
}

// ---------------------------------------------------------------- colour maths (pure)
export interface Rgba { r: number; g: number; b: number; a: number }

const NAMED: Record<string, string> = { white: "#ffffff", black: "#000000" };

/** Parse `#rgb`, `#rgba`, `#rrggbb`, `#rrggbbaa`, `rgb()` / `rgba()` (comma or space syntax, numbers or percentages),
 *  `white`, `black` and `transparent`. Anything else (hsl(), oklch(), color(), var(), ...) is null. */
export function parseColour(c: string | null | undefined): Rgba | null {
  if (typeof c !== "string") return null;
  let s = c.trim().toLowerCase();
  if (s === "transparent") return { r: 0, g: 0, b: 0, a: 0 };
  s = NAMED[s] ?? s;
  let m = /^#([0-9a-f]{3,8})$/.exec(s);
  if (m) {
    let h = m[1];
    if (h.length === 3 || h.length === 4) h = [...h].map((x) => x + x).join("");
    if (h.length !== 6 && h.length !== 8) return null;
    const n = (i: number) => parseInt(h.slice(i, i + 2), 16);
    return { r: n(0), g: n(2), b: n(4), a: h.length === 8 ? n(6) / 255 : 1 };
  }
  m = /^rgba?\(([^)]*)\)$/.exec(s);
  if (!m) return null;
  const parts = m[1].split(/[\s,/]+/).filter(Boolean);
  if (parts.length < 3 || parts.length > 4) return null;
  const chan = (p: string) => (p.endsWith("%") ? (parseFloat(p) / 100) * 255 : parseFloat(p));
  const [r, g, b] = [chan(parts[0]), chan(parts[1]), chan(parts[2])];
  const a = parts.length === 4 ? (parts[3].endsWith("%") ? parseFloat(parts[3]) / 100 : parseFloat(parts[3])) : 1;
  if ([r, g, b, a].some((x) => Number.isNaN(x))) return null;
  const clamp = (x: number, hi: number) => Math.min(hi, Math.max(0, x));
  return { r: clamp(r, 255), g: clamp(g, 255), b: clamp(b, 255), a: clamp(a, 1) };
}

/** WCAG relative luminance (0 black .. 1 white) of a CSS colour; NaN when it cannot be parsed. Alpha is ignored. */
export function relativeLuminance(c: string | Rgba | null | undefined): number {
  const rgb = typeof c === "string" || c == null ? parseColour(c) : c;
  if (!rgb) return NaN;
  const lin = (v: number) => {
    const x = v / 255;
    return x <= 0.04045 ? x / 12.92 : Math.pow((x + 0.055) / 1.055, 2.4);
  };
  return 0.2126 * lin(rgb.r) + 0.7152 * lin(rgb.g) + 0.0722 * lin(rgb.b);
}

/** WCAG contrast ratio (1 .. 21) between two CSS colours; NaN when either cannot be parsed. */
export function contrastRatio(a: string | Rgba | null | undefined, b: string | Rgba | null | undefined): number {
  const la = relativeLuminance(a), lb = relativeLuminance(b);
  if (Number.isNaN(la) || Number.isNaN(lb)) return NaN;
  return (Math.max(la, lb) + 0.05) / (Math.min(la, lb) + 0.05);
}

/** Minimum contrast (WCAG AA body text) the readability fill guarantees. */
export const MIN_CONTRAST = 4.5;
export const FALLBACK_DARK = "#111";
export const FALLBACK_LIGHT = "#eee";

/** `preferred` when it reads on `base` (contrast >= 4.5); else `#111` or `#eee`, whichever contrasts more.
 *  When a colour cannot be parsed the preferred (palette) colour is trusted. */
function readableOn(base: string, preferred: string): string {
  const r = contrastRatio(base, preferred);
  if (Number.isNaN(r) || r >= MIN_CONTRAST) return preferred;
  return contrastRatio(base, FALLBACK_DARK) >= contrastRatio(base, FALLBACK_LIGHT) ? FALLBACK_DARK : FALLBACK_LIGHT;
}

export interface ReadableFill {
  /** Colour to paint on #write as its background, or null to leave the theme's. */
  background: string | null;
  /** Colour to use for #write's text, or null to leave the theme's. */
  text: string | null;
}

/**
 * Readability rule. `effectiveBg` / `effectiveFg` are the colours the THEME set (background on #write, body or html;
 * text colour on #write or inherited from body / html); unset = null, empty, `transparent` or zero alpha.
 *   - both set: nothing is filled;
 *   - both unset: the palette pair;
 *   - only the text unset: the palette text, or, if it would contrast under 4.5:1 with the effective background,
 *     `#111` / `#eee` (whichever contrasts more);
 *   - only the background unset: the palette background, or, if it would contrast under 4.5:1 with the theme's
 *     text colour, `#111` / `#eee` likewise.
 * Without a palette nothing is filled. An unparsable colour (hsl(), oklch(), ...) skips the contrast check and the
 * palette colour is used as is.
 */
export function pickReadableColours(
  effectiveBg: string | null | undefined,
  effectiveFg: string | null | undefined,
  palette: Palette | null | undefined,
): ReadableFill {
  if (!palette) return { background: null, text: null };
  const bgUnset = isUnsetColour(effectiveBg), fgUnset = isUnsetColour(effectiveFg);
  if (bgUnset && fgUnset) return { background: palette.background, text: palette.text };
  if (bgUnset) return { background: readableOn(effectiveFg as string, palette.background), text: null };
  if (fgUnset) return { background: null, text: readableOn(effectiveBg as string, palette.text) };
  return { background: null, text: null };
}

// ---------------------------------------------------------------- appearance JSON keys
export interface LiveThemePatch {
  liveThemeFilter?: boolean;
  palette?: Palette;
}

/** Extract `liveThemeFilter` (boolean) and `palette` from a `setAppearance` payload; bad values are ignored. */
export function parseLiveThemePatch(raw: unknown): LiveThemePatch {
  const out: LiveThemePatch = {};
  if (typeof raw !== "object" || raw === null) return out;
  const o = raw as Record<string, unknown>;
  if (typeof o.liveThemeFilter === "boolean") out.liveThemeFilter = o.liveThemeFilter;
  const p = parsePalette(o.palette);
  if (p) out.palette = p;
  return out;
}

// ---------------------------------------------------------------- DOM
/** Add (or move to the end of <head>, so it follows the theme element) / remove the overlay stylesheet. */
export function applyLiveOverlay(enabled: boolean, doc: Document | undefined): void {
  if (!doc) return;
  let el = doc.getElementById(LIVE_OVERLAY_ID);
  if (!enabled) {
    el?.remove();
    return;
  }
  if (!el) {
    el = doc.createElement("style");
    el.id = LIVE_OVERLAY_ID;
    el.textContent = LIVE_OVERLAY_CSS;
  }
  if (doc.head.lastChild !== el) doc.head.appendChild(el);
}

const mappedSource = new WeakMap<object, string>();

/**
 * Keep `<style id="fomawrite-theme-mapped">` in step with the theme element: present (between the theme and the
 * overlay) iff `enabled` and the theme maps to something; its text is `mapThemeCss(theme text)`. The caller passes
 * `true` whatever the filter switch says: the copy is scoped to every Live, so exact mode keeps it.
 */
export function applyThemeMapped(enabled: boolean, doc: Document | undefined): void {
  if (!doc) return;
  const existing = doc.getElementById(MAPPED_ID);
  const theme = doc.getElementById(THEME_ID);
  const source = enabled && theme ? theme.textContent ?? "" : "";
  if (existing && mappedSource.get(existing) === source && source) return;
  const css = source ? mapThemeCss(source) : "";
  if (!css) {
    existing?.remove();
    return;
  }
  const el = existing ?? doc.createElement("style");
  if (!existing) {
    el.id = MAPPED_ID;
    const overlay = doc.getElementById(LIVE_OVERLAY_ID);
    if (overlay) doc.head.insertBefore(el, overlay);
    else doc.head.appendChild(el);
  }
  el.textContent = css;
  mappedSource.set(el, source);
}

export interface LiveThemeState {
  mode: "source" | "live";
  /** `liveThemeFilter`, default true. */
  filter: boolean;
  palette?: Palette;
}

/** Class on <html> while Live is active (filtered or exact): editor.css hangs the host face / line-height fallback on it. */
export const LIVE_ROOT_CLASS = "fw-live-root";

interface ComputedLike { backgroundColor?: string; color?: string; getPropertyValue?: (name: string) => string }

const sameColour = (a: string | null | undefined, b: string | null | undefined): boolean => {
  const x = parseColour(a), y = parseColour(b);
  return !!x && !!y && x.r === y.r && x.g === y.g && x.b === y.b && x.a === y.a;
};

/**
 * What the THEME paints: `bg` is #write's computed background if it has one, else <body>'s, else <html>'s (the editor's
 * own `html, body { background: var(--fw-bg) }` does not count: a body / html background equal to --fw-bg is the
 * editor default, so unset); `fg` is #write's computed colour unless it equals the editor default `--fw-fg`, which also
 * covers a colour inherited from a theme's `body { color }` or `html { color }`. null = the theme set nothing.
 */
export function measureTheme(doc: Document, write: Element): { bg: string | null; fg: string | null } | null {
  const win = doc.defaultView;
  if (!win) return null;
  const css = (el: Element | null | undefined): ComputedLike | null => (el ? (win.getComputedStyle(el) as ComputedLike) : null);
  const root = css(doc.documentElement);
  const editorVar = (name: string) => root?.getPropertyValue?.(name)?.trim() || null;
  const editorBg = editorVar("--fw-bg"), editorFg = editorVar("--fw-fg");

  let bg: string | null = null;
  const own = css(write)?.backgroundColor;
  if (own && !isUnsetColour(own)) bg = own;
  else {
    for (const el of [doc.body, doc.documentElement]) {
      const c = css(el)?.backgroundColor;
      if (c && !isUnsetColour(c) && !(editorBg && sameColour(c, editorBg))) { bg = c; break; }
    }
  }

  const fgNow = css(write)?.color;
  let fg: string | null = null;
  if (fgNow && !isUnsetColour(fgNow)) {
    const base = editorFg ?? root?.color ?? null; // no --fw-fg to read: fall back to <html>'s own colour
    if (!(base && (editorFg ? sameColour(fgNow, base) : fgNow === base))) fg = fgNow;
  }
  return { bg, fg };
}

/**
 * Bring the page in line with the state: overlay element present iff the filter is on; the mapped-theme element
 * present whenever the theme maps to something (its rules are scoped to Live, filtered or exact); `fw-live-filtered`
 * on <html> iff Live and the filter is on; `fw-live-root` on <html> iff Live; `fw-live-exact` on #write iff the filter
 * is off; and, in live mode with the filter on, measure what the theme paints (see measureTheme: background on
 * #write / body / html, text colour on #write or inherited) with the fill classes removed first so the measurement
 * is the theme alone, then fill what is missing from the palette through `--fw-live-bg` / `--fw-live-fg` + the
 * `fw-live-fill-bg|fg` classes the overlay keys on, picking colours that contrast at least 4.5:1 with what the theme
 * did set (pickReadableColours).
 */
export function applyLiveTheme(s: LiveThemeState, doc: Document | undefined): void {
  if (!doc) return;
  applyLiveOverlay(s.filter, doc);
  applyThemeMapped(true, doc);
  doc.documentElement.classList.toggle(LIVE_HTML_CLASS, s.mode === "live" && s.filter);
  doc.documentElement.classList.toggle(LIVE_ROOT_CLASS, s.mode === "live");
  const write = doc.getElementById("write");
  if (!write) return;
  write.classList.toggle(LIVE_EXACT_CLASS, !s.filter);
  write.classList.remove(FILL_BG_CLASS, FILL_FG_CLASS);
  write.style.removeProperty("--fw-live-bg");
  write.style.removeProperty("--fw-live-fg");
  if (s.mode !== "live" || !s.filter || !s.palette) return;
  const m = measureTheme(doc, write);
  if (!m) return;
  const fill = pickReadableColours(m.bg, m.fg, s.palette);
  if (fill.background !== null) {
    write.style.setProperty("--fw-live-bg", fill.background);
    write.classList.add(FILL_BG_CLASS);
  }
  if (fill.text !== null) {
    write.style.setProperty("--fw-live-fg", fill.text);
    write.classList.add(FILL_FG_CLASS);
  }
}
