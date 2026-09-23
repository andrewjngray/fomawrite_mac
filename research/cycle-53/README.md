# Cycle 53: Go menu and bounded hashtag refresh (native QA pending)

Synthetic fixtures live under `sample/`. The captured Go menu is mapped in `docs/ia-menu-audit-2026-09-23.md`; this source checkpoint adds menu access to existing library navigation, recent files, saved queries and hashtags without changing Markdown storage.

Go → Smart Folders lists saved queries and opens a blank Quick Search for New Smart Folder. A query is persisted only on Save. Unavailable roots and missing recent files cannot silently redirect the library. The present Recents submenu does not yet match iA's direct Recents action.

Hashtag scanning remains bounded by the existing file, byte and tag limits. A complete small-root scan attempts to watch up to 256 folders/files and rescans after a 400 ms debounce. Failed registration, inaccessible/skipped files and larger roots show an explicit manual-refresh status. Root changes cancel old scans and suppress stale results. Quick Search independently rereads bounded saved content. This is not a recursive incremental content index or full iA smart-folder implementation.

`./bin/build` passes and the full `./bin/test` suite passes with **88 tests, zero failures, zero skips** using native macOS access. In the restricted sandbox, older macOS integration checks failed; the focused live watcher check also could not register paths, so that run is not the acceptance result. Native Dev menu/visual QA is pending because the Mac is locked and the running Dev app cannot yet be closed normally. No private writing was used.

The ordinary `dist/Omawrite.app` was packaged from this source and passed strict local-signature verification. After unlock, close Dev normally, prepare the stable Dev bundle and check Go menu grouping, saved-query roundtrip, external tag edit/atomic replacement and unavailable-root fallback using this sample. Record screenshots and results here.
