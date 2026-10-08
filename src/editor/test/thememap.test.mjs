import test from "node:test";
import assert from "node:assert/strict";
import { existsSync, readFileSync, readdirSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import { LIVE_SCOPE, MAPPED_SCOPE, mapSelector, mapThemeCss, stripComments } from "../src/thememap.ts";

const S = "#write.fw-mode-live"; // mapped copies are scoped to every Live (filtered and exact)

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
  assert.deepEqual(props(quote1), ["padding-left: 1.2em !important"], "padding-left after the `padding` shorthand wins");
  assert.deepEqual(props(find(css, ".cm-line.fw-quote-line.fw-quote-d3")), ["padding-left: calc(1.2em + 2em) !important"]);
  // `blockquote > p` is the same line; its absolute size is relative to the theme's `#write` size (30px): 18 / 30
  const quoteP = all.filter((r) => r.selectors?.includes(`${S} .cm-line.fw-quote-line`));
  assert.deepEqual(props(quoteP[1]), ["color: #555", "font-size: 0.6em"]);

  // inline marks
  assert.deepEqual(props(find(css, ".fw-code")), ["color: purple", "background-color: #eee"]);
  assert.deepEqual(props(find(css, ".fw-live-link")), ["color: teal", "text-decoration: none"]);
  assert.deepEqual(props(find(css, ".fw-live-link:hover")), ["text-decoration: underline"]);
  assert.deepEqual(props(find(css, ".fw-strong")), ["font-weight: 800"]);
  assert.deepEqual(props(find(css, ".fw-em")), ["color: #a00"]);
  assert.deepEqual(props(find(css, ".fw-strike")), ["color: #888"]);

  // list items: `ul li` and `ol > li` map to their own kind of list line
  const li = find(css, ".cm-line.fw-list-line.fw-list-ul");
  assert.deepEqual(li.selectors, [`${S} .cm-line.fw-list-line.fw-list-ul`, `${S} .cm-line.fw-list-line.fw-list-ol`]);
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
  const sels = ["h1", "h4", "p", "li", "code", "a", "strong", "em", "del"]; // blockquote: "quote spacing" test below
  const css = mapThemeCss(sels.map((s) => `#write ${s} { ${layout.join("; ")}; color: blue }`).join("\n"));
  assert.ok(css.length > 0);
  for (const p of ["position", "float", "width", "margin", "display", "columns", "column-count", "transform", "background-image", "box-shadow", "padding", "height", "top:", "left:", "overflow", "content", "url(", "gradient", "--theme", "list-style", "text-align", "text-indent", "border-radius"])
    assert.ok(!css.includes(p), p);
  // only heading and quote lines keep (only) the left / bottom borders
  const withBorder = rules(css).filter((r) => /\bborder-/.test(r.body)).flatMap((r) => r.selectors);
  assert.ok(withBorder.every((s) => /fw-h1|fw-h4/.test(s)), withBorder.join("\n"));
  assert.ok(!/border: 1px|border-top/.test(css));
  // inline code is the only kind that keeps a background colour, and only a colour
  assert.ok(css.includes("background-color") === false, "no background colour was given");
  const bg = mapThemeCss("#write code { background: #eee } #write a { background: #eee } #write p { background-color: #eee } #write code.x { color: red }");
  assert.ok(bg.includes(`${S} .fw-code { background-color: #eee; }`));
  assert.equal(bg.split("background").length - 1, 1);
  assert.ok(!bg.includes(".x"));
});

