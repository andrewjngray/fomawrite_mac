// Bundles src/main.ts -> dist/editor.js (IIFE, minified, no runtime deps, chrome120).
//   node esbuild.mjs            one-shot production build
//   node esbuild.mjs --serve    dev: unminified + sourcemap, rebuild on change, serve on :8000
import * as esbuild from "esbuild";

const serve = process.argv.includes("--serve");

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
