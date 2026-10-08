import test from "node:test";
import assert from "node:assert/strict";
import {
  FILL_BG_CLASS, FILL_FG_CLASS, LIVE_EXACT_CLASS, LIVE_OVERLAY_CSS, LIVE_OVERLAY_ID,
  HOST_SIZE_CLASS, LIVE_HTML_CLASS, MAPPED_ID, THEME_ID, applyLiveOverlay, applyLiveTheme, applyThemeMapped, isUnsetColour, parseLiveThemePatch, parsePalette, pickReadableColours,
} from "../src/livetheme.ts";
import { INITIAL_APPEARANCE, effectiveAppearanceClasses, parseAppearancePatch, reduceAppearance } from "../src/appearance.ts";
import { Session, applyAppearanceDom, applyThemeDom } from "../src/modes.ts";

// ---------------------------------------------------------------- tiny fake Document
function fakeDoc({ computed = {} } = {}) {
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
  const html = el("html");
  const doc = {
    head, documentElement: html, write,
    createElement: el,
    getElementById: (id) => (id === "write" ? write : head.children.find((c) => c.id === id) ?? null),
    defaultView: {
      getComputedStyle: (e) => (e === write ? { backgroundColor: "rgba(0, 0, 0, 0)", color: "rgb(36, 41, 47)", ...computed } : { color: "rgb(36, 41, 47)" }),
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
  assert.deepEqual(pickReadableColours("rgba(0, 0, 0, 0)", "rgb(0, 0, 0)", PALETTE), { background: "#101010", text: null });
  assert.deepEqual(pickReadableColours("rgb(255, 255, 255)", null, PALETTE), { background: null, text: "#eeeeee" });
  assert.deepEqual(pickReadableColours("transparent", "", PALETTE), { background: "#101010", text: "#eeeeee" });
  assert.deepEqual(pickReadableColours("rgba(0, 0, 0, 0)", null, undefined), { background: null, text: null });
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
  // exact mode removes it with the overlay; the theme element itself is untouched
  applyThemeDom(THEME, doc);
  applyLiveTheme({ mode: "live", filter: true }, doc);
  assert.ok(has(doc, MAPPED_ID));
  applyLiveTheme({ mode: "live", filter: false }, doc);
  assert.deepEqual(ids(doc), [THEME_ID]);
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
    assert.ok(!has(doc, MAPPED_ID) && !doc.documentElement.classList.has(LIVE_HTML_CLASS));
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
  assert.ok(mapped.includes("font-size: 2.2em") && mapped.includes("font-size: 1.5em"));
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
    "margin: 0 !important", "padding: 0 !important",
  ]);
  assert.ok(!/background-color|background:|\bcolor\b/.test(m[1]));
});
