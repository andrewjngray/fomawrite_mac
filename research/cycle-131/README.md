# Cycle131 — byte-exact persistence

Build **0.3.0-dev27 / macOS 0.3.0 (131)**. Source/test changes only; no packaged bundles were refreshed in this cycle (see Limits).

Addresses the data-integrity findings C1–C6 of the [independent review](../../INDEPENDENT_REVIEW.md) (§2). Before this cycle the editor core was safe, but the bytes going in and out were not: a document could be rewritten on the first Save without being marked modified, and a non-UTF-8 file was destroyed outright.

## What changed

| Finding | Behavior now |
| --- | --- |
| C1 — non-UTF-8 input decoded lossily and written back | `Backend::decodeDocumentBytes` validates UTF-8 with `QStringDecoder` and recognises UTF-16 byte-order marks. The readable text is still shown, but `m_lossyDecode` makes Save, autosave and quit-save **refuse to write over the original**; the status says why and points to Save As, which writes a UTF-8 copy and clears the flag. |
| C2 — `toPlainText()` rewrote NBSP / U+2028 on every save | `currentDocumentText()` now derives from `toRawText()`, replacing only paragraph separators with `\n`. No-break spaces, French guillemet spacing and Unicode line separators survive save, recovery, duplicate and publishing. Offsets are identical to `toPlainText()`, so the Source↔Visual mapping is unchanged. |
| C3 — CR/CRLF: converted on save; lone-CR files collapsed to one line; CRLF files never matched their disk baseline (autosave paused forever, Rename/Move refused) | Files are read as raw bytes (no `QIODevice::Text`). The dominant line ending is recorded (`m_lineEnding`) and re-applied by `encodeDocumentText` on save/duplicate; the disk baseline holds the real bytes, so autosave, rename, move and the external-change watcher compare like for like. Mixed endings are reported in the status and normalised to the dominant style on the next save. |
| C4 — UTF-8 BOM stripped | Remembered (`m_hadByteOrderMark`) and re-emitted. |
| C5 — manual Save never checked the disk; a recovered document overwrote newer external edits | `saveTo` now compares the on-disk bytes with the known baseline for every same-document save, not only autosave. On mismatch it cancels the staged write and raises the existing **File changed** dialog, which gains a **Save Anyway** button (keeps the window's text and saves explicitly). |
| C6 — un-normalised `m_lastDocumentText` baseline | Both sides of the change comparison use the canonical text. |

Recovery snapshots carry the line-ending, byte-order-mark and lossy flags so a relaunch restores the same conventions. `restoreVersion` normalises historical line endings before insertion.

## Verification

- Full offscreen regression suite: **208 passed, 0 failed, 0 skipped** (204 prior + 4 new), with the two known Qt Material `SplitView` warnings unchanged.
- New byte round-trip fixtures in [`tests/cycle131-persistence.inc`](../../tests/cycle131-persistence.inc): LF, CRLF, CR, BOM, BOM+CRLF, NBSP/guillemets, U+2028, no final newline, trailing/hard-break whitespace. Each is opened, saved untouched (bytes must be identical), edited and saved again (bytes must keep the file's own BOM and line-ending style).
- Lossy fixtures: Latin-1 `caf\xE9` and a UTF-16LE file. Save and autosave are refused and reported; the original bytes are untouched; Save As produces a UTF-8 copy on which ordinary saving resumes.
- CRLF workflow: autosave succeeds and writes CRLF; `renameDocument` and `moveDocument` succeed; later edits keep CRLF.
- Manual-save baseline: an external rewrite between open and ⌘S is refused with `externalChangeDetected(false, true)`; Keep Mine followed by Save replaces it explicitly. The existing crash-recovery process fixture (`recoverySnapshotsSurviveProcessExit`) now also asserts that a plain Save after relaunch does not overwrite the external writer's version.
- Red/green: with the source fixes stashed and the new tests kept, all four new tests and the extended recovery fixture **fail** on the Cycle 130 code (CRLF autosave reports "Autosave paused: file changed outside Fomawrite"; the recovered document is silently overwritten); with the fixes restored they pass. These are therefore genuine regression tests, not ones that also pass the old implementation.

## Limits

- Offscreen Qt only. No packaged `dist`/`/Applications` bundle was refreshed in this cycle because the installed editor was in use; run `./bin/package-mac` / `./bin/prepare-dev-app` / `./bin/install` after closing it. No `verified-builds.json` is recorded for 131.
- A document that itself contains U+2029 still becomes a paragraph break (QTextDocument semantics); documented, not fixed.
- Mixed line endings are normalised to the dominant style on save rather than preserved per line.
- UTF-16 files are displayed but not written back as UTF-16; Save As produces UTF-8 by design.
- The mapper's CRLF-preservation branches in `sourcevisualmapping.cpp` remain unreachable from the live app (the document is always LF); they are left in place for Cycle133's structural pass.
- Physical input, VoiceOver and multi-display acceptance remain open as before.