test("!important survives, rem becomes em, absolute sizes become em against the theme base", () => {
  const css = mapThemeCss("#write h2 { color: red !important; font-size: 1.8rem !IMPORTANT } #write p { font-size: 17px; color: #222 !important; line-height: 1.5 } #write li { font: 700 14px Georgia } #write h3 { font: 700 1.2rem/1.3 Georgia }");
  assert.ok(css.includes("color: red !important; font-size: 1.8em !important"));
  assert.ok(css.includes("font-size: 1.0625em; color: #222 !important; line-height: 1.5"), "17px / 16px, no theme base");
  assert.ok(css.includes("font: 700 0.875em Georgia"), "li shorthand: 14px / 16px");
  assert.ok(!css.includes("17px") && !css.includes("14px"));
  assert.ok(css.includes("font: 700 1.2em/1.3 Georgia"));
  assert.ok(mapThemeCss("#write h1 { font-size: 32px !important }").includes("font-size: 2em !important"));
});

test("absolute heading sizes follow the host size: em against the theme's #write size, else 16px", () => {
  const one = (theme) => mapThemeCss(theme).replace(/^.*\{ /, "").replace(/; \}.*$/s, "");
  assert.equal(one("#write { font-size: 20px } #write h1 { font-size: 40px }"), "font-size: 2em");
  assert.equal(one("#write h1 { font-size: 40px }"), "font-size: 2.5em", "no base: 40 / 16");
  assert.equal(one("#write h1 { font-size: 30pt } #write { font-size: 15pt }"), "font-size: 2em", "pt on both sides; order does not matter");
  assert.equal(one("#write { font-size: 15pt } #write h2 { font-size: 20px }"), "font-size: 1em", "15pt = 20px");
  assert.equal(one("#write { font-size: 18px } #write h1 { font-size: 24pt }"), "font-size: 1.7778em", "24pt = 32px");
  assert.equal(one("#write { font-size: 1.1rem } #write h1 { font-size: 32px }"), "font-size: 2em", "a relative base is not a px base");
  assert.equal(one("#write { font-size: 100% } #write h1 { font-size: 24px }"), "font-size: 1.5em");
  assert.equal(one("#write { font-size: 10px } #write { font-size: 20px } #write h1 { font-size: 40px }"), "font-size: 2em", "last base wins");
  assert.equal(one("html, #write { font-size: 20px } #write h1 { font-size: 40px }"), "font-size: 2em", "#write in a selector list");
  assert.equal(one("#write h1 { font-size: 40px !important }"), "font-size: 2.5em !important");
  // `%`, `em`, `rem` and keywords stay as they are (rem -> em)
  assert.equal(one("#write { font-size: 20px } #write h1 { font-size: 200% }"), "font-size: 200%");
  assert.equal(one("#write { font-size: 20px } #write h1 { font-size: 1.5em }"), "font-size: 1.5em");
  assert.equal(one("#write { font-size: 20px } #write h1 { font-size: 2rem }"), "font-size: 2em");
  assert.equal(one("#write { font-size: 20px } #write h1 { font-size: xx-large }"), "font-size: xx-large");
  // every size-bearing kind converts; the font shorthand converts its size token only
  const css = mapThemeCss("#write { font-size: 20px } #write h3 { font: 700 30px/36px Georgia } #write blockquote { font-size: 10px } #write code { font-size: 15px } #write li { font-size: 25px } #write p { font-size: 20px } #write strong { font-size: 40px }");
  for (const x of ["font: 700 1.5em/36px Georgia", "font-size: 0.5em", "font-size: 0.75em", "font-size: 1.25em", "font-size: 1em", "font-size: 2em"]) assert.ok(css.includes(x), x);
  assert.ok(!/\dpx; |\dpx }/.test(css.replace("36px", "")), css);
  // inside a media block the base is still the theme's top-level one; an @media base does not count
  const media = mapThemeCss("#write { font-size: 20px } @media print { #write { font-size: 10px } #write h1 { font-size: 40px } }");
  assert.ok(media.includes("font-size: 2em"), media);
  // a calc() with an absolute length stays on headings, is dropped on body text
  assert.ok(mapThemeCss("#write h1 { font-size: calc(10px + 1em) }").includes("calc(10px + 1em)"));
  assert.equal(mapThemeCss("#write p { font-size: calc(10px + 1em) }"), "");
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
  assert.equal(m("#write ul > li"), `${S} .cm-line.fw-list-line.fw-list-ul`);
  assert.equal(m("#write ol li p"), `${S} .cm-line.fw-list-line.fw-list-ol`);
  assert.equal(m("#write ul ol li"), `${S} .cm-line.fw-list-line.fw-list-ol`, "the nearest list named before the li");
  assert.equal(m("#write ol ul > li"), `${S} .cm-line.fw-list-line.fw-list-ul`);
  assert.equal(m("#write li"), `${S} .cm-line.fw-list-line`, "bare li: both kinds");
  assert.equal(m("#write > li"), `${S} .cm-line.fw-list-line`);
  assert.equal(m("#write ul li a:hover"), `${S} .cm-line.fw-list-line.fw-list-ul .fw-live-link:hover`);
  assert.equal(m("#write ol li code"), `${S} .cm-line.fw-list-line.fw-list-ol .fw-code`);
  assert.equal(m("#write ul blockquote li"), `${S} .cm-line.fw-quote-line.fw-list-line.fw-list-ul`);
  assert.equal(m("#write ul p"), `${S} .cm-line:not(:where(.fw-h1,.fw-h2,.fw-h3,.fw-h4,.fw-h5,.fw-h6,.fw-list-line,.fw-code-line,.fw-table-line,.fw-front-matter-line,.fw-math-src-line,.fw-footnote-def))`, "no li: the list type is not used");
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
  assert.deepEqual(mapSelector("#write blockquote").cssDepths, [1, 2, 3, 4].map((n) => `${S} .cm-line.fw-quote-line.fw-quote-d${n}`));
  assert.equal(MAPPED_SCOPE, S);
  assert.equal(LIVE_SCOPE, "#write.fw-mode-live:not(.fw-live-exact)", "the overlay scope is unchanged");
});

