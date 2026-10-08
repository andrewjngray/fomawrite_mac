import test from "node:test";
import assert from "node:assert/strict";
import { LIVE_SCOPE, mapSelector, mapThemeCss, stripComments } from "../src/thememap.ts";

const S = "#write.fw-mode-live:not(.fw-live-exact)";

/** Parse mapper output into [{selectors, body}] for top-level rules (at-rules are returned as {at, text}). */
function rules(css) {
  const out = [];
  let i = 0;
  while (i < css.length) {
    while (/\s/.test(css[i] ?? "x")) i++;
    if (i >= css.length) break;
    const open = css.indexOf("{", i);
    let depth = 0, j = open;
    for (; j < css.length; j++) {
      if (css[j] === "{") depth++;
      else if (css[j] === "}" && --depth === 0) break;
    }
    const head = css.slice(i, open).trim(), body = css.slice(open + 1, j);
    out.push(head.startsWith("@") ? { at: head, text: css.slice(i, j + 1), body } : { selectors: head.split(/,\s*(?=#write)/), body: body.trim() });
    i = j + 1;
  }
  return out;
}
const find = (css, selector) => rules(css).find((r) => r.selectors?.includes(S + " " + selector));
const props = (r) => r.body.split(";").map((d) => d.trim()).filter(Boolean);

// A Typora-like theme: every kind of rule the mapper has to decide on.
const THEME = `
/* Typora-ish theme */
@font-face { font-family: "Lora"; src: url(fonts/lora.woff2) format("woff2"); font-weight: 400; }
#write { font-size: 30px; max-width: 800px; margin: 0 auto; padding: 2em; background: #fafafa url(paper.png) }
#write > h1 { color: crimson; font-family: Georgia; border-bottom: 2px solid #ccc; margin: 2em 0 1em; position: relative; font-size: 2rem !important }
#write h1 { font-weight: 700 }
h2, h3 { color: #333; letter-spacing: 0.02em; width: 100%; float: left }
#write blockquote { color: gray; font-style: italic; border-left: 4px solid #ddd; padding: 0 1em; padding-left: 1.2em; margin: 0 }
#write blockquote > p { color: #555; font-size: 18px; margin: 0 }
#write code { color: purple; background: #eee; border: 1px solid #ddd; border-radius: 3px; padding: 2px 4px }
#write pre code { color: red }
#write a { color: teal; text-decoration: none; transition: color .2s }
#write a:hover { text-decoration: underline }
#write strong { font-weight: 800 }
#write em { color: #a00 }
#write del { color: #888 }
#write hr { border: 1px solid #ccc }
#write table td { color: red }
#write ul li, #write ol > li { line-height: 1.8; list-style: square }
#write p { line-height: 1.7; font-size: 1rem; text-align: justify; margin-bottom: 1em; text-transform: none }
@media (max-width: 600px) { #write h1 { font-size: 1.5em } #write p { color: #111 } #write img { width: 100% } }
@keyframes pulse { from { opacity: 0 } to { opacity: 1 } }
body { background: #fff; padding: 20px }
`;

test("realistic theme: mapped selectors and kept properties", () => {
  const css = mapThemeCss(THEME);
  const all = rules(css);

  // headings: typography + border-bottom/left only, !important kept, rem -> em
  const h1 = all.filter((r) => r.selectors?.includes(`${S} .cm-line.fw-h1`));
  assert.equal(h1.length, 2, "both `#write > h1` and `#write h1` map");
  assert.deepEqual(props(h1[0]), ["color: crimson", "font-family: Georgia", "border-bottom: 2px solid #ccc", "font-size: 2em !important"]);
  assert.deepEqual(props(h1[1]), ["font-weight: 700"]);

  // selector list `h2, h3` (no #write at all) -> both lines, layout dropped
  const h23 = find(css, ".cm-line.fw-h2");
  assert.deepEqual(h23.selectors, [`${S} .cm-line.fw-h2`, `${S} .cm-line.fw-h3`]);
  assert.deepEqual(props(h23), ["color: #333", "letter-spacing: 0.02em"]);

  // blockquote: typography + border-left; padding-left only on depth-1 quotes, forced past the editor's own
  const quote = find(css, ".cm-line.fw-quote-line");
  assert.deepEqual(props(quote), ["color: gray", "font-style: italic", "border-left: 4px solid #ddd"]);
  const quote1 = find(css, ".cm-line.fw-quote-line.fw-quote-d1");
  assert.deepEqual(props(quote1), ["padding-left: 1.2em !important"]);
  // `blockquote > p` is the same line; the absolute font-size on body text is dropped, colour kept
  const quoteP = all.filter((r) => r.selectors?.includes(`${S} .cm-line.fw-quote-line`));
  assert.deepEqual(props(quoteP[1]), ["color: #555"]);

  // inline marks
  assert.deepEqual(props(find(css, ".fw-code")), ["color: purple", "background-color: #eee"]);
  assert.deepEqual(props(find(css, ".fw-live-link")), ["color: teal", "text-decoration: none"]);
  assert.deepEqual(props(find(css, ".fw-live-link:hover")), ["text-decoration: underline"]);
  assert.deepEqual(props(find(css, ".fw-strong")), ["font-weight: 800"]);
  assert.deepEqual(props(find(css, ".fw-em")), ["color: #a00"]);
  assert.deepEqual(props(find(css, ".fw-strike")), ["color: #888"]);

  // list items: both `ul li` and `ol > li` collapse onto the one list line class
  const li = find(css, ".cm-line.fw-list-line");
  assert.deepEqual(li.selectors, [`${S} .cm-line.fw-list-line`], "ul / ol ancestors drop out and the selectors dedupe");
  assert.deepEqual(props(li), ["line-height: 1.8"]);

  // p: every line that is not a heading / list / code / table / front matter / math / footnote line
  const p = all.find((r) => r.selectors?.[0].includes(":not(:where("));
  assert.ok(p.selectors[0].startsWith(`${S} .cm-line:not(:where(.fw-h1,`));
  for (const c of [".fw-h6", ".fw-list-line", ".fw-code-line", ".fw-table-line", ".fw-front-matter-line"]) assert.ok(p.selectors[0].includes(c), c);
  assert.deepEqual(props(p), ["line-height: 1.7", "font-size: 1em", "text-transform: none"]);

  // real elements stay with the theme: nothing is emitted for pre/code, hr, table, img, body
  for (const bad of [" pre", " hr", " table", " img", " body", " td", "fw-code { color: red"]) assert.ok(!css.includes(bad), bad);
  assert.ok(!css.includes("color: red"), "`pre code` and `table td` are not mapped");
  assert.ok(!css.includes("padding: 20px") && !css.includes("#fff"), "body rule is not mapped");
});

test("@media is descended into, @font-face and @keyframes pass through untouched", () => {
  const css = mapThemeCss(THEME);
  assert.ok(css.includes('@font-face { font-family: "Lora"; src: url(fonts/lora.woff2) format("woff2"); font-weight: 400; }'));
  assert.ok(css.includes("@keyframes pulse { from { opacity: 0 } to { opacity: 1 } }"));
  const media = rules(css).find((r) => r.at?.startsWith("@media"));
  assert.equal(media.at, "@media (max-width: 600px)");
  const inner = rules(media.body);
  assert.equal(inner.length, 2, "the `img` rule inside the media block is not mapped");
  assert.deepEqual(props(inner[0]), ["font-size: 1.5em"]);
  assert.ok(inner[0].selectors[0].startsWith(`${S} .cm-line.fw-h1`));
  assert.deepEqual(props(inner[1]), ["color: #111"]);
  // a media block with nothing mappable disappears
  assert.equal(mapThemeCss("@media print { #write img { width: 100% } #write table { color: red } }"), "");
  // @supports descends too; @import / @charset / @page are ignored
  assert.ok(mapThemeCss("@supports (display: grid) { #write h2 { color: red } }").startsWith("@supports (display: grid) {"));
  assert.equal(mapThemeCss('@charset "utf-8"; @import url(a.css); @page { margin: 1in }'), "");
});

test("mapped copies never contain layout properties", () => {
  const layout = [
    "position: absolute", "float: left", "width: 50%", "max-width: 40em", "margin: 0 auto", "margin-left: 0", "margin-right: 0",
    "display: block", "columns: 2", "column-count: 2", "transform: scale(2)", "background-image: url(a.png)",
    "box-shadow: 0 0 4px red", "padding: 1em", "padding-top: 1em", "height: 3em", "top: 0", "left: 0", "overflow: hidden", "content: 'x'",
    "border: 1px solid red", "border-top: 1px solid red", "border-radius: 4px", "list-style: none", "text-align: center", "text-indent: 2em",
    "background: url(a.png) no-repeat", "background: linear-gradient(red, blue)", "--theme-gap: 4px",
  ];
  const sels = ["h1", "h4", "blockquote", "p", "li", "code", "a", "strong", "em", "del"];
  const css = mapThemeCss(sels.map((s) => `#write ${s} { ${layout.join("; ")}; color: blue }`).join("\n"));
  assert.ok(css.length > 0);
  for (const p of ["position", "float", "width", "margin", "display", "columns", "column-count", "transform", "background-image", "box-shadow", "padding", "height", "top:", "left:", "overflow", "content", "url(", "gradient", "--theme", "list-style", "text-align", "text-indent", "border-radius"])
    assert.ok(!css.includes(p), p);
  // only heading and quote lines keep (only) the left / bottom borders
  const withBorder = rules(css).filter((r) => /\bborder-/.test(r.body)).flatMap((r) => r.selectors);
  assert.ok(withBorder.every((s) => /fw-h1|fw-h4|fw-quote-line/.test(s)), withBorder.join("\n"));
  assert.ok(!/border: 1px|border-top/.test(css));
  // inline code is the only kind that keeps a background colour, and only a colour
  assert.ok(css.includes("background-color") === false, "no background colour was given");
  const bg = mapThemeCss("#write code { background: #eee } #write a { background: #eee } #write p { background-color: #eee } #write code.x { color: red }");
  assert.ok(bg.includes(`${S} .fw-code { background-color: #eee; }`));
  assert.equal(bg.split("background").length - 1, 1);
  assert.ok(!bg.includes(".x"));
});

test("!important survives, rem becomes em, absolute sizes on body text are dropped", () => {
  const css = mapThemeCss("#write h2 { color: red !important; font-size: 1.8rem !IMPORTANT } #write p { font-size: 17px; color: #222 !important; line-height: 1.5 } #write li { font: 700 14px Georgia } #write h3 { font: 700 1.2rem/1.3 Georgia }");
  assert.ok(css.includes("color: red !important; font-size: 1.8em !important"));
  assert.ok(css.includes("color: #222 !important; line-height: 1.5"));
  assert.ok(!css.includes("17px") && !css.includes("14px"), "host size wins on p / li");
  assert.ok(css.includes("font: 700 1.2em/1.3 Georgia"));
  // headings keep an absolute size (the theme's choice)
  assert.ok(mapThemeCss("#write h1 { font-size: 32px }").includes("font-size: 32px"));
});

test("mapSelector: accepted forms", () => {
  const m = (s) => mapSelector(s)?.css;
  assert.equal(m("#write h1"), `${S} .cm-line.fw-h1`);
  assert.equal(m("#write > h1"), `${S} .cm-line.fw-h1`);
  assert.equal(m("#write>h6"), `${S} .cm-line.fw-h6`);
  assert.equal(m("h2"), `${S} .cm-line.fw-h2`);
  assert.equal(m("#write blockquote"), `${S} .cm-line.fw-quote-line`);
  assert.equal(m("#write blockquote > p"), `${S} .cm-line.fw-quote-line`);
  assert.equal(m("#write blockquote blockquote"), `${S} .cm-line.fw-quote-line`);
  assert.equal(m("#write ul > li"), `${S} .cm-line.fw-list-line`);
  assert.equal(m("#write ol li p"), `${S} .cm-line.fw-list-line`);
  assert.equal(m("#write blockquote li"), `${S} .cm-line.fw-quote-line.fw-list-line`);
  assert.equal(m("#write a"), `${S} .fw-live-link`);
  assert.equal(m("#write a:hover"), `${S} .fw-live-link:hover`);
  assert.equal(m("#write a:link"), `${S} .fw-live-link`, ":link matches every Live link");
  assert.equal(m("#write b"), `${S} .fw-strong`);
  assert.equal(m("#write i"), `${S} .fw-em`);
  assert.equal(m("#write s"), `${S} .fw-strike`);
  assert.equal(m("#write strike"), `${S} .fw-strike`);
  assert.equal(m("#write code"), `${S} .fw-code`);
  assert.equal(m("#write p code"), `${S} .cm-line:not(:where(.fw-h1,.fw-h2,.fw-h3,.fw-h4,.fw-h5,.fw-h6,.fw-list-line,.fw-code-line,.fw-table-line,.fw-front-matter-line,.fw-math-src-line,.fw-footnote-def)) .fw-code`);
  assert.equal(m("#write h1 code"), `${S} .cm-line.fw-h1 .fw-code`);
  assert.equal(m("#write h2 > a:hover"), `${S} .cm-line.fw-h2 .fw-live-link:hover`);
  assert.equal(m("#write li em strong"), `${S} .cm-line.fw-list-line .fw-em .fw-strong`);
  assert.equal(m("#write.typora-export h1"), `${S}.typora-export .cm-line.fw-h1`);
  assert.equal(m("  #write   h1  "), `${S} .cm-line.fw-h1`);
  assert.equal(m("#write p:hover"), `${S} .cm-line:not(:where(.fw-h1,.fw-h2,.fw-h3,.fw-h4,.fw-h5,.fw-h6,.fw-list-line,.fw-code-line,.fw-table-line,.fw-front-matter-line,.fw-math-src-line,.fw-footnote-def)):hover`);
  assert.equal(mapSelector("#write blockquote").cssDepth1, `${S} .cm-line.fw-quote-line.fw-quote-d1`);
  assert.equal(LIVE_SCOPE, S);
});

test("mapSelector: rejected forms", () => {
  for (const s of [
    "#write", "#write > *", "#write pre", "#write pre code", "#write pre > code", "#write table", "#write td code", "#write img", "#write hr",
    "#write h1 + p", "#write h1 ~ p", "#write p:first-child", "#write li:nth-child(2)", "#write h1:not(.x)", "#write a:visited",
    "#write a[href^=http]", "#write h1.title", "#write p.lead", "#write code#x", "#write h1::before", "#write blockquote::after",
    "#write li::marker", "#write a:hover code", "#write ul", "#write ol", "#write ul a", "#write div", "#write span", ".md-fences code",
    "#editor h1", "body h1", "html", "h1 #write", "#write a h1", "> p", "#write >", "", "#write h1[", "#write :hover",
  ]) assert.equal(mapSelector(s), null, JSON.stringify(s));
  // a non-:link anchor pseudo on a non-anchor is not silently dropped
  assert.equal(mapSelector("#write p:link"), null);
});

test("quote padding-left is forced for depth-1 quotes only and never emitted for other kinds", () => {
  const css = mapThemeCss("#write blockquote { padding-left: 2em } #write li { padding-left: 2em } #write p { padding-left: 2em }");
  assert.equal(css, `${S} .cm-line.fw-quote-line.fw-quote-d1 { padding-left: 2em !important; }`);
});

test("comments, strings and malformed input are tolerated", () => {
  assert.equal(stripComments('a /* x */ b "/* not */" c'), 'a   b "/* not */" c');
  const css = mapThemeCss(`
    /* #write h1 { color: red } */
    #write h2 /* inline */ { color: /* c */ blue; }
    #write h3 { font-family: "Fira; Sans", serif; color: green }
    #write h4 { content: "}"; color: teal }
  `);
  assert.ok(!css.includes("fw-h1"));
  assert.ok(css.includes(".fw-h2 { color: blue; }") || css.includes(".fw-h2 { color:  blue; }") || /\.fw-h2 \{ color: +blue; \}/.test(css), css);
  assert.ok(css.includes('font-family: "Fira; Sans", serif; color: green'));
  assert.ok(css.includes("fw-h4 { color: teal; }"));
  for (const bad of ["", "{", "}", "}}}", "#write h1 {", "#write h1 { color: red", "#write h1", "@media { #write h1 { color: red }", "@media", "#write h1 { color: red } }", "a{b{c{d}", "\u0000\u{1F600}", "#write h1 { color: red; ;; : ; : x }"]) {
    assert.doesNotThrow(() => mapThemeCss(bad), JSON.stringify(bad));
    assert.equal(typeof mapThemeCss(bad), "string");
  }
  assert.ok(mapThemeCss("#write h1 { color: red").includes("fw-h1 { color: red; }"), "unterminated block still maps");
  assert.equal(mapThemeCss(null), "");
  assert.equal(mapThemeCss(undefined), "");
  // nested CSS rules (`&`) are skipped rather than mangled
  assert.equal(mapThemeCss("#write h1 { color: red; & em { color: blue } }"), "");
  // pure: same input, same output
  assert.equal(mapThemeCss(THEME), mapThemeCss(THEME));
});

test("every mapped selector is scoped to filtered Live", () => {
  const css = mapThemeCss(THEME);
  const heads = css.replace(/@font-face[^}]*}|@keyframes[^{]*\{(?:[^{}]|\{[^}]*\})*\}/g, "").match(/[^{}]+(?=\{)/g).map((h) => h.trim()).filter((h) => !h.startsWith("@"));
  assert.ok(heads.length > 10);
  for (const head of heads) for (const sel of head.split(/,\s*(?=#write)/)) assert.ok(sel.startsWith(S + " ") || sel.startsWith(S + "."), sel);
});
