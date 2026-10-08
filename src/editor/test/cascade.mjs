// A deliberately tiny CSS cascade for tests (not a test file: the runner globs *.test.mjs).
// Enough to answer "which declaration wins for property P on element E": flat rules (`@media` / `@supports` blocks
// are skipped), compound selectors with tag / #id / .class / :where() / :is() / :not() / state pseudo-classes,
// descendant (` `) and child (`>`) combinators, specificity, !important, document order, inheritance of chosen
// properties, and var() with fallbacks.
//
// NOT modelled, and LOUD about it: the sibling combinators `+` and `~`, attribute selectors, structural
// pseudo-classes (`:nth-child`, `:first-child`, ...), `:has()`. A selector using one of
// them THROWS "cascade: ..." instead of silently never matching, so a test can never pass because a rule was
// quietly ignored. The one deliberate skip is a pseudo-element (`::before`, `::marker`), which cannot style an
// element and so simply does not match.

export function el(tag, { id = "", classes = [], parent = null, vars = {} } = {}) {
  return { tag, id, classes: new Set(classes), parent, vars };
}

function skipBlock(css, i) {
  let depth = 0;
  for (; i < css.length; i++) {
    if (css[i] === "{") depth++;
    else if (css[i] === "}" && --depth === 0) return i + 1;
  }
  return css.length;
}

