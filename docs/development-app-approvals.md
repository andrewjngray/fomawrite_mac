# Development app approvals and notifications

Use a single QA bundle for future cycles:

```sh
./bin/build
./bin/prepare-dev-app
open -a "$PWD/dist/Fomawrite Dev.app" "$PWD/examples/Preview-check.md"
```

Stable path: `dist/Fomawrite Dev.app`. Stable bundle identifier: `io.github.andrewjngray.fomawrite.dev`. This is separate from the ordinary packaged Fomawrite app. Do not create new per-cycle bundle IDs. The preparation script refuses to replace this app while its executable is running; close it normally after preserving unsaved work, then rebuild. Build outputs remain ignored by Git.

This reduces needless identity changes; it does not guarantee that changed ad-hoc signatures never require renewed approval. If approval is requested, Andrew can choose **Always allow** for **Fomawrite Dev only**. Never approve prompts on his behalf, change the global approval policy, or edit OS privacy databases. The remembered app permission is distinct from sensitive-action, filesystem and shell approvals.

Before a Computer Use call that might request access, explicitly tell Andrew which app is about to be requested. A long attachment wait may be an unseen user approval, not an application failure. Do not repeatedly create fresh QA identities to work around it.

## Notification checks

macOS System Settings → Notifications → ChatGPT was inspected. Notifications, Desktop, Notification Center, Lock Screen, badges and sound were already on. Changed Alert Style from Temporary to **Persistent** and verified the selection. There was no separate Codex entry in the list.

The global setting for notifications while mirroring/sharing the display was Notifications Off. It was left unchanged because changing it exposes notifications from other apps too.

Andrew must check the in-app Settings → Notifications controls for **permission** and **question** notifications. Computer Use explicitly refuses to operate Codex itself, so these internal settings were not changed or verified. In-app Settings → Computer Use lists always-allowed apps; permission must be granted by Andrew through that UI or the app-access prompt.

Sources: https://learn.chatgpt.com/docs/notifications and https://learn.chatgpt.com/docs/computer-use .

## Verification and old windows

`sh -n bin/prepare-dev-app`, bundle preparation and strict/deep signature verification passed. The new bundle was prepared without launching another window or requesting new app access. Two inspected clean QA windows (Cycle7b and Cycle7c) were closed. Cycle7-final had recovered unsaved content and was left open. Other unsaved windows were not discarded. Persistence of Always allow across a subsequent rebuilt binary remains to be checked after Andrew grants it.

## Which app should Andrew use?

- **Fomawrite.app** is the packaged build for everyday writing. It bundles Qt dependencies and is refreshed by `bin/package-mac`.
- **Fomawrite Dev.app** is our workshop/test build. It uses the local Qt installation and one stable macOS identity so repeated UI checks cause fewer new access prompts.

Both come from the same source code; Dev is not a separate feature edition, nor does it make disposable copies of documents. Both edit the real Markdown file that you open. They can be at different revisions until both bundles are refreshed; an already-running process also keeps its old code until reopened. Prefer Fomawrite for daily writing and Dev for cycle reviews. Avoid editing the same document in both simultaneously. Closing either should preserve unsaved-change prompts. The Dev bundle must not be replaced while running.
