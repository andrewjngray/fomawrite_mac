# Cycle 158 — Licence notices and no GPL-only Qt libraries

Build 158 (`0.3.0-dev54`), 18 November 2026. Andrew asked whether Fomawrite is open source and what its licensing status is, then asked for the gaps found to be fixed on the Mac and Windows versions.

## Findings
- Fomawrite is public on GitHub under the MIT licence it inherited from Omawrite (copyright David Heinemeier Hansson). LICENSE named only DHH.
- The Mac app shipped notices for Omawrite, the fonts, MD4C and Harper only: none for Qt (LGPL v3), Chromium (in Qt WebEngine), PDFium (in Qt PDF), the Live editor's JavaScript packages (CodeMirror, Lezer, KaTeX: MIT, and esbuild strips their comments) or the Homebrew libraries Qt brings (ICU, OpenSSL, glib, FreeType, HarfBuzz, image codecs and others).
- macdeployqt bundled about 25 Qt libraries Fomawrite never imports, among them Qt Virtual Keyboard and its Hunspell input method (GPL-3.0-only or commercial) and Qt Quick Timeline, plus Qt 3D, Multimedia, Particles, SQL/Local Storage and State Machine.
- The Windows notice said Chromium's credits are at `chrome://credits`; Qt WebEngine refuses that URL (`net::ERR_INVALID_URL`, checked with Qt 6.11.2).

## Changes
- `LICENSE` adds "Copyright (c) 2026 Andrew Gray".
- `bin/tidy-mac-bundle.py` (run by `bin/package-mac` after macdeployqt) removes the unused QML modules and their PlugIns/quick libraries and plug-in folders, then every framework or dylib nothing left loads (25 removed), then copies each remaining Homebrew library's licence files into `licenses/homebrew/` (32 files). glib's keg has no licence file, so `licenses/glib-2.88.3-COPYING.txt` (LGPL 2.1, from glib's repository at that tag) is used, and a newer glib fails packaging until its text is added.
- `licenses/` holds the shared notices: `THIRD-PARTY-NOTICES.txt`, the Qt LGPL 3.0 and GPL 3.0 texts (SHA-256 identical to the GNU originals), Chromium's BSD licence (Homebrew Qt WebEngine's `LICENSE.Chromium`) and PDFium's (from pdfium.googlesource.com). Packaging adds a macOS section listing the Qt version and every bundled library.
- `src/editor/esbuild.mjs` writes `dist/THIRD-PARTY-NOTICES.txt` from the build's metafile: the 45 packages esbuild put into `editor.js`, plus KaTeX for its CSS and fonts, each with version and full licence text. CI's committed-asset check keeps it current.
- `bin/check-licenses <bundle>` fails a Mac app or Windows folder that lacks a required notice, lacks Andrew Gray's copyright line, or contains a GPL-only Qt module; `bin/package-mac` runs it.

## Verification
- `bin/check-licenses` fails on the build 157 bundle (7 missing notices; Virtual Keyboard, Hunspell input method and Timeline bundled) and passes on build 158.
- The packaged app's own self-check (`--check-document-views`, run headless from a scratch copy with Qt's offscreen platform plug-in added) gives the same result as build 157: all editor-acceptance (7), daily-writing (5), zoom (5), pane-chrome (3) and editing-layout (5) groups pass, no QML warnings; the same five publishing groups that need hover or Web/PDF switching fail headless as before.
- Editor tests 327/0 and typecheck pass; `editor.js` is unchanged.
- The bundle is 515 MB.

This is a reading of the licences, not legal advice. Qt WebEngine's Chromium third-party components are listed by reference to Qt's licensing page, not reproduced.
