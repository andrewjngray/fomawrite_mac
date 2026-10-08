import test from "node:test";
import assert from "node:assert/strict";
import {
  FILL_BG_CLASS, FILL_FG_CLASS, LIVE_EXACT_CLASS, LIVE_OVERLAY_CSS, LIVE_OVERLAY_ID,
  HOST_SIZE_CLASS, LIVE_HTML_CLASS, LIVE_ROOT_CLASS, MAPPED_ID, THEME_ID, applyLiveOverlay, applyLiveTheme, applyThemeMapped,
  contrastRatio, isUnsetColour, measureTheme, parseColour, parseLiveThemePatch, parsePalette, pickReadableColours, relativeLuminance,
} from "../src/livetheme.ts";
import { INITIAL_APPEARANCE, effectiveAppearanceClasses, parseAppearancePatch, reduceAppearance } from "../src/appearance.ts";
import { Session, applyAppearanceDom, applyThemeDom } from "../src/modes.ts";

// ---------------------------------------------------------------- tiny fake Document
function fakeDoc({ computed = {}, body = {}, html = {}, vars = {} } = {}) {
  const head = {
    children: [],
    get lastChild() { return this.children[this.children.length - 1] ?? null; },
    appendChild(el) { this.children = this.children.filter((c) => c !== el); this.children.push(el); el.parent = this; return el; },
    insertBefore(el, ref) {
      this.children = this.children.filter((c) => c !== el);
      this.children.splice(this.children.indexOf(ref), 0, el);
      el.parent = this;
      return el;
    },
  };
  const el = (tag) => {
    const classes = new Set(), props = new Map();
    return {
      tag, id: "", textContent: "", parent: null,
      classList: {
        toggle(c, on) { (on ?? !classes.has(c)) ? classes.add(c) : classes.delete(c); },
        add(...cs) { cs.forEach((c) => classes.add(c)); },
        remove(...cs) { cs.forEach((c) => classes.delete(c)); },
        contains: (c) => classes.has(c),
        has: (c) => classes.has(c),
      },
      style: { setProperty: (k, v) => props.set(k, v), removeProperty: (k) => props.delete(k), get: (k) => props.get(k) },
      remove() { if (this.parent) this.parent.children = this.parent.children.filter((c) => c !== this); this.parent = null; },
    };
  };
  const write = el("div"); write.id = "write";
  const htmlEl = el("html");
  const bodyEl = el("body");
  // the editor's own defaults: --fw-bg / --fw-fg on :root, painted on html and body, inherited by #write
  const rootVars = { "--fw-bg": "#ffffff", "--fw-fg": "#24292f", ...vars };
  const doc = {
    head, documentElement: htmlEl, body: bodyEl, write,
    createElement: el,
    getElementById: (id) => (id === "write" ? write : head.children.find((c) => c.id === id) ?? null),
    defaultView: {
      getComputedStyle: (e) => {
        if (e === write) return { backgroundColor: "rgba(0, 0, 0, 0)", color: "rgb(36, 41, 47)", ...computed };
        if (e === bodyEl) return { backgroundColor: "rgb(255, 255, 255)", color: "rgb(36, 41, 47)", ...body };
        return { backgroundColor: "rgb(255, 255, 255)", color: "rgb(36, 41, 47)", getPropertyValue: (n) => rootVars[n] ?? "", ...html };
      },
    },
  };
  return doc;
}
const has = (doc, id) => doc.head.children.some((c) => c.id === id);
const PALETTE = { background: "#101010", text: "#eeeeee" };

// ---------------------------------------------------------------- overlay CSS string
test("overlay css neutralises layout and is scoped to live + not exact", () => {
  for (const rule of [
    "float: none", "background-image: none", "box-shadow: none", "transform: none", "position: static",
    "max-width", "columns: auto", "column-count: auto", "::before", "::after", "margin", "padding",
  ]) assert.ok(LIVE_OVERLAY_CSS.includes(rule), rule);
  // every selector is scoped to live mode and respects the exact switch
  const selectors = LIVE_OVERLAY_CSS.replace(/\/\*[\s\S]*?\*\//g, "").split("{").slice(0, -1)
    .map((chunk) => chunk.split("}").pop().trim()).filter(Boolean);
  assert.ok(selectors.length > 5);
  for (const sel of selectors) for (const part of sel.replace(/:is\([^)]*\)/g, ":is()").split(",")) {
    // the one exception is the <body> reset, which keys on the class set on <html> only in filtered Live
    assert.ok(part.trim().startsWith("#write.fw-mode-live:not(.fw-live-exact)") || part.trim() === "html.fw-live-filtered body", part);
  }
});