test("mapSelector: rejected forms", () => {
  for (const s of [
    "#write", "#write > *", "#write pre", "#write pre code", "#write pre > code", "#write table", "#write td code", "#write img", "#write hr",
    "#write h1 + p", "#write h1 ~ p", "#write p:first-child", "#write li:nth-child(2)", "#write h1:not(.x)", "#write a:visited",
    "#write a[href^=http]", "#write h1.title", "#write p.lead", "#write code#x", "#write h1::before", "#write blockquote::after",
    "#write li::marker", "#write blockquote::before", "#write ul li::before", "#write ul", "#write a:hover code", "#write ul", "#write ol", "#write ul a", "#write div", "#write span", ".md-fences code",
    "#editor h1", "body h1", "html", "h1 #write", "#write a h1", "> p", "#write >", "", "#write h1[", "#write :hover",
  ]) assert.equal(mapSelector(s), null, JSON.stringify(s));
  // a non-:link anchor pseudo on a non-anchor is not silently dropped
  assert.equal(mapSelector("#write p:link"), null);
});

test("quote spacing: the left value of padding / padding-left / margin-left, nothing else", () => {
  const q = (decls) => mapThemeCss(`#write blockquote { ${decls} }`);
  const d = (n) => `${S} .cm-line.fw-quote-line.fw-quote-d${n}`;
  // padding-left alone: forced at depth 1, one em more per deeper level
  assert.equal(
    q("padding-left: 2em"),
    [`${d(1)} { padding-left: 2em !important; }`, `${d(2)} { padding-left: calc(2em + 1em) !important; }`,
      `${d(3)} { padding-left: calc(2em + 2em) !important; }`, `${d(4)} { padding-left: calc(2em + 3em) !important; }`].join("\n"),
  );
  // the shorthand: 1 value = all sides, 2 = vertical horizontal, 3 = top horizontal bottom, 4 = top right bottom left
  const left = (decls) => /padding-left: ([^;]+?) !important/.exec(q(decls))?.[1];
  assert.equal(left("padding: 12px"), "12px");
  assert.equal(left("padding: 0.5em 1.4em"), "1.4em");
  assert.equal(left("padding: 0.5em 1.4em 3em"), "1.4em");
  assert.equal(left("padding: 1px 2px 3px 4px"), "4px");
  assert.equal(left("padding:0.55rem   1rem"), "1rem");
  assert.equal(left("padding: 0 1em !important"), "1em");
  assert.equal(left("padding: 0"), "0");
  assert.equal(q("padding: 0").split("\n")[1], `${d(2)} { padding-left: 1em !important; }`, "zero + 1em needs no calc");
  assert.equal(left("padding: 1em; padding-left: 3em"), "3em", "later declaration wins");
  assert.equal(left("padding-left: 3em; padding: 1em 2em"), "2em");
  assert.equal(left("padding-left: 3em; padding: 1em 2em 3em 4em 5em"), "3em", "an invalid declaration is ignored");
  assert.equal(left("padding: 1em 5%"), "5%");
  // top / right / bottom never appear; unusable values give nothing
  assert.ok(!/padding-(?:top|right|bottom)/.test(q("padding-top: 1em; padding-right: 1em; padding-bottom: 1em; padding-left: 1em")));
  for (const bad of ["padding-top: 1em", "padding-right: 2em", "padding-bottom: 3em", "padding: var(--p)", "padding: 1em var(--p)", "padding: auto", "padding: 1em auto",
    "padding-left: auto", "padding-left: inherit", "padding-left: -2em", "padding: 5", "padding-left: url(a.png)"])
    assert.equal(q(bad), "", bad);
  assert.equal(left("padding-left: var(--indent)"), "var(--indent)", "a longhand var() is a single value");
  assert.equal(left("padding-left: calc(1em + 4px)"), "calc(1em + 4px)");

  // margin-left: a plain declaration on every depth; margin shorthand, right, top, bottom never
  assert.equal(q("margin-left: 1.5em"), `${S} .cm-line.fw-quote-line { margin-left: 1.5em; }`);
  assert.equal(q("margin: 0 2em; margin-right: 1em; margin-top: 1em; margin-bottom: 1em"), "");
  assert.equal(q("margin-left: -2em"), "");
  assert.equal(q("margin-left: auto"), "");
  assert.equal(q("margin-left: 1em !important"), `${S} .cm-line.fw-quote-line { margin-left: 1em !important; }`);

  // kept: border-left*, colour, font-style, background-colour (and a single-colour `background`); dropped: the rest
  const full = q(`color: #555; font-style: italic; border-left: 4px solid #ccc; border-left-color: red; background-color: #f3ede5; background: #eee;
    border: 1px solid red; border-right: 1px solid red; border-top: 1px solid red; border-bottom: 1px solid red; border-radius: 10px;
    margin: 1em 0; width: 50%; display: block; position: relative; background-image: url(a.png); box-shadow: 0 0 2px red; float: left;
    padding: 0.5em 1em; margin-left: 1em; max-width: 30em; text-indent: 1em; text-align: right`);
  const main = rules(full)[0];
  assert.deepEqual(props(main), ["color: #555", "font-style: italic", "border-left: 4px solid #ccc", "border-left-color: red", "background-color: #f3ede5", "background-color: #eee", "margin-left: 1em"]);
  assert.ok(!/(?:^|\s)(?:border|border-top|border-right|border-bottom|border-radius|margin|width|display|position|float|max-width|text-indent|text-align|box-shadow|background-image)\s*:/.test(rules(full).map((r) => r.body).join(";")));
  assert.equal(rules(full).length, 5, "main rule + padding-left at depths 1 to 4");
  assert.equal(q("background: url(a.png) no-repeat #eee"), "");
  assert.equal(q("background: linear-gradient(red, blue)"), "");

  // only quote lines take spacing; the same declarations on p / li / headings give nothing
  assert.equal(mapThemeCss("#write p, #write li, #write h1, #write code, #write a { padding: 1em 2em; padding-left: 2em; margin-left: 2em }"), "");
  // a quote's inner paragraph is the same line: its spacing is not the quote's
  assert.equal(mapThemeCss("#write blockquote > p { padding-left: 2em; margin-left: 2em }"), "");
  // selector lists: every quote selector shares the rule
  const list = mapThemeCss("#write blockquote, blockquote.x, .quote blockquote { padding-left: 1em }");
  assert.equal(rules(list).length, 4);
});