/** Flat list of `{ selector, decls: [{ prop, value, important }], order }`. */
export function parseRules(css, orderBase = 0) {
  css = css.replace(/\/\*[\s\S]*?\*\//g, "");
  const out = [];
  let i = 0, order = orderBase;
  while (i < css.length) {
    const open = css.indexOf("{", i);
    if (open < 0) break;
    const prelude = css.slice(i, open).trim();
    if (prelude.startsWith("@")) { i = skipBlock(css, open); continue; }
    const close = css.indexOf("}", open);
    const decls = [];
    for (const d of css.slice(open + 1, close).split(";")) {
      const k = d.indexOf(":");
      if (k < 0) continue;
      let value = d.slice(k + 1).trim(), important = false;
      if (/!important\s*$/.test(value)) { important = true; value = value.replace(/\s*!important\s*$/, ""); }
      decls.push({ prop: d.slice(0, k).trim(), value, important });
    }
    for (const selector of splitTop(prelude, ",")) out.push({ selector: selector.trim(), decls, order: order++ });
    i = close + 1;
  }
  return out;
}

function splitTop(s, sep) {
  const parts = [];
  let depth = 0, cur = "";
  for (const ch of s) {
    if (ch === "(") depth++;
    if (ch === ")") depth--;
    if (ch === sep && depth === 0) { parts.push(cur); cur = ""; } else cur += ch;
  }
  parts.push(cur);
  return parts;
}

/** Split a complex selector into compounds with their combinators: [{ comb: "" | " " | ">", text }]. */
function compounds(sel) {
  const parts = [];
  let depth = 0, cur = "", comb = "";
  const flush = () => { if (cur) { parts.push({ comb: parts.length ? comb || " " : "", text: cur }); cur = ""; comb = ""; } };
  for (const ch of sel.trim()) {
    if (ch === "(" || ch === "[") depth++;
    if (ch === ")" || ch === "]") depth--;
    if (depth === 0 && (ch === "+" || ch === "~")) throw new Error("cascade: sibling combinator not modelled: " + sel);
    if (depth === 0 && ch === ">") { flush(); if (!parts.length || comb === ">") throw new Error("cascade: bad child combinator: " + sel); comb = ">"; }
    else if (/\s/.test(ch) && depth === 0) flush();
    else cur += ch;
  }
  flush();
  if (comb) throw new Error("cascade: trailing combinator: " + sel);
  return parts;
}

/** Parse one compound into simple selectors. */
function simples(comp) {
  const out = [];
  const re = /(#[\w-]+)|(\.[\w-]+)|(:(where|is|not)\()|(::?[\w-]+)|([\w-]+|\*)/y;
  let i = 0;
  while (i < comp.length) {
    re.lastIndex = i;
    const m = re.exec(comp);
    if (!m) throw new Error("cascade: cannot parse " + comp);
    if (m[1]) out.push({ k: "id", v: m[1].slice(1) });
    else if (m[2]) out.push({ k: "class", v: m[2].slice(1) });
    else if (m[3]) {
      let depth = 1, j = re.lastIndex;
      for (; j < comp.length && depth; j++) { if (comp[j] === "(") depth++; if (comp[j] === ")") depth--; }
      out.push({ k: m[4], v: splitTop(comp.slice(re.lastIndex, j - 1), ",").map((s) => s.trim()) });
      i = j;
      continue;
    } else if (m[5]) {
      const name = m[5].replace(/^:+/, "");
      if (m[5].startsWith("::")) out.push({ k: "element", v: name });
      else if (/^(hover|focus|active|focus-visible|focus-within)$/.test(name)) out.push({ k: "state", v: name });
      else throw new Error("cascade: pseudo-class not modelled: :" + name + " in " + comp);
    } else out.push({ k: "tag", v: m[6] });
    i = re.lastIndex;
  }
  if (comp.includes("[")) throw new Error("cascade: attribute selectors not modelled: " + comp);
  return out;
}

const cmp = (x, y) => x[0] - y[0] || x[1] - y[1] || x[2] - y[2];
const maxSpec = (list) => list.map(specificity).sort(cmp).pop() ?? [0, 0, 0];
export function specificity(selector) {
  let a = 0, b = 0, c = 0;
  for (const { text } of compounds(selector)) {
    for (const s of simples(text)) {
      let add = null;
      if (s.k === "id") a++;
      else if (s.k === "class" || s.k === "state") b++;
      else if (s.k === "tag" && s.v !== "*") c++;
      else if (s.k === "element") c++;
      else if (s.k === "is" || s.k === "not") add = maxSpec(s.v);
      // :where() adds nothing
      if (add) { a += add[0]; b += add[1]; c += add[2]; }
    }
  }
  return [a, b, c];
}

export const higher = (a, b) => cmp(specificity(a), specificity(b)) > 0;

function matchCompound(comp, e, state) {
  return simples(comp).every((s) => {
    switch (s.k) {
      case "id": return e.id === s.v;
      case "class": return e.classes.has(s.v);
      case "tag": return s.v === "*" || e.tag === s.v;
      case "state": return state.has(s.v);
      case "element": return false; // a pseudo-element never matches an element
      case "where": case "is": return s.v.some((x) => matches(x, e, state));
      case "not": return !s.v.some((x) => matches(x, e, state));
    }
    return false;
  });
}

/** Does `e` match `cs[i]` with the combinators to its left holding (backtracking over descendant steps)? */
function matchFrom(cs, i, e, state) {
  if (!matchCompound(cs[i].text, e, state)) return false;
  if (i === 0) return true;
  if (cs[i].comb === ">") return !!e.parent && matchFrom(cs, i - 1, e.parent, state);
  for (let a = e.parent; a; a = a.parent) if (matchFrom(cs, i - 1, a, state)) return true;
  return false;
}

export function matches(selector, e, state = new Set()) {
  const cs = compounds(selector);
  return matchFrom(cs, cs.length - 1, e, state);
}

function resolveVars(value, e) {
  let guard = 0;
  while (value.includes("var(") && guard++ < 20) {
    const at = value.lastIndexOf("var(");
    let depth = 1, j = at + 4;
    for (; j < value.length && depth; j++) { if (value[j] === "(") depth++; if (value[j] === ")") depth--; }
    const [name, ...rest] = splitTop(value.slice(at + 4, j - 1), ",");
    let found, node = e;
    for (; node && found === undefined; node = node.parent) found = node.vars[name.trim()];
    value = value.slice(0, at) + (found ?? rest.join(",").trim()) + value.slice(j);
  }
  return value.trim();
}

/**
 * Computed value of `prop` on `e`. `inherited` properties fall back to the parent's value; others to undefined.
 * `rules` is the concatenation, in document order, of every stylesheet.
 */
export function computed(prop, e, rules, { state = new Set(), inherited = true } = {}) {
  let best = null;
  for (const r of rules) {
    // A rule that does not declare `prop` is irrelevant, so a selector this engine cannot model is skipped there.
    // One that DOES declare it must be understood: the error propagates instead of the rule silently not matching.
    let hit = false;
    try { hit = matches(r.selector, e, state); } catch (err) {
      if (r.decls.some((d) => d.prop === prop)) throw err;
    }
    if (!hit) continue;
    for (const d of r.decls) {
      if (d.prop !== prop) continue;
      const cand = { important: d.important, spec: specificity(r.selector), order: r.order, value: d.value };
      const wins = !best || (cand.important !== best.important ? cand.important : (cmp(cand.spec, best.spec) || cand.order - best.order) >= 0);
      if (wins) best = cand;
    }
  }
  if (best && best.value !== "inherit") return resolveVars(best.value, e);
  if (best && e.parent) return computed(prop, e.parent, rules, { state, inherited });
  return inherited && e.parent ? computed(prop, e.parent, rules, { state, inherited }) : undefined;
}
