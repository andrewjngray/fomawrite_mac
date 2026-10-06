// Bundles src/main.ts -> dist/editor.js (IIFE, minified, no runtime deps, chrome120).
//   node esbuild.mjs            one-shot production build
//   node esbuild.mjs --serve    dev: unminified + sourcemap, rebuild on change, serve on :8000
import * as esbuild from "esbuild";
import { copyFileSync, mkdirSync, readFileSync, readdirSync, writeFileSync } from "node:fs";

const serve = process.argv.includes("--serve");

// KaTeX assets: dist/katex.css + dist/fonts/*.woff2 (committed; the Qt build has no node step and the page is
// served from qrc:/editor/). Only woff2 is kept: Chromium (QtWebEngine) supports it, so the woff/ttf fallbacks
// in katex.min.css are dropped and every url() becomes fonts/<name>.woff2, relative to dist/katex.css.
function copyKatexAssets() {
  const dist = new URL("./node_modules/katex/dist/", import.meta.url).pathname;
  mkdirSync("dist/fonts", { recursive: true });
  const css = readFileSync(dist + "katex.min.css", "utf8").replace(/,url\(fonts\/[^)]+\.(?:woff|ttf)\)\s*format\("[^"]*"\)/g, "");
  writeFileSync("dist/katex.css", css);
  for (const f of readdirSync(dist + "fonts")) if (f.endsWith(".woff2")) copyFileSync(dist + "fonts/" + f, "dist/fonts/" + f);
}
copyKatexAssets();

/** @type {import('esbuild').BuildOptions} */
const options = {
  entryPoints: ["src/main.ts"],
  outfile: "dist/editor.js",
  bundle: true,
  format: "iife",
  target: "chrome120",
  platform: "browser",
  minify: !serve,
  sourcemap: serve ? "linked" : false,
  legalComments: "none",
  banner: { js: "// built from src/ — run npm run build" },
  logLevel: "info",
};

if (serve) {
  const ctx = await esbuild.context(options);
  await ctx.watch();
  const { port } = await ctx.serve({ servedir: ".", port: 8000 });
  console.log(`dev server: http://localhost:${port}/index.html  (add ?mode=source for source mode)`);
} else {
  await esbuild.build(options);
}
