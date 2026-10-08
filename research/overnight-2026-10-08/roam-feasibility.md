# Roam "send to daily note" — feasibility (written overnight, 8 October 2026; nothing built)

Status: IDEAS.md lists Roam as Maybe, so nothing was built. This note is what a Yes would need.

## What exists on this machine

- The Roam connector used in Claude sessions is a claude.ai connector (two copies in the bundle). It is not a local CLI: `roam-mcp`/`roam` are not on the PATH and there is no local config. The graph does contain a page "Local API Token: roam-mcp CLI (MacBook-Pro-14M5.local)", which suggests a token was issued for a CLI at some point; it is not installed now.
- So a Qt-side feature cannot borrow the connector. It needs its own path to Roam.

## Two ways Fomawrite could reach Roam

1. **Roam backend API directly** (`https://api.roamresearch.com`, graph token). The app would hold an API token (macOS Keychain via the existing native bridge), call `create-block` on the daily-note page uid (`MM-DD-YYYY`), and show the result. Pros: no extra install, works offline-queued. Cons: a credential in the app, and the token has write scope to the whole graph, so the UI must be deliberately small (one action, one target).
2. **A local helper process** (a tiny Node/Python script with the token in its own config) that the app shells out to, the way `bin/` scripts work. Pros: the app never sees the token. Cons: another thing to install and keep running.

Recommendation if Yes: option 1 with the token in Keychain, a single menu action **Share → Send to Roam** that sends the current paragraph (or selection) to today's daily note as one block, prefixed with an optional `[[page]]` the writer types once per document (stored in front matter `roam: [[Page]]`), and suffixed with a back-link `fomawrite://open?path=…&pos=…`. Nothing is read back; no sync.

## What it would cost

- Qt: a `RoamClient` (QNetworkAccessManager, ~150 lines), Keychain read/write in `macbridge`, one command, one dialog for the token, tests with a local fake HTTP server. About a day with review.
- Page: nothing; the selection comes from the bridge as today.

## Decision needed from Andrew

- Yes/No on building it at all.
- If Yes: is the daily note the only target, or also a named page per document?
- Whether a back-link into the file is wanted on the Roam side.