test("quote glyphs (::before / ::after) are deliberately not mapped", () => {
  const css = mapThemeCss(`#write blockquote::before { content: "\\201C"; color: red; font-size: 3em }
    #write blockquote::after { content: "\\201D"; color: red } #write blockquote:before { color: red } #write blockquote p::before { color: red }
    #write blockquote { color: blue }`);
  assert.equal(css, `${S} .cm-line.fw-quote-line { color: blue; }`);
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

test("every mapped selector is scoped to Live, filtered and exact alike", () => {
  const css = mapThemeCss(THEME);
  assert.ok(!css.includes("fw-live-exact"), "no :not(.fw-live-exact): exact mode keeps the mapped copy");
  const heads = css.replace(/@font-face[^}]*}|@keyframes[^{]*\{(?:[^{}]|\{[^}]*\})*\}/g, "").match(/[^{}]+(?=\{)/g).map((h) => h.trim()).filter((h) => !h.startsWith("@"));
  assert.ok(heads.length > 10);
  for (const head of heads) for (const sel of head.split(/,\s*(?=#write)/)) assert.ok(sel.startsWith(S + " ") || sel.startsWith(S + "."), sel);
});

// ---------------------------------------------------------------- robustness on real CSS
const REPO_SRC = fileURLToPath(new URL("../../", import.meta.url)); // <repo>/src/
const REPO_DATA = fileURLToPath(new URL("../../../data/", import.meta.url)); // <repo>/data/ (may not exist)

function cssFilesUnder(dir) {
  if (!existsSync(dir)) return [];
  const out = [];
  for (const e of readdirSync(dir, { withFileTypes: true })) {
    if (e.name === "node_modules" || e.name.startsWith(".")) continue;
    const full = join(dir, e.name);
    if (e.isDirectory()) out.push(...cssFilesUnder(full));
    else if (e.name.toLowerCase().endsWith(".css")) out.push(full);
  }
  return out;
}

const LAYOUT_PROPS = new Set([
  "position", "float", "width", "max-width", "min-width", "height", "margin", "margin-top", "margin-right", "margin-bottom",
  "margin-left", "display", "columns", "column-count", "column-width", "transform", "background-image", "box-shadow",
]);

/** [{selector, prop}] for every declaration in a mapped stylesheet (@font-face / @keyframes blocks are passed through untouched, skipped). */
function declarationsOf(mapped) {
  const flat = mapped.replace(/@(?:font-face|(?:-webkit-)?keyframes)[^{]*\{(?:[^{}]|\{[^{}]*\})*\}/g, "");
  const out = [];
  for (const m of flat.matchAll(/([^{}]+)\{([^{}]*)\}/g)) {
    if (m[1].trim().startsWith("@")) continue;
    for (const piece of m[2].split(";")) {
      const name = /^\s*([a-z-]+)\s*:/i.exec(piece)?.[1]?.toLowerCase();
      if (name) out.push({ selector: m[1], prop: name });
    }
  }
  return out;
}

function assertNoLayout(mapped, label) {
  assert.equal(typeof mapped, "string", label);
  for (const { selector, prop } of declarationsOf(mapped)) {
    if (!LAYOUT_PROPS.has(prop)) continue;
    // the one deliberate exception: a quote line takes its left margin
    assert.ok(prop === "margin-left" && /fw-quote-line/.test(selector) && !/\.fw-h|\.fw-list-line|\.fw-code/.test(selector), `${label}: ${prop} in ${selector.trim()}`);
  }
}

test("bundled CSS files all map without throwing and without layout properties", () => {
  const files = [...cssFilesUnder(REPO_SRC), ...cssFilesUnder(REPO_DATA)];
  const themes = files.filter((f) => /[\\/]themes[\\/]/.test(f));
  assert.ok(themes.length >= 1, "the bundled publishing theme is found: " + files.join(", "));
  for (const f of files) {
    const css = readFileSync(f, "utf8");
    let mapped;
    assert.doesNotThrow(() => { mapped = mapThemeCss(css); }, f);
    assert.equal(typeof mapped, "string", f);
    assertNoLayout(mapped, f);
    assert.equal(mapThemeCss(css), mapped, "pure: " + f);
  }
  for (const f of themes) {
    const mapped = mapThemeCss(readFileSync(f, "utf8"));
    assert.ok(mapped.length > 0, "a real theme maps to something: " + f);
    for (const sel of ["fw-h1", "fw-h6", "fw-quote-line", "fw-list-line.fw-list-ul", "fw-list-line.fw-list-ol", "fw-code", "fw-live-link", "fw-strong"]) {
      // not every theme styles every element, but every mapped selector must be scoped to Live
      if (mapped.includes(sel)) assert.ok(mapped.includes(S + " "), sel);
    }
    // every mapped head is scoped
    const flat = mapped.replace(/@(?:font-face|(?:-webkit-)?keyframes)[^{]*\{(?:[^{}]|\{[^{}]*\})*\}/g, "");
    for (const m of flat.matchAll(/([^{}]+)\{[^{}]*\}/g)) {
      if (m[1].trim().startsWith("@")) continue;
      for (const sel of m[1].split(/,\s*(?=#write)/)) assert.ok(sel.trim().startsWith(S), `${f}: ${sel.trim()}`);
    }
  }
});

test("truncated and corrupted copies of the bundled CSS never throw", () => {
  const files = [...cssFilesUnder(REPO_SRC), ...cssFilesUnder(REPO_DATA)].filter((f) => /[\\/]themes[\\/]/.test(f));
  for (const f of files) {
    const css = readFileSync(f, "utf8");
    for (let cut = 0; cut <= css.length; cut += 5) {
      let out;
      assert.doesNotThrow(() => { out = mapThemeCss(css.slice(0, cut)); }, `${f} cut at ${cut}`);
      assert.equal(typeof out, "string");
    }
    // brace / paren / quote damage in a few places
    for (const [from, to] of [["{", ""], ["}", ""], ["(", ""], [")", ""], [";", ""], [":", ""], ['"', ""], ["/*", ""], ["*/", ""], ["#write", "{"]]) {
      assert.doesNotThrow(() => mapThemeCss(css.split(from).join(to)), `${f} without ${JSON.stringify(from)}`);
    }
    assert.doesNotThrow(() => mapThemeCss(css + css + "}}}{{{"));
  }
});

test("typical Typora theme idioms map without layout leaking", () => {
  const themes = [
    // github-ish
    `html { font-size: 16px } body { font-family: Helvetica; line-height: 1.6 } #write { max-width: 860px; margin: 0 auto; padding: 30px; padding-bottom: 100px }
     #write > ul:first-child, #write > ol:first-child { margin-top: 30px } a { color: #4183C4 } h1, h2, h3, h4, h5, h6 { position: relative; margin-top: 1rem; margin-bottom: 1rem; font-weight: bold; line-height: 1.4; cursor: text }
     h1 { font-size: 2.25em; line-height: 1.2; border-bottom: 1px solid #eee } h2 { font-size: 1.75em; line-height: 1.225; border-bottom: 1px solid #eee }
     h1:hover a.anchor { text-decoration: none } blockquote { border-left: 4px solid #dfe2e5; padding: 0 15px; color: #777 } blockquote blockquote { padding-right: 0 }
     ul, ol { padding-left: 30px } li > ol, li > ul { margin: 0 0 } code, tt { margin: 0 2px; padding: 0 5px; white-space: nowrap; border: 1px solid #eaeaea; background-color: #f8f8f8; border-radius: 3px }
     pre code { white-space: pre; border: 0 } hr { height: 4px; margin: 1rem 0; border: 0 none; background: #e7e7e7 } table { padding: 0; word-break: initial } img { max-width: 100% }
     @media only screen and (min-width: 1400px) { #write { max-width: 1024px } } @media print { html { font-size: 13px } body { margin: 0 } table, pre { page-break-inside: avoid } }`,
    // newsprint-ish: columns, floats, absolute sizes, pseudo-element furniture
    `#write { font-size: 20px; column-count: 2; column-gap: 40px; width: 80vw; margin: 0 auto; background: #fff url(paper.jpg) repeat; box-shadow: 0 0 10px #000; transform: rotate(0.2deg) }
     #write h1 { font-size: 48pt; text-align: center; float: none; position: absolute; border-bottom: 3px double #000 } #write h2 { font-size: 30px; text-transform: uppercase }
     #write p:first-of-type::first-letter { font-size: 3em; float: left } #write blockquote { margin-left: 3em; padding: 1em 2em; font-style: italic; background: #eee; border-left: 5px solid #333 }
     #write blockquote::before { content: "\\201C"; position: absolute; font-size: 4em } #write ul li::before { content: "\\2022"; position: absolute; left: -1em }
     #write li { font-size: 18px; list-style: none; display: block } #write ol li { font-size: 17px } #write code { font-family: Courier; font-size: 16px; background: #ddd }
     #write a:hover { background: yellow } @font-face { font-family: Playfair; src: url(a.woff2) } @page { margin: 2cm } @keyframes spin { to { transform: rotate(360deg) } }`,
    // CSS custom properties, nesting, layers, unusual syntax
    `:root { --a: 1px } @layer base { #write h1 { color: red } } #write { --x: { a: b } } #write h1 { color: var(--a, red); font-size: calc(2 * var(--s)) }
     #write h2 { & em { color: red } color: blue } @container (min-width: 10px) { h3 { color: red } } @media (min-width: 1px) { @media (max-width: 9px) { h4 { color: red } } }
     #write h5 { font: italic bold 12px/30px Georgia, serif } #write h6 { font-size: 0 } #write li { padding-left: 1em } #write p { font-size: .8E1px }
     \\ #write\\ h1 { color: red } #write h1 { color: red;;; } ;; } {} h1 {`,
  ];
  for (const css of themes) {
    let mapped;
    assert.doesNotThrow(() => { mapped = mapThemeCss(css); });
    assert.ok(mapped.length > 0);
    assertNoLayout(mapped, "idiom theme");
  }
  // the newsprint-ish theme: sizes, quote spacing and list kinds come out as designed
  const news = mapThemeCss(themes[1]);
  assert.ok(news.includes(`${S} .cm-line.fw-h1 { font-size: 3.2em;`), "48pt = 64px over a 20px base");
  assert.ok(news.includes(`${S} .cm-line.fw-h2 { font-size: 1.5em; text-transform: uppercase; }`));
  assert.ok(news.includes(`${S} .cm-line.fw-list-line { font-size: 0.9em; }`), "bare li: both kinds");
  assert.ok(news.includes(`${S} .cm-line.fw-list-line.fw-list-ol { font-size: 0.85em; }`));
  assert.ok(news.includes(`${S} .cm-line.fw-quote-line { margin-left: 3em; font-style: italic; background-color: #eee; border-left: 5px solid #333; }`), news);
  assert.ok(news.includes(`${S} .cm-line.fw-quote-line.fw-quote-d1 { padding-left: 2em !important; }`));
  assert.ok(news.includes(`${S} .fw-code { font-family: Courier; font-size: 0.8em; background-color: #ddd; }`));
  assert.ok(!/::|content|first-letter/.test(news.replace(/@font-face[^}]*}/, "")), "furniture is never mapped");
});
