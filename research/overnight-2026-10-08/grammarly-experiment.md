# Grammarly experiment — ten minutes at the keyboard (for Andrew)

Purpose: decide the spell-check entry in IDEAS.md with evidence instead of guesses. Nothing to build first; build 136 is installed.

1. Install **Grammarly Desktop for Mac** (the overlay app, not the browser extension) and sign in with your account.
2. Open `/Applications/Fomawrite.app`, open any document, and type a sentence with two obvious errors ("Their going too the shop tomorow.") in **Source** mode. Note: does the Grammarly indicator appear, does it underline, can you accept a suggestion?
3. Switch to **Live** and type the same sentence. Note the same three things. Live is a Chromium page inside the app, so this is the case most likely to behave differently.
4. Try once in the **link panel** field in Live and once in the **Find** field, to see whether the overlay attaches to small fields.
5. Quit Grammarly Desktop and note whether the editor feels any different (the "little bug chasing around" problem you described).

What the answers decide:
- Works in both surfaces: the spell-check entry becomes "rely on Grammarly Desktop; add a View toggle that hides nothing of ours but documents how to pause Grammarly" (we cannot turn their overlay off from inside the app; a per-app disable exists in Grammarly's own settings).
- Works in Source only: Live needs its own spell check (bundled dictionaries) or Grammarly stays a Source-mode habit.
- Works in neither: native spell check for Source plus dictionaries for Live, Grammarly stays in Chrome.

Screenshots of each step would be useful; drop them in `research/overnight-2026-10-08/` and I will fold the result into IDEAS.md.
