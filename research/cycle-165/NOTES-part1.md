# Cycle 165 part 1 — qpdf layer and Mac packaging (notes for part 2)

## API (`src/pdfeditor.h`; the only app file including qpdf headers, with `pdfeditor.cpp`)
```cpp
namespace PdfEditor {
struct PageRef { int source; int rotation; };   // source: 0-based INPUT page, -1 = inserted; rotation: clockwise change applied (0/90/180/270)
struct Result { bool ok = false; QString error; QByteArray bytes; QVector<PageRef> pages; };  // pages: one per OUTPUT page
enum class Refusal { None, Damaged, NeedsPassword, Signed };
Refusal check(const QByteArray &pdf, QString *message = nullptr);
Result rotate(const QByteArray &pdf, const QVector<int> &pages, int degreesClockwise, bool allowSigned = false);
Result remove(const QByteArray &pdf, const QVector<int> &pages, bool allowSigned = false);
Result move(const QByteArray &pdf, const QVector<int> &pages, int before, bool allowSigned = false);
Result insert(const QByteArray &pdf, const QByteArray &other, int before, bool allowSigned = false);
Result extract(const QByteArray &pdf, const QVector<int> &pages);
}
```
## Decisions
- Selections are sets (order/repeats ignored; `extract` keeps given order and repeats). Empty selection or index out of range = `ok=false`, "Page N does not exist…"; never a silent no-op.
- `rotate`: ±90, ±270, 180 (anything else errors); relative to current /Rotate, inherited values included. `pages` lists every page; only selected ones have rotation != 0.
- `remove`: refuses to remove every page. `move`: `before` 0..pageCount; a move that changes nothing returns ok, the INPUT bytes untouched and an identity `pages`. `insert` accepts `before` 0..pageCount and a signed `other`; `other` damaged/password-protected = error "The PDF to insert cannot be used…".
- `check`: Damaged = qpdf throws, empty input, or no pages. qpdf-recovered files (warnings only) are treated as editable. NeedsPassword = needs a user password, OR opens with the empty password but `allowModifyAssembly()` is false (owner password needed). An encrypted PDF with an empty user password that allows changes is EDITABLE and the output stays encrypted (edit functions); `extract` writes an unencrypted copy. Signed = a /Sig field with /V (searched through /Kids), /AcroForm /SigFlags bit 1, or /Perms (certified). An empty signature field is not Signed.
- Edit functions refuse Damaged/NeedsPassword always, Signed unless `allowSigned`. `extract` needs no flag (the source is not modified; the copy has no AcroForm, metadata or outlines).
- Output is plain `QPDFWriter` to memory (object streams/stream data preserved, encryption preserved, no linearization). Metadata, outlines, links, annotations and page boxes are left as qpdf keeps them (tested).
- Pages removed by `remove`/`move` stay in the file if an outline or link still points at them (size only; Qt PDF count is right).

## qpdf and the build
The project is qmake, not CMake: `fomawrite.pro` and `tests/tests.pro` use `link_pkgconfig` + `PKGCONFIG += libqpdf`, or `QPDF_PREFIX=<prefix with include/ lib/>` (env or qmake variable) to link directly. Homebrew's .pc adds `-std=c++20` (harmless). CI: new step `brew install qpdf pkgconf` before the build.
Windows: vcpkg `qpdf` (installs lib/pkgconfig; set `PKG_CONFIG_PATH`, or pass `QPDF_PREFIX=<vcpkg installed/<triplet>>`; link name `qpdf`, import lib `qpdf.lib`). Deploy qpdf.dll plus its vcpkg dependencies (jpeg, zlib, and an OpenSSL or the native crypto per the port's features; check `dumpbin /dependents`). `bin/check-licenses` already requires `licenses/qpdf-Apache-2.0.txt` when a `qpdf*.dll` is in the folder: the Windows packager must copy `licenses/qpdf-Apache-2.0.txt` into the folder's `licenses/` and add the qpdf, libjpeg-turbo and OpenSSL notices. pdfeditor.cpp is pure memory, so it needs no file or path handling.

## Mac bundle (`bin/package-mac` ran, exit 0; `dist/Fomawrite.app` 524 MB, signed ad hoc, verify OK)
- Added: `libqpdf.30.dylib` (Apache-2.0), `libcrypto.4.dylib` from openssl@4 4.1.0 (Apache-2.0; the bundle already had `libcrypto.3` for Qt). `libjpeg.8` (jpeg-turbo) was already bundled and qpdf resolves to it. No script change was needed for install names/rpaths: macdeployqt found libqpdf and the existing fix-up loop rewrote it to `@rpath`.
- Licences: `tidy-mac-bundle.py` gathers `qpdf-12.4.2_1-LICENSE.txt`, `-NOTICE.md`, `openssl@4-4.1.0-LICENSE.txt` from the kegs unchanged (no committed text needed). New `licenses/qpdf-Apache-2.0.txt` (NOTICE + LICENSE) is copied by `package-mac`; `THIRD-PARTY-NOTICES.txt` has a "PDF page editing" section.
- `bin/check-licenses dist/Fomawrite.app`: passes; with `qpdf-Apache-2.0.txt` removed from a scratch copy: "Licence check failed: qpdf is bundled but licenses/qpdf-Apache-2.0.txt is missing", exit 1.

## Tests (`tests/cycle165-pdf-editor.inc`; all 9 `pdfEditor*` slots) — 9/9 pass; existing 61 pdf* slots pass (63 incl. init/cleanup)
pdfEditorChecksWhetherADocumentCanBeEdited, …RotatesOnePageSeveralPagesAndExistingRotation, …RotatesAnInheritedRotation, …RemovesPages, …MovesPages, …InsertsPagesAtStartMiddleAndEnd, …ExtractsPagesIntoANewDocument, …RefusesWhatItCannotEditAndRejectsBadArguments, …KeepsMetadataOutlineAndLinks. Verified through QPdfDocument (count, page text order, pagePointSize orientation) and the `pages` map; effective /Rotate (180 cannot show in size) is read with qpdf in the test. New fixtures: `tests/fixtures/emptypassword.pdf`, `restricted.pdf` (qpdf 256-bit, from password.pdf's content; `--modify=none` for the latter).
Red/green (mutate `pdfeditor.cpp`, run, restore): remove-all guard disabled -> pdfEditorRemovesPages FAIL (`!r.ok`); `rotatePage(turn,false)` -> rotation test FAIL (index 1); `isSigned` returns false -> check test FAIL; restored -> all pass.

## Known gaps
No password-supplying variant (a locked PDF is refused). No test of an encrypted PDF extract. Tests include qpdf headers (to read /Rotate). Windows build, UI, pending-edit model, saving and notes remapping are not done.
