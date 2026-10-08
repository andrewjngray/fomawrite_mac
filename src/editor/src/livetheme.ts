// Live mode follows the publishing theme, filtered (docs/live-appearance-design.md in the repo root).
//
// The host pushes the theme CSS (rules under `#write`) with `setTheme`; it is applied unchanged, in both modes.
// In Live this module adds an *editing overlay* stylesheet, `<style id="fomawrite-live-overlay">`, placed AFTER
// the theme element so it wins on precedence. It neutralises the theme's page layout and keeps its typography.
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
 *   heading styles, code and blockquote colouring and borders, table borders / cell alignment, link colour,
 *   paragraph and heading margins, background-color of blocks.
 * NEUTRALISED, with !important:
 *   - #write: position -> static; float -> none; columns -> auto; transform, box-shadow, background-image -> none;
 *     width / max-width / margin / padding -> the editor's own measure (760px column, 48px 40px 40vh padding,
 *     50vh top/bottom in typewriter mode, i.e. the values in editor.css); ::before / ::after furniture removed.
 *   - #write > * (theme wrappers, #editor): position static, float none, columns auto, transform / box-shadow /
 *     background-image none, width auto, max-width 100%, no page margin or padding.
 *   - block containers inside (p, ul, ol, blockquote, pre, table, figure, section, ...): float none, columns auto,
 *     transform / box-shadow / background-image none, max-width 100%. `position` is reset on the block-level
 *     furniture (pre, table, figure, img, hr, section, article, aside, header, footer, nav, main, details) but NOT on
 *     li, blockquote, headings or paragraphs, whose ::before/::after are often list markers or quote glyphs
 *     positioned against them.
 * HOST SIZE: once the host has sent `fontSize` (class fw-host-size on #write), `--fw-font-size` beats a theme's
 *   `#write { font-size }`; the theme's heading sizes stay in em, so its scale still follows. Without a host size the
 *   theme's size stands.
 * <body>: `html.fw-live-filtered body` (the class is on <html> only in filtered Live) resets margin, padding,
 *   background-image and columns; colours are left alone.
 * GUARANTEES: the caret follows the text colour; when the theme sets no background and/or text colour on #write,
 *   the host palette fills the missing one through --fw-live-bg / --fw-live-fg (see applyLiveTheme).
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
${S}.${HOST_SIZE_CLASS} { font-size: var(--fw-font-size) !important; }
html.${LIVE_HTML_CLASS} body {
  margin: 0 !important;
  padding: 0 !important;
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

export interface ReadableFill {
  /** Colour to paint on #write as its background, or null to leave the theme's. */
  background: string | null;
  /** Colour to use for #write's text, or null to leave the theme's. */
  text: string | null;
}

/**
 * Readability rule: a theme that sets only one of background / text colour on #write is paired with the host's
 * palette for the other; a theme that sets neither gets the palette pair; a theme that sets both is left alone.
 * `themeBg` / `themeFg` are the theme's computed values on #write (unset = transparent, empty or null).
 * Without a palette nothing is filled.
 */
export function pickReadableColours(
  themeBg: string | null | undefined,
  themeFg: string | null | undefined,
  palette: Palette | null | undefined,
): ReadableFill {
  if (!palette) return { background: null, text: null };
  return {
    background: isUnsetColour(themeBg) ? palette.background : null,
    text: isUnsetColour(themeFg) ? palette.text : null,
  };
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
 * overlay) iff the filter is on and the theme maps to something; its text is `mapThemeCss(theme text)`.
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

/**
 * Bring the page in line with the state: overlay element (and the mapped-theme element) present iff the filter is on;
 * `fw-live-filtered` on <html> iff Live and the filter is on; `fw-live-exact` on #write iff
 * it is off; and, in live mode with the filter on, measure the theme's own background / text colour on #write
 * (fill classes removed first so the measurement is the theme alone) and fill what is missing from the palette
 * through `--fw-live-bg` / `--fw-live-fg` + the `fw-live-fill-bg|fg` classes the overlay keys on.
 * Text colour counts as "set" when #write's computed colour differs from <html>'s (the editor's own default).
 */
export function applyLiveTheme(s: LiveThemeState, doc: Document | undefined): void {
  if (!doc) return;
  applyLiveOverlay(s.filter, doc);
  applyThemeMapped(s.filter, doc);
  doc.documentElement.classList.toggle(LIVE_HTML_CLASS, s.mode === "live" && s.filter);
  const write = doc.getElementById("write");
  if (!write) return;
  write.classList.toggle(LIVE_EXACT_CLASS, !s.filter);
  write.classList.remove(FILL_BG_CLASS, FILL_FG_CLASS);
  write.style.removeProperty("--fw-live-bg");
  write.style.removeProperty("--fw-live-fg");
  const win = doc.defaultView;
  if (s.mode !== "live" || !s.filter || !s.palette || !win) return;
  const cs = win.getComputedStyle(write);
  const base = win.getComputedStyle(doc.documentElement).color;
  const themeFg = cs.color && cs.color !== base ? cs.color : null;
  const fill = pickReadableColours(cs.backgroundColor, themeFg, s.palette);
  if (fill.background !== null) {
    write.style.setProperty("--fw-live-bg", fill.background);
    write.classList.add(FILL_BG_CLASS);
  }
  if (fill.text !== null) {
    write.style.setProperty("--fw-live-fg", fill.text);
    write.classList.add(FILL_FG_CLASS);
  }
}
