# Cycle 32 verification

- Build passed; 56 tests passed, zero failures.
- Automated: atomic session round-trip, malformed JSON rejection, excessive write preserving prior state; real local socket ownership/forwarding with Unicode and spaces, activation-only forwarding.
- Native: second executable launch forwarded Second.md and exited successfully; repeat launch of First.md selected its existing tab without a third tab. Native merge produced Second/First order. Quit/relaunch restored that order, selected tab and cursor position 2 in Second.md.
- Native dirty First.md: Quit prompt, Cancel retained the edit, Undo restored original text, Save in Quit completed clean exit. Final rebuilt app restored both tabs. Last active First.md remained recorded while app was inactive.
- Screenshot `restored-tabs.png` reviewed: synthetic sample writing only. Prior-cycle synthetic sample filenames in library; no private writing.
- Normal bounds/library restoration implemented; no claim of fullscreen/minimized/external-display fidelity. No injected crash/disk-full test in this cycle. Native Save As during Quit not separately exercised. Sequential Quit still closes earlier windows before later cancellation.
- Stable Dev and ordinary bundles use the same source; generated bundles stay outside Git.

See docs/remaining-cycles.md for ten subsequent planned cycles and the new themes scope.