test("overlay css never touches typography except through the readability variables", () => {
  const css = LIVE_OVERLAY_CSS.replace(/\/\*[\s\S]*?\*\//g, "");
  // the only font-size is the host's base size, and only once the host has sent one
  assert.ok(/#write\.fw-mode-live:not\(\.fw-live-exact\)\.fw-host-size\s*\{\s*font-size:\s*var\(--fw-font-size\)\s*!important;\s*\}/.test(css));
  assert.equal(css.match(/(?<![-\w])font-size\s*:/g).length, 1);
  for (const prop of ["font-family", "font-weight", "font-style", "line-height", "letter-spacing", "text-decoration", "list-style"])
    assert.ok(!css.includes(prop), prop);
  const decls = [...css.matchAll(/(?:^|[;{\s])(color|background-color|background)\s*:\s*([^;}]*)/g)];
  for (const [, prop, value] of decls) {
    assert.ok(/var\(--fw-live-(bg|fg)\)/.test(value), `${prop}: ${value}`);
  }
  assert.ok(css.includes("var(--fw-live-bg)") && css.includes("var(--fw-live-fg)"));
  // background-image is only ever reset, never set to a value
  for (const m of css.matchAll(/background-image:\s*([^;]+);/g)) assert.equal(m[1].replace("!important", "").trim(), "none");
  // list markers are not positioned away
  assert.ok(!/\bli\b[^{]*\{[^}]*position/.test(css));
});

// ---------------------------------------------------------------- effective classes
test("effectiveAppearanceClasses: no fw-appearance-* in live, usual ones in source", () => {
  const s = reduceAppearance(INITIAL_APPEARANCE, parseAppearancePatch({ appearance: "book", focus: true, typewriter: true }));
  assert.deepEqual(effectiveAppearanceClasses("source", s), ["fw-appearance-book", "fw-focus", "fw-typewriter"]);
  assert.deepEqual(effectiveAppearanceClasses("live", s), ["fw-focus", "fw-typewriter"]);
  for (const name of ["manuscript", "editorial", "book", "code"]) {
    const t = reduceAppearance(INITIAL_APPEARANCE, parseAppearancePatch({ appearance: name }));
    assert.deepEqual(effectiveAppearanceClasses("source", t), [`fw-appearance-${name}`]);
    assert.deepEqual(effectiveAppearanceClasses("live", t), []);
  }
  assert.deepEqual(effectiveAppearanceClasses("live", INITIAL_APPEARANCE), []);
  // the state itself keeps the appearance, so switching back re-applies it
  assert.equal(s.appearance, "book");
});

// ---------------------------------------------------------------- pickReadableColours
test("pickReadableColours: both set, bg missing, fg missing, both missing", () => {
  assert.deepEqual(pickReadableColours("rgb(255, 255, 255)", "rgb(0, 0, 0)", PALETTE), { background: null, text: null });
  // bg missing, theme text black: the palette's near-black background would not read, so #eee
  assert.deepEqual(pickReadableColours("rgba(0, 0, 0, 0)", "rgb(0, 0, 0)", PALETTE), { background: "#eee", text: null });
  // text missing on a white background: the palette's light text would not read, so #111
  assert.deepEqual(pickReadableColours("rgb(255, 255, 255)", null, PALETTE), { background: null, text: "#111" });
  assert.deepEqual(pickReadableColours("transparent", "", PALETTE), { background: "#101010", text: "#eeeeee" });
  assert.deepEqual(pickReadableColours("rgba(0, 0, 0, 0)", null, undefined), { background: null, text: null });
});

// ---------------------------------------------------------------- colour maths and the contrast rule
const LIGHT = { background: "#ffffff", text: "#24292f" };
const DARK = { background: "#1e1f22", text: "#e6edf3" };
const CREAM = "rgb(250, 249, 245)"; // claude-like body background
const BROWN = "rgb(43, 38, 33)"; // claude-like text

test("parseColour, relativeLuminance, contrastRatio", () => {
  assert.deepEqual(parseColour("#fff"), { r: 255, g: 255, b: 255, a: 1 });
  assert.deepEqual(parseColour("#1e1f22"), { r: 30, g: 31, b: 34, a: 1 });
  assert.deepEqual(parseColour("rgb(1 2 3 / 50%)"), { r: 1, g: 2, b: 3, a: 0.5 });
  assert.deepEqual(parseColour(" RGBA(1, 2, 3, 0.25) "), { r: 1, g: 2, b: 3, a: 0.25 });
  assert.deepEqual(parseColour("rgb(100%, 0%, 0%)"), { r: 255, g: 0, b: 0, a: 1 });
  assert.equal(parseColour("white").r, 255);
  for (const bad of ["oklch(0.5 0.1 20)", "var(--x)", "#12", "rgb(1,2)", "", null, undefined, "rgb(a,b,c)"]) assert.equal(parseColour(bad), null, String(bad));
  assert.equal(relativeLuminance("#000"), 0);
  assert.equal(relativeLuminance("#fff"), 1);
  assert.ok(Math.abs(relativeLuminance("#808080") - 0.2158) < 0.001);
  assert.ok(Number.isNaN(relativeLuminance("oklch(0.5 0.1 20)")));
  assert.equal(contrastRatio("#000", "#fff"), 21);
  assert.equal(contrastRatio("#fff", "#000"), 21, "symmetric");
  assert.equal(contrastRatio("#777", "#777"), 1);
  assert.ok(Math.abs(contrastRatio("#777777", "#ffffff") - 4.48) < 0.01);
  assert.ok(Number.isNaN(contrastRatio("#fff", "nonsense")));
});

test("pickReadableColours: body-painted theme (claude-like) with unset text, light and dark palette", () => {
  // light mode: the palette's dark text reads on cream
  assert.deepEqual(pickReadableColours(CREAM, null, LIGHT), { background: null, text: LIGHT.text });
  // dark mode: the palette's light text would be unreadable on cream, so plain #111; never the palette background
  assert.deepEqual(pickReadableColours(CREAM, null, DARK), { background: null, text: "#111" });
  assert.deepEqual(pickReadableColours(CREAM, "", DARK), { background: null, text: "#111" });
});

test("pickReadableColours: light background + unset text in dark mode, dark background + unset text in light mode", () => {
  assert.deepEqual(pickReadableColours("rgb(255, 255, 255)", null, DARK), { background: null, text: "#111" });
  assert.deepEqual(pickReadableColours("#fdf6e3", null, DARK), { background: null, text: "#111" });
  assert.deepEqual(pickReadableColours("rgb(28, 24, 21)", null, LIGHT), { background: null, text: "#eee" });
  // a dark background that suits the dark palette keeps the palette text
  assert.deepEqual(pickReadableColours("rgb(28, 24, 21)", null, DARK), { background: null, text: DARK.text });
});

test("pickReadableColours: text set, background unset, and both unset", () => {
  assert.deepEqual(pickReadableColours(null, BROWN, LIGHT), { background: LIGHT.background, text: null });
  assert.deepEqual(pickReadableColours(null, BROWN, DARK), { background: "#eee", text: null }, "dark palette bg would hide dark text");
  assert.deepEqual(pickReadableColours("rgba(0, 0, 0, 0)", "rgb(240, 240, 240)", LIGHT), { background: "#111", text: null });
  assert.deepEqual(pickReadableColours(null, null, LIGHT), { background: LIGHT.background, text: LIGHT.text });
  assert.deepEqual(pickReadableColours("transparent", "", DARK), { background: DARK.background, text: DARK.text });
  assert.deepEqual(pickReadableColours(null, null, null), { background: null, text: null });
});

test("pickReadableColours: whatever is filled reads at 4.5:1 against what the theme set", () => {
  const grounds = ["#ffffff", "#faf9f5", "#fdf6e3", "#eeeeee", "#1e1f22", "#000000", "#1c1815", "#222222", "#0b3d91", "#a52a2a", "#ffeb3b"];
  for (const p of [LIGHT, DARK, { background: "#777777", text: "#888888" }]) {
    for (const g of grounds) {
      const fill = pickReadableColours(g, null, p);
      assert.ok(contrastRatio(g, fill.text) >= 4.5, `text ${fill.text} on ${g}`);
      const fill2 = pickReadableColours(null, g, p);
      assert.ok(contrastRatio(g, fill2.background) >= 4.5, `bg ${fill2.background} behind ${g}`);
    }
  }
  // an unparsable colour (oklch) cannot be judged: the palette colour is used as is
  assert.deepEqual(pickReadableColours("oklch(0.95 0.02 90)", null, DARK), { background: null, text: DARK.text });
});

test("isUnsetColour and palette parsing", () => {
  for (const c of ["", "transparent", "rgba(0, 0, 0, 0)", "rgb(0 0 0 / 0)", " TRANSPARENT ", null, undefined]) assert.ok(isUnsetColour(c), String(c));
  for (const c of ["rgb(0, 0, 0)", "rgba(0, 0, 0, 0.5)", "#fff", "white"]) assert.ok(!isUnsetColour(c), c);
  assert.deepEqual(parsePalette({ background: " #fff ", text: "#000" }), { background: "#fff", text: "#000" });
  for (const bad of [null, "x", {}, { background: "#fff" }, { background: "", text: "#000" }, { background: 1, text: 2 }])
    assert.equal(parsePalette(bad), undefined);
  assert.deepEqual(parseLiveThemePatch({ liveThemeFilter: false, palette: { background: "#fff", text: "#000" }, x: 1 }),
    { liveThemeFilter: false, palette: { background: "#fff", text: "#000" } });
  assert.deepEqual(parseLiveThemePatch({ liveThemeFilter: "no", palette: 3 }), {});
});

// ---------------------------------------------------------------- DOM: overlay on/off, ordering
test("liveThemeFilter:false removes the overlay and marks #write exact; true restores it", () => {
  const doc = fakeDoc();
  applyLiveTheme({ mode: "live", filter: true }, doc);
  assert.ok(has(doc, LIVE_OVERLAY_ID));
  assert.equal(doc.head.children[0].textContent, LIVE_OVERLAY_CSS);
  assert.ok(!doc.write.classList.has(LIVE_EXACT_CLASS));
  applyLiveTheme({ mode: "live", filter: false }, doc);
  assert.ok(!has(doc, LIVE_OVERLAY_ID));
  assert.ok(doc.write.classList.has(LIVE_EXACT_CLASS));
  applyLiveTheme({ mode: "live", filter: true }, doc);
  assert.ok(has(doc, LIVE_OVERLAY_ID));
  assert.ok(!doc.write.classList.has(LIVE_EXACT_CLASS));
  applyLiveOverlay(true, doc); // idempotent: still one element
  assert.equal(doc.head.children.filter((c) => c.id === LIVE_OVERLAY_ID).length, 1);
});

test("the overlay stays after the theme element however they are added", () => {
  const doc = fakeDoc();
  applyThemeDom("#write { color: red }", doc); // theme first
  applyLiveOverlay(true, doc);
  assert.deepEqual(doc.head.children.map((c) => c.id), ["fomawrite-theme", LIVE_OVERLAY_ID]);

  const doc2 = fakeDoc();
  applyLiveOverlay(true, doc2); // overlay first, theme arrives later
  applyThemeDom("#write { color: red }", doc2);
  assert.deepEqual(doc2.head.children.map((c) => c.id), ["fomawrite-theme", LIVE_OVERLAY_ID]);
  applyThemeDom("#write { color: blue }", doc2); // update in place keeps the order
  assert.deepEqual(doc2.head.children.map((c) => c.id), ["fomawrite-theme", LIVE_OVERLAY_ID]);
  applyThemeDom("", doc2);
  assert.deepEqual(doc2.head.children.map((c) => c.id), [LIVE_OVERLAY_ID]);
});

// ---------------------------------------------------------------- DOM: readability fill
test("palette fills a missing background and colour in live mode only", () => {
  // theme sets neither (computed colour equals <html>'s, background transparent)
  const doc = fakeDoc();
  applyLiveTheme({ mode: "live", filter: true, palette: PALETTE }, doc);
  assert.ok(doc.write.classList.has(FILL_BG_CLASS) && doc.write.classList.has(FILL_FG_CLASS));
  assert.equal(doc.write.style.get("--fw-live-bg"), "#101010");
  assert.equal(doc.write.style.get("--fw-live-fg"), "#eeeeee");
  // source mode: nothing filled, and leftovers are cleared
  applyLiveTheme({ mode: "source", filter: true, palette: PALETTE }, doc);
  assert.ok(!doc.write.classList.has(FILL_BG_CLASS) && !doc.write.classList.has(FILL_FG_CLASS));
  assert.equal(doc.write.style.get("--fw-live-bg"), undefined);
  // exact: nothing filled
  applyLiveTheme({ mode: "live", filter: false, palette: PALETTE }, doc);
  assert.ok(!doc.write.classList.has(FILL_BG_CLASS));
  // no palette: nothing filled
  applyLiveTheme({ mode: "live", filter: true }, doc);
  assert.ok(!doc.write.classList.has(FILL_BG_CLASS) && !doc.write.classList.has(FILL_FG_CLASS));
});

test("a theme that sets both colours is left alone; one missing is filled", () => {
  const both = fakeDoc({ computed: { backgroundColor: "rgb(20, 20, 20)", color: "rgb(240, 240, 240)" } });
  applyLiveTheme({ mode: "live", filter: true, palette: PALETTE }, both);
  assert.ok(!both.write.classList.has(FILL_BG_CLASS) && !both.write.classList.has(FILL_FG_CLASS));

  const bgOnly = fakeDoc({ computed: { backgroundColor: "rgb(20, 20, 20)" } }); // colour = default
  applyLiveTheme({ mode: "live", filter: true, palette: PALETTE }, bgOnly);
  assert.ok(!bgOnly.write.classList.has(FILL_BG_CLASS) && bgOnly.write.classList.has(FILL_FG_CLASS));

  const fgOnly = fakeDoc({ computed: { color: "rgb(240, 240, 240)" } }); // background transparent
  applyLiveTheme({ mode: "live", filter: true, palette: PALETTE }, fgOnly);
  assert.ok(fgOnly.write.classList.has(FILL_BG_CLASS) && !fgOnly.write.classList.has(FILL_FG_CLASS));
});

// ---------------------------------------------------------------- Session wiring
function withDocument(doc, fn) {
  globalThis.document = doc;
  try { return fn(); } finally { delete globalThis.document; }
}

function fakeView(session) {
  const view = { state: session.createState(""), dispatch(...specs) { this.state = this.state.update(...specs).state; }, setState(s) { this.state = s; } };
  session.attach(view);
  return view;
}

test("Session.setAppearance reads liveThemeFilter and palette; mode switches re-evaluate the fill", () => {
  const doc = fakeDoc();
  withDocument(doc, () => {
    const session = new Session();
    fakeView(session);
    session.syncLiveTheme();
    assert.ok(has(doc, LIVE_OVERLAY_ID), "default: overlay on");
    session.setAppearance({ palette: PALETTE, appearance: "book" });
    assert.ok(!doc.write.classList.has(FILL_BG_CLASS), "source mode: no fill");
    session.setMode("live");
    assert.ok(doc.write.classList.has(FILL_BG_CLASS), "live: palette fills the missing background");
    session.setAppearance({ liveThemeFilter: false });
    assert.ok(!has(doc, LIVE_OVERLAY_ID));
    assert.ok(doc.write.classList.has(LIVE_EXACT_CLASS));
    assert.ok(!doc.write.classList.has(FILL_BG_CLASS));
    assert.deepEqual(session.appearance, {}, "new keys are not treated as legacy font keys");
    session.setAppearance({ liveThemeFilter: true });
    assert.ok(has(doc, LIVE_OVERLAY_ID));
    session.setMode("source");
    assert.ok(!doc.write.classList.has(FILL_BG_CLASS));
  });
});

// ---------------------------------------------------------------- selector mapping element (thememap.ts)
const ids = (doc) => doc.head.children.map((c) => c.id);
const THEME = "#write h1 { color: crimson; font-family: Georgia } #write blockquote { color: gray } #write code { color: purple } #write { font-size: 30px }";

test("mapped theme element sits between the theme and the overlay, whatever the arrival order", () => {
  const a = fakeDoc();
  applyThemeDom(THEME, a);
  applyLiveTheme({ mode: "live", filter: true }, a);
  assert.deepEqual(ids(a), [THEME_ID, MAPPED_ID, LIVE_OVERLAY_ID]);
  assert.ok(a.getElementById(MAPPED_ID).textContent.includes(".cm-line.fw-h1 { color: crimson; font-family: Georgia; }"));
  assert.ok(a.getElementById(MAPPED_ID).textContent.includes(".fw-code { color: purple; }"));

  const b = fakeDoc(); // overlay exists first, the theme arrives later
  applyLiveTheme({ mode: "live", filter: true }, b);
  assert.deepEqual(ids(b), [LIVE_OVERLAY_ID], "no theme, nothing to map");
  applyThemeDom(THEME, b);
  applyLiveTheme({ mode: "live", filter: true }, b);
  assert.deepEqual(ids(b), [THEME_ID, MAPPED_ID, LIVE_OVERLAY_ID]);

  const c = fakeDoc(); // the theme element is removed and re-created while a mapped element exists
  applyThemeDom(THEME, c);
  applyLiveTheme({ mode: "live", filter: true }, c);
  applyThemeDom("", c);
  applyThemeDom("#write h2 { color: red }", c);
  assert.deepEqual(ids(c), [THEME_ID, MAPPED_ID, LIVE_OVERLAY_ID]);
});

test("mapped theme element follows the theme text and the filter switch", () => {
  const doc = fakeDoc();
  applyThemeDom(THEME, doc);
  applyLiveTheme({ mode: "live", filter: true }, doc);
  applyThemeDom("#write h2 { color: red }", doc);
  applyLiveTheme({ mode: "live", filter: true }, doc);
  assert.equal(ids(doc).filter((i) => i === MAPPED_ID).length, 1);
  assert.ok(doc.getElementById(MAPPED_ID).textContent.includes("fw-h2 { color: red; }"));
  assert.ok(!doc.getElementById(MAPPED_ID).textContent.includes("fw-h1"));
  // a theme with nothing mappable has no mapped element
  applyThemeDom("#write table { color: red }", doc);
  applyLiveTheme({ mode: "live", filter: true }, doc);
  assert.ok(!has(doc, MAPPED_ID));
  // exact mode removes the overlay but KEEPS the mapped copy (typography of Live lines); the theme element is untouched
  applyThemeDom(THEME, doc);
  applyLiveTheme({ mode: "live", filter: true }, doc);
  assert.ok(has(doc, MAPPED_ID));
  const mappedText = doc.getElementById(MAPPED_ID).textContent;
  applyLiveTheme({ mode: "live", filter: false }, doc);
  assert.deepEqual(ids(doc), [THEME_ID, MAPPED_ID]);
  assert.equal(doc.getElementById(MAPPED_ID).textContent, mappedText, "same mapped text in exact mode");
  assert.ok(!mappedText.includes("fw-live-exact"), "mapped rules are not scoped away from exact mode");
  assert.equal(doc.getElementById(THEME_ID).textContent, THEME);
  applyLiveTheme({ mode: "live", filter: true }, doc);
  assert.deepEqual(ids(doc), [THEME_ID, MAPPED_ID, LIVE_OVERLAY_ID]);
  applyThemeDom("", doc); // theme removed
  applyThemeMapped(true, doc);
  assert.deepEqual(ids(doc), [LIVE_OVERLAY_ID]);
});

test("Session.setMode / setAppearance keep the mapped element and the html class in step", () => {
  const doc = fakeDoc();
  withDocument(doc, () => {
    const session = new Session();
    fakeView(session);
    applyThemeDom(THEME, doc);
    session.syncLiveTheme();
    assert.ok(has(doc, MAPPED_ID), "present in source too (its rules are scoped to live)");
    assert.ok(!doc.documentElement.classList.has(LIVE_HTML_CLASS));
    session.setMode("live");
    assert.ok(doc.documentElement.classList.has(LIVE_HTML_CLASS));
    session.setAppearance({ liveThemeFilter: false });
    assert.ok(has(doc, MAPPED_ID), "exact mode keeps the mapped copy");
    assert.ok(!has(doc, LIVE_OVERLAY_ID) && !doc.documentElement.classList.has(LIVE_HTML_CLASS));
    session.setAppearance({ liveThemeFilter: true });
    assert.ok(has(doc, MAPPED_ID) && doc.documentElement.classList.has(LIVE_HTML_CLASS));
    session.setMode("source");
    assert.ok(!doc.documentElement.classList.has(LIVE_HTML_CLASS));
  });
});

// ---------------------------------------------------------------- host text size beats the theme's #write size
test("host fontSize marks #write; the overlay lets --fw-font-size beat the theme's #write font-size", () => {
  assert.ok(LIVE_OVERLAY_CSS.includes("#write.fw-mode-live:not(.fw-live-exact).fw-host-size { font-size: var(--fw-font-size) !important; }"));
  const doc = fakeDoc();
  applyAppearanceDom({}, doc);
  assert.ok(!doc.write.classList.has(HOST_SIZE_CLASS), "no host size yet: the theme's size stands");
  applyAppearanceDom({ fontFamily: "Georgia" }, doc);
  assert.ok(!doc.write.classList.has(HOST_SIZE_CLASS));
  applyAppearanceDom({ fontSize: 17 }, doc);
  assert.equal(doc.write.style.get("--fw-font-size"), "17px");
  assert.ok(doc.write.classList.has(HOST_SIZE_CLASS));
  applyAppearanceDom({ fontSize: 19 }, doc); // Larger / Smaller / Reset keep sending numbers
  assert.equal(doc.write.style.get("--fw-font-size"), "19px");
  applyAppearanceDom({ dark: true }, doc); // other keys leave it alone
  assert.ok(doc.write.classList.has(HOST_SIZE_CLASS));
});

test("mapped headings stay relative so the theme's scale follows the host size", () => {
  const doc = fakeDoc();
  applyThemeDom("#write { font-size: 30px } #write h1 { font-size: 2.2rem } #write h2 { font-size: 1.5em }", doc);
  applyLiveTheme({ mode: "live", filter: true }, doc);
  const mapped = doc.getElementById(MAPPED_ID).textContent;
  assert.ok(mapped.includes("font-size: 1.1733em"), "2.2rem = 35.2px over the theme's 30px #write");
  assert.ok(mapped.includes("font-size: 1.5em"));
  assert.ok(!mapped.includes("30px") && !mapped.includes("rem"));
});

// ---------------------------------------------------------------- html / body rules
test("fw-live-filtered is on <html> only in live mode with the filter on", () => {
  const doc = fakeDoc();
  const on = () => doc.documentElement.classList.has(LIVE_HTML_CLASS);
  applyLiveTheme({ mode: "source", filter: true }, doc);
  assert.ok(!on());
  applyLiveTheme({ mode: "live", filter: true }, doc);
  assert.ok(on());
  applyLiveTheme({ mode: "live", filter: false }, doc);
  assert.ok(!on(), "exact mode");
  applyLiveTheme({ mode: "live", filter: true, palette: PALETTE }, doc);
  assert.ok(on());
  applyLiveTheme({ mode: "source", filter: true, palette: PALETTE }, doc);
  assert.ok(!on());
  applyLiveTheme({ mode: "source", filter: false }, doc);
  assert.ok(!on());
  assert.equal(LIVE_HTML_CLASS, "fw-live-filtered");
});

test("body rule resets geometry only: no colour, and it keys on the html class", () => {
  const m = /html\.fw-live-filtered body \{([^}]*)\}/.exec(LIVE_OVERLAY_CSS);
  assert.ok(m, "rule present");
  const decls = m[1].split(";").map((d) => d.trim()).filter(Boolean).sort();
  assert.deepEqual(decls, [
    "background-image: none !important", "column-count: auto !important", "columns: auto !important",
    "display: block !important", "margin: 0 !important", "max-width: none !important", "padding: 0 !important",
    "position: static !important", "transform: none !important", "width: auto !important",
  ]);
  assert.ok(!/background-color|background:|\bcolor\b/.test(m[1]));
});

// ---------------------------------------------------------------- readability measured on the DOM (body-painted themes)
const fillOf = (doc) => ({
  bg: doc.write.classList.has(FILL_BG_CLASS) ? doc.write.style.get("--fw-live-bg") : null,
  fg: doc.write.classList.has(FILL_FG_CLASS) ? doc.write.style.get("--fw-live-fg") : null,
});

test("claude-like (body painted, #write transparent, text colour inherited from body): nothing is filled, light or dark", () => {
  // light: body bg cream, #write transparent, text brown from `body { color }`
  const light = fakeDoc({ body: { backgroundColor: CREAM }, html: { backgroundColor: CREAM }, computed: { color: BROWN } });
  applyLiveTheme({ mode: "live", filter: true, palette: LIGHT }, light);
  assert.deepEqual(fillOf(light), { bg: null, fg: null }, "no white column on cream");
  // dark mode: the editor defaults are dark, the theme still paints cream and brown: left alone (no dark column with brown text)
  const dark = fakeDoc({
    vars: { "--fw-bg": "#1e1f22", "--fw-fg": "#e6edf3" },
    body: { backgroundColor: CREAM }, html: { backgroundColor: CREAM }, computed: { color: BROWN },
  });
  applyLiveTheme({ mode: "live", filter: true, palette: DARK }, dark);
  assert.deepEqual(fillOf(dark), { bg: null, fg: null });
});

test("body-painted theme that leaves the text colour alone: light bg + dark mode gets #111 text, never a palette background", () => {
  const dark = fakeDoc({
    vars: { "--fw-bg": "#1e1f22", "--fw-fg": "#e6edf3" },
    body: { backgroundColor: CREAM }, html: { backgroundColor: CREAM },
    computed: { color: "rgb(230, 237, 243)" }, // inherited editor default (dark mode: light)
  });
  applyLiveTheme({ mode: "live", filter: true, palette: DARK }, dark);
  assert.deepEqual(fillOf(dark), { bg: null, fg: "#111" });
  // same page in light mode: the palette's own dark text reads on cream
  const light = fakeDoc({ body: { backgroundColor: CREAM }, html: { backgroundColor: CREAM } });
  applyLiveTheme({ mode: "live", filter: true, palette: LIGHT }, light);
  assert.deepEqual(fillOf(light), { bg: null, fg: LIGHT.text });
});

test("a background on html alone counts; the editor's own body / html background does not", () => {
  const htmlOnly = fakeDoc({ html: { backgroundColor: CREAM }, body: { backgroundColor: "rgba(0, 0, 0, 0)" } });
  applyLiveTheme({ mode: "live", filter: true, palette: LIGHT }, htmlOnly);
  assert.equal(fillOf(htmlOnly).bg, null, "html paints it");
  assert.equal(fillOf(htmlOnly).fg, LIGHT.text);
  // nothing painted by the theme: body and html are just the editor's --fw-bg
  const none = fakeDoc();
  applyLiveTheme({ mode: "live", filter: true, palette: LIGHT }, none);
  assert.deepEqual(fillOf(none), { bg: LIGHT.background, fg: LIGHT.text });
  const noneDark = fakeDoc({ vars: { "--fw-bg": "#1e1f22", "--fw-fg": "#e6edf3" }, body: { backgroundColor: "rgb(30, 31, 34)" }, html: { backgroundColor: "rgb(30, 31, 34)" }, computed: { color: "rgb(230, 237, 243)" } });
  applyLiveTheme({ mode: "live", filter: true, palette: DARK }, noneDark);
  assert.deepEqual(fillOf(noneDark), { bg: DARK.background, fg: DARK.text });
  // theme text colour set (differs from --fw-fg) but no background: palette background chosen for contrast with that text
  const fgOnly = fakeDoc({ vars: { "--fw-bg": "#1e1f22", "--fw-fg": "#e6edf3" }, body: { backgroundColor: "rgb(30, 31, 34)" }, html: { backgroundColor: "rgb(30, 31, 34)" }, computed: { color: BROWN } });
  applyLiveTheme({ mode: "live", filter: true, palette: DARK }, fgOnly);
  assert.deepEqual(fillOf(fgOnly), { bg: "#eee", fg: null });
});

// ---------------------------------------------------------------- overlay: tables at full measure, more neutralising
const overlayRule = (selector) => {
  const css = LIVE_OVERLAY_CSS.replace(/\/\*[\s\S]*?\*\//g, "");
  const i = css.indexOf(selector + " {");
  assert.ok(i >= 0, selector);
  return css.slice(css.indexOf("{", i) + 1, css.indexOf("}", i)).split(";").map((d) => d.trim()).filter(Boolean);
};
const SCOPE = "#write.fw-mode-live:not(.fw-live-exact)";

test("the rendered table is forced to the full measure in filtered Live", () => {
  assert.ok(overlayRule(`${SCOPE} .fw-table`).includes("width: 100% !important"));
});

test("#write is neutralised further: display, box model, border, outline, filter, opacity, zoom, overflow, height", () => {
  const rule = overlayRule(SCOPE);
  for (const d of [
    "display: block", "box-sizing: border-box", "border: none", "border-radius: 0", "min-height: auto", "height: auto",
    "outline: none", "filter: none", "opacity: 1", "zoom: 1", "overflow: visible",
  ]) assert.ok(rule.includes(d + " !important"), d);
});

test("the <body> reset also clears width, max-width, display, position and transform", () => {
  const rule = overlayRule("html.fw-live-filtered body");
  for (const d of ["width: auto", "max-width: none", "display: block", "position: static", "transform: none"])
    assert.ok(rule.includes(d + " !important"), d);
});

// ---------------------------------------------------------------- host font / line height are fallbacks (real cascade)
import { readFileSync } from "node:fs";
import { computed as cascade, el as celem, higher, parseRules, specificity } from "./cascade.mjs";
import { mapThemeCss } from "../src/thememap.ts";

const EDITOR_CSS = readFileSync(new URL("../editor.css", import.meta.url), "utf8");
const BLOCKS_CSS = readFileSync(new URL("../blocks.css", import.meta.url), "utf8");

// A tiny page: html > body > #write > #editor > .cm-content (a Live or Source content area).
function page({ live, theme = "", host = {}, exact = false }) {
  const vars = {};
  if (host.font) vars["--fw-font"] = host.font; // applyAppearanceDom mirrors these onto <html> and #write
  if (host.lineHeight) vars["--fw-line-height"] = String(host.lineHeight);
  const html = celem("html", { classes: live ? [LIVE_ROOT_CLASS] : [], vars });
  const body = celem("body", { parent: html });
  const write = celem("div", { id: "write", parent: body, classes: live ? ["fw-mode-live"].concat(exact ? ["fw-live-exact"] : []) : ["fw-mode-source"], vars });
  const content = celem("div", { parent: write, classes: ["cm-content"] });
  const rules = [...parseRules(EDITOR_CSS), ...parseRules(BLOCKS_CSS, 5000), ...parseRules(theme, 10000)];
  return { html, body, write, content, rules };
}
const face = (p) => cascade("font-family", p.content, p.rules);
const lh = (p) => cascade("line-height", p.write, p.rules);

test("filtered Live: a face the theme sets on body wins over the host fontFamily", () => {
  const p = page({ live: true, theme: "body { font-family: Palatino, serif; line-height: 1.62 }", host: { font: "Menlo", lineHeight: 2 } });
  assert.equal(face(p), "Palatino, serif");
  assert.equal(lh(p), "1.62");
});

test("filtered Live: the theme's face on html or #write also wins (and #write beats body)", () => {
  assert.equal(face(page({ live: true, theme: "html { font-family: Optima }", host: { font: "Menlo" } })), "Optima");
  assert.equal(face(page({ live: true, theme: "#write { font-family: Garamond } body { font-family: Palatino }", host: { font: "Menlo" } })), "Garamond");
  assert.equal(lh(page({ live: true, theme: "#write { line-height: 1.9 }", host: { lineHeight: 1.4 } })), "1.9");
  assert.equal(lh(page({ live: true, theme: "html { line-height: 1.7 }", host: { lineHeight: 1.4 } })), "1.7");
});

test("filtered Live: a theme that sets no face falls back to the host fontFamily, then Georgia", () => {
  const themeNoFont = "body { color: #222; background: #fafafa } #write { padding: 1px }";
  assert.equal(face(page({ live: true, theme: themeNoFont, host: { font: '"Iowan Old Style", serif' } })), '"Iowan Old Style", serif');
  assert.equal(face(page({ live: true, theme: themeNoFont })), "Georgia, serif");
  assert.equal(face(page({ live: true, theme: "", host: { font: "Menlo" } })), "Menlo");
  assert.equal(lh(page({ live: true, theme: themeNoFont, host: { lineHeight: 1.75 } })), "1.75");
  assert.equal(lh(page({ live: true, theme: themeNoFont })), "1.6");
});

test("exact Live follows the same face rule", () => {
  assert.equal(face(page({ live: true, exact: true, theme: "body { font-family: Palatino }", host: { font: "Menlo" } })), "Palatino");
  assert.equal(face(page({ live: true, exact: true, theme: "", host: { font: "Menlo" } })), "Menlo");
});

test("Source keeps the host face on the content and the host line height on #write, whatever the theme's body says", () => {
  const theme = "body { font-family: Palatino; line-height: 1.62 }";
  const p = page({ live: false, theme, host: { font: "Menlo", lineHeight: 2 } });
  assert.equal(face(p), "Menlo");
  assert.equal(lh(p), "2");
  const mono = page({ live: false, theme });
  assert.equal(face(mono), '"iA Writer Mono S", Menlo, monospace');
  assert.equal(lh(mono), "1.6");
  // a theme's own `#write { line-height }` still wins in Source (zero-specificity host rule, theme comes later)
  assert.equal(lh(page({ live: false, theme: "#write { line-height: 1.9 }", host: { lineHeight: 2 } })), "1.9");
});

test("applyAppearanceDom mirrors --fw-font and --fw-line-height onto <html> (the Live fallback reads them there)", () => {
  const doc = fakeDoc();
  applyAppearanceDom({ fontFamily: "Menlo", lineHeight: 1.8, fontSize: 18 }, doc);
  assert.equal(doc.documentElement.style.get("--fw-font"), "Menlo");
  assert.equal(doc.documentElement.style.get("--fw-line-height"), "1.8");
  assert.equal(doc.write.style.get("--fw-font"), "Menlo");
  assert.equal(doc.documentElement.style.get("--fw-font-size"), undefined, "size is not mirrored");
  applyAppearanceDom({ fontFamily: "" }, doc); // empty clears
  assert.equal(doc.documentElement.style.get("--fw-font"), undefined);
  assert.equal(doc.write.style.get("--fw-font"), undefined);
});

test("fw-live-root is on <html> in Live, filtered or exact, and off in Source", () => {
  const doc = fakeDoc();
  const on = () => doc.documentElement.classList.has(LIVE_ROOT_CLASS);
  applyLiveTheme({ mode: "source", filter: true }, doc);
  assert.ok(!on());
  applyLiveTheme({ mode: "live", filter: true }, doc);
  assert.ok(on());
  applyLiveTheme({ mode: "live", filter: false }, doc);
  assert.ok(on());
  applyLiveTheme({ mode: "source", filter: false }, doc);
  assert.ok(!on());
  assert.equal(LIVE_ROOT_CLASS, "fw-live-root");
  assert.ok(EDITOR_CSS.includes(":where(html.fw-live-root)"));
  assert.ok(!/(^|\n)#write\.fw-mode-live[^{]*\{[^}]*font-family:\s*var\(--fw-font/.test(EDITOR_CSS.replace(/\.cm-content/g, "")), "no host face directly on #write in Live");
});

// ---------------------------------------------------------------- revealed marks beat the mapped theme copy
// The nesting comes from the real decoration builder (src/live.ts), ordered the way CodeMirror nests mark spans:
// outer = earlier start, then later end, then (equal ranges) LATER in the decoration set. The last rule was checked
// in the real DOM (dev server): `<span class="fw-revealed"><span class="fw-strong">**b**</span></span>`, for strong,
// em, code and strike alike, and `<span class="fw-revealed">[<span class="fw-live-link">l</span>](u)</span>`.
import { EditorSelection, EditorState } from "@codemirror/state";
import { markdown, markdownLanguage } from "@codemirror/lang-markdown";
import { buildLiveDecorations } from "../src/live.ts";

/** Mark classes (outer -> inner) wrapping the character at `pos` when the caret is at `caret`. */
function spanChain(doc, caret, pos) {
  const state = EditorState.create({ doc, selection: EditorSelection.single(caret), extensions: [markdown({ base: markdownLanguage })] });
  const found = [];
  let n = 0;
  buildLiveDecorations(state).between(0, doc.length, (from, to, d) => {
    if (d.spec.class && from < to && from <= pos && pos < to) found.push({ from, to, cls: d.spec.class, n: n++ });
  });
  return found.sort((a, b) => a.from - b.from || b.to - a.to || b.n - a.n).map((f) => f.cls);
}

const REVEAL_DOC = "x **b** *i* `c` ~~s~~ [l](u) y";
const REVEAL_CASES = [
  { name: "link", caret: 25, pos: 23, mark: "fw-live-link" },
  { name: "strong", caret: 4, pos: 4, mark: "fw-strong" },
  { name: "em", caret: 10, pos: 9, mark: "fw-em" },
  { name: "strike", caret: 19, pos: 18, mark: "fw-strike" },
  { name: "code", caret: 14, pos: 13, mark: "fw-code" },
];

test("live.ts really emits separate revealed and mark spans (the structure the CSS has to match)", () => {
  for (const c of REVEAL_CASES) {
    const chain = spanChain(REVEAL_DOC, c.caret, c.pos);
    assert.ok(chain.includes("fw-revealed") && chain.includes(c.mark), `${c.name}: ${chain}`);
    assert.equal(chain.filter((x) => x === c.mark || x === "fw-revealed").length, 2, "two distinct spans, never one compound span");
  }
  // a link's mark covers only the label, so the revealed span wraps it
  assert.deepEqual(spanChain(REVEAL_DOC, 25, 23), ["fw-revealed", "fw-live-link"]);
});

test("revealed marks stay un-underlined and un-tinted whatever the theme says (filtered and exact, real nesting)", () => {
  const theme = "#write { color: #333 } a { color: #bc6a3a; text-decoration: underline } a:hover { color: red; text-decoration: underline } " +
    "strong { color: crimson } em { color: teal } del { color: gray } code { color: purple; background: #eee }";
  const mapped = mapThemeCss(theme);
  for (const c of ["fw-live-link", "fw-strong", "fw-em", "fw-strike", "fw-code"]) assert.ok(mapped.includes(c), c);
  const rules = [...parseRules(EDITOR_CSS), ...parseRules(BLOCKS_CSS, 5000), ...parseRules(theme, 10000), ...parseRules(mapped, 20000)];
  for (const exact of [false, true]) for (const c of REVEAL_CASES) {
    const html = celem("html", { classes: [LIVE_ROOT_CLASS] });
    const write = celem("div", { id: "write", parent: celem("body", { parent: html }), classes: ["fw-mode-live"].concat(exact ? ["fw-live-exact"] : []) });
    const line = celem("div", { parent: write, classes: ["cm-line"] });
    let node = line;
    const spans = spanChain(REVEAL_DOC, c.caret, c.pos).filter((k) => k === "fw-revealed" || k === c.mark);
    for (const k of spans) node = celem("span", { parent: node, classes: [k] });
    const plain = celem("span", { parent: line, classes: [c.mark] });
    for (const state of [new Set(), new Set(["hover"])]) {
      const where = `${c.name}, exact=${exact}, ${[...state]}`;
      const lineColour = cascade("color", line, rules, { state });
      if (c.mark === "fw-live-link") assert.equal(cascade("text-decoration", node, rules, { state, inherited: false }), "none", where);
      // every span in the chain (outer mark and revealed alike) reads the line colour
      for (let n = node; n !== line; n = n.parent) assert.equal(cascade("color", n, rules, { state }), lineColour, "chain: " + where);
    }
    assert.notEqual(cascade("color", plain, rules), cascade("color", line, rules), "an unrevealed " + c.name + " keeps the theme colour");
    if (c.mark === "fw-live-link") assert.equal(cascade("text-decoration", plain, rules, { inherited: false }), "underline");
    if (c.mark === "fw-code") assert.ok(/monospace|Menlo/.test(cascade("font-family", node, rules) ?? ""), "revealed code keeps its monospace face");
  }
});

test("the old compound-selector rule (`.fw-revealed.fw-live-link`) is gone: no span carries both classes", () => {
  assert.ok(!/\.fw-revealed\.fw-/.test(EDITOR_CSS.replace(/\/\*[\s\S]*?\*\//g, "")));
});

test("the revealed rules out-specify any mapped selector", () => {
  const revealed = [
    "#write#write.fw-mode-live .fw-revealed .fw-live-link",
    "#write#write.fw-mode-live .fw-revealed .fw-strong",
    "#write#write.fw-mode-live .fw-revealed .fw-em",
    "#write#write.fw-mode-live .fw-revealed .fw-code",
  ];
  for (const r of revealed) assert.ok(EDITOR_CSS.includes(r), r);
  assert.ok(EDITOR_CSS.includes("#write#write.fw-mode-live .fw-revealed { color: inherit; }"));
  for (const r of revealed) for (const sel of ["#write a", "#write a:hover", "#write a:focus", "#write.x h1 a:active", "#write li strong", "#write blockquote em", "#write > h2 code"]) {
    const m = mapThemeCss(`${sel} { color: red }`).split("{")[0].trim();
    assert.ok(m, sel);
    assert.ok(higher(r, m), `${r} vs ${m}: ${specificity(r)} vs ${specificity(m)}`);
  }
});

// ---------------------------------------------------------------- measureTheme on a fake DOM (H1)
const CREAM_RGB = "rgb(250, 245, 235)", DARK_BG = "rgb(30, 31, 34)", DARK_FG = "rgb(230, 237, 243)", WHITE_RGB = "rgb(255, 255, 255)";
const DARK_VARS = { "--fw-bg": "#1e1f22", "--fw-fg": "#e6edf3" };
const measure = (doc) => measureTheme(doc, doc.write);

test("measureTheme: a bundled preset in dark Live paints nothing (white <html> hides behind the editor's dark <body>)", () => {
  const doc = fakeDoc({ vars: DARK_VARS, body: { backgroundColor: DARK_BG, color: DARK_FG }, html: { backgroundColor: WHITE_RGB, color: "rgb(36, 41, 47)" }, computed: { color: DARK_FG } });
  assert.deepEqual(measure(doc), { bg: null, fg: null });
  applyLiveTheme({ mode: "live", filter: true, palette: DARK }, doc);
  assert.deepEqual(fillOf(doc), { bg: DARK.background, fg: DARK.text }, "the palette pair: light text on a dark page, never #111 on dark");
});

test("measureTheme: a bundled preset in light Live is also unset", () => {
  const doc = fakeDoc({ html: { backgroundColor: WHITE_RGB, color: "rgb(36, 41, 47)" } });
  assert.deepEqual(measure(doc), { bg: null, fg: null });
});

test("measureTheme: claude-like (cream body, text set) reports a background; dark editor too", () => {
  const light = fakeDoc({ body: { backgroundColor: CREAM_RGB }, html: { backgroundColor: CREAM_RGB }, computed: { color: "rgb(61, 57, 41)" } });
  assert.deepEqual(measure(light), { bg: CREAM_RGB, fg: "rgb(61, 57, 41)" });
  const dark = fakeDoc({ vars: DARK_VARS, body: { backgroundColor: CREAM_RGB }, html: { backgroundColor: DARK_BG }, computed: { color: "rgb(61, 57, 41)" } });
  assert.deepEqual(measure(dark), { bg: CREAM_RGB, fg: "rgb(61, 57, 41)" });
  applyLiveTheme({ mode: "live", filter: true, palette: DARK }, dark);
  assert.deepEqual(fillOf(dark), { bg: null, fg: null }, "both set: nothing filled");
});

test("measureTheme: an opaque body decides; html is never consulted past it", () => {
  // body painted dark (the theme's own dark), html white: white never shows
  const bodyDark = fakeDoc({ body: { backgroundColor: "rgb(20, 20, 30)" }, html: { backgroundColor: WHITE_RGB }, computed: { color: "rgb(220, 220, 220)" } });
  assert.deepEqual(measure(bodyDark), { bg: "rgb(20, 20, 30)", fg: "rgb(220, 220, 220)" });
  // body is the editor's own colour, html is cream: still unset (html is not looked at)
  const bodyEditor = fakeDoc({ vars: DARK_VARS, body: { backgroundColor: DARK_BG }, html: { backgroundColor: CREAM_RGB } });
  assert.equal(measure(bodyEditor).bg, null);
  // a transparent body lets html through
  const bodyClear = fakeDoc({ body: { backgroundColor: "rgba(0, 0, 0, 0)" }, html: { backgroundColor: CREAM_RGB } });
  assert.equal(measure(bodyClear).bg, CREAM_RGB);
  // #write's own background wins over both
  const own = fakeDoc({ computed: { backgroundColor: "rgb(10, 10, 10)" }, body: { backgroundColor: CREAM_RGB } });
  assert.equal(measure(own).bg, "rgb(10, 10, 10)");
});

test("measureTheme: `html { color }` alone never reaches #write (the editor's body colour overrides it): unset", () => {
  // the browser computes #write's colour from body's `color: var(--fw-fg)`, so it equals the editor default
  const doc = fakeDoc({ html: { color: "rgb(120, 40, 40)" }, computed: { color: "rgb(36, 41, 47)" } });
  assert.deepEqual(measure(doc), { bg: null, fg: null });
  // a colour on body or #write does reach it
  const body = fakeDoc({ computed: { color: "rgb(120, 40, 40)" } });
  assert.deepEqual(measure(body), { bg: null, fg: "rgb(120, 40, 40)" });
  applyLiveTheme({ mode: "live", filter: true, palette: LIGHT }, body);
  assert.equal(fillOf(body).fg, null);
  assert.ok(fillOf(body).bg, "text set, background unset: palette background chosen for contrast with the text");
});

test("measureTheme: no window gives null", () => {
  const doc = fakeDoc();
  doc.defaultView = null;
  assert.equal(measure(doc), null);
});

// ---------------------------------------------------------------- follow-ons: inline code and table under the palette fill
test("the palette text fill drops a mapped inline-code background; the rule is inert otherwise", () => {
  const rule = "#write#write.fw-mode-live.fw-live-fill-fg .fw-code { background-color: var(--fw-code-bg) !important; }";
  assert.ok(EDITOR_CSS.includes(rule));
  assert.equal(FILL_FG_CLASS, "fw-live-fill-fg");
  const theme = "code { background: #f3f4f6 }";
  const rules = [...parseRules(EDITOR_CSS), ...parseRules(BLOCKS_CSS, 5000), ...parseRules(theme, 10000), ...parseRules(mapThemeCss(theme), 20000)];
  const page = (fill) => {
    const write = celem("div", { id: "write", parent: celem("body", { parent: celem("html", { vars: { "--fw-code-bg": "rgba(110, 118, 129, 0.3)" } }) }), classes: ["fw-mode-live", ...(fill ? [FILL_FG_CLASS] : [])] });
    return celem("span", { parent: celem("div", { parent: write, classes: ["cm-line"] }), classes: ["fw-code"] });
  };
  assert.equal(cascade("background-color", page(false), rules, { inherited: false }), "#f3f4f6");
  assert.equal(cascade("background-color", page(true), rules, { inherited: false }), "rgba(110, 118, 129, 0.3)");
});

test("the real table: header band is a translucent tint; theme row / cell backgrounds are dropped under the text fill", () => {
  // M2: the head background no longer depends on the editor's dark / light colour scheme
  const heads = [...BLOCKS_CSS.matchAll(/--fw-table-head-bg:\s*([^;]+);/g)].map((m) => m[1]);
  assert.equal(heads.length, 2);
  for (const h of heads) assert.ok(/^rgba\(\s*127,\s*127,\s*127,\s*0?\.\d+\)$/.test(h), h);
  assert.ok(BLOCKS_CSS.includes(".fw-table th { background: var(--fw-table-head-bg); font-weight: 600; }"));
  // presets: `tr:nth-child(even) { background: #f8f9fa }` stays light under light palette text unless dropped
  assert.ok(BLOCKS_CSS.includes("#write#write.fw-mode-live.fw-live-fill-fg .fw-table tr"));
  assert.ok(/fw-live-fill-fg \.fw-table td \{ background-color: transparent !important; \}/.test(BLOCKS_CSS));
  assert.ok(BLOCKS_CSS.includes("fw-live-fill-fg .fw-table tbody tr:nth-child(even) td { background-color: var(--fw-table-stripe-bg) !important; }"));
});

test("the header band reads under a light theme in a dark editor (dark-claude-like): theme text over a neutral tint", () => {
  // The header's text is the theme's own colour (inherited); the band is rgba(127,127,127,.12) over the cream page.
  const tint = 0.12, cream = [250, 245, 235], grey = 127;
  const band = cream.map((c) => Math.round(c * (1 - tint) + grey * tint));
  assert.ok(contrastRatio(`rgb(${band.join(",")})`, "rgb(61, 57, 41)") >= 4.5, "theme text on the tinted band");
});

// ---------------------------------------------------------------- the cascade helper is loud about what it cannot model
import { matches } from "./cascade.mjs";

test("cascade.mjs: child combinators are evaluated (a `#write > *` rule really matches), unknown selectors throw", () => {
  const write = celem("div", { id: "write" });
  const editor = celem("div", { parent: write, id: "editor" });
  const line = celem("div", { parent: editor, classes: ["cm-line"] });
  assert.ok(matches("#write > *", editor));
  assert.ok(!matches("#write > *", line), "a grandchild is not a child");
  assert.ok(matches("#write > #editor > .cm-line", line));
  assert.ok(matches("#write .cm-line", line));
  assert.ok(!matches("#write > .cm-line", line));
  assert.ok(matches(".cm-line:not(#editor)", line));
  assert.ok(!matches(".cm-line::before", line), "pseudo-elements never match an element");
  for (const bad of [".a + .b", ".a ~ .b", "#write > ", "li:nth-child(2)", "a[href]"]) assert.throws(() => matches(bad, line), /cascade:/, bad);
});

test("cascade.mjs: an overlay `#write > *` rule is evaluated, not silently skipped", () => {
  // With the old silent catch this rule never matched, so `margin` fell through to the theme's value.
  const rules = parseRules(LIVE_OVERLAY_CSS);
  const html = celem("html");
  const write = celem("div", { id: "write", parent: celem("body", { parent: html }), classes: ["fw-mode-live"] });
  const wrapper = celem("div", { parent: write, id: "editor" });
  const theme = parseRules("#write > #editor { margin: 40px auto }", 10000);
  assert.equal(cascade("margin", wrapper, [...theme, ...rules], { inherited: false }), "0");
  // and a rule it cannot model that declares the asked property is a loud failure, not a quiet non-match
  assert.throws(() => cascade("margin", wrapper, parseRules("#write + p { margin: 1px }"), { inherited: false }), /cascade: sibling/);
  assert.equal(cascade("color", wrapper, parseRules("#write + p { margin: 1px }"), { inherited: false }), undefined, "unrelated property: ignored");
});
