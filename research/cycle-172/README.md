# Cycle 172 — Right-click actions for Preview links

Build 172 (`0.3.0-dev67`).

The PDF publishing preview drew clickable link regions and only handled the primary click. Web Preview accepted and suppressed context-menu requests. Both now use the same compact link menu, created on demand.

- Web links offer **Open in Browser** and **Copy Link Address**. Mail links open the mail app. Internal links navigate to their destination in Preview.
- Local links offer **Open File**, **Copy Path**, **Reveal in Finder** on Mac, and **Save As** for files. The standard Open action follows the existing Markdown/PDF route and opens other files using the system association. On Mac, **Open With** lists registered apps.
- **Copy File Contents** is available for readable UTF-8 text files up to 8 MiB. **Save As** copies the file's bytes and protects the source path.

## Verification

- A pointer click on a real generated-PDF link failed on 171 because no menu opened. Cycle 172 tests exercise actual right-clicks in both Preview modes, an encoded file path, copied local text and web address, Save As activation, copy fidelity and the original source document state.
- The initial targeted Mac suite passed seven checks, including PDF Reader copy and existing Web and PDF publishing previews.
- Both focused Preview gestures passed in native Cocoa. The stable Mac Dev bundle carries build 172 and passed strict signature verification.
- The full Mac regression suite passed 433 tests with no failures or skips. The signed Dev bundle passed the focused publishing checks and the full document-view harness. Earlier harness runs timed out during a Preview mode switch and a PDF Reader page render. Both passed in the final run.
- [Hands-on checklist](../usability/cycle-172.md).

Mac build 172 was promoted from source commit `455c67f`. The ARM64 archive hashes verified. The signed distribution and Applications copies have identical executables, and the installed copy passed the four focused Preview groups with eight settled pane captures.

[Mac CI](https://github.com/andrewjngray/fomawrite_mac/actions/runs/44987281122) passed with 421 Qt checks, zero failures and 12 skips. Both Preview link regressions ran and passed. The editor bundle job also passed.
