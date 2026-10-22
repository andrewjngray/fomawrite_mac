# Windows desktop checkpoint — 0e9ef4a

22 October 2026. Exact application commit `0e9ef4a71d0c43e21440d9efe0c1d1110998d6c5`, ARM64, ordinary Windows 11 startup on Andrew’s Parallels VM. This is a qualification checkpoint, not a release declaration. Current packages remain recorded in [Windows status](STATUS.md).

## Passing evidence

The executable was staged on Windows-local NTFS with all 1,608 manifest files verified. Manifest SHA-256: `701aba657eaf842949a6d81be6507ecad4ea3e29dd006ecb85c252427f05727a`; executable SHA-256: `646af5b141359217a37564d18bc4e5402746b3987bb2db3fef4a28183e91df91`.

| Actual app workflow | Result |
| --- | --- |
| Normal startup, native folder/file dialogs, editing, Save, Live, Source and restart | Six stages passed; both ordinary launches exited normally. Independent strict report verification passed. |
| Live editing and Undo → Save timing | All five maintained cases passed: settled, immediate, 50 ms, 100 ms and 150 ms. Exact Source and disk contents, clean state and ordinary restoration passed. Independent strict series verification reported full acceptance for these five cases. |
| Publishing | Nine stages passed: A/B/A identity in Web and PDF, Modern Sans → Classic Serif, reload and native HTML/PDF exports. All 95 evidence hashes matched; root visually reviewed all eight format captures. |

Publishing’s Parallels wrapper returned 255 despite complete app results and normal exit. This transport failure is retained; the claim is limited to the verified workflow/report and captures. It is not a successful-wrapper claim or all-theme/endurance qualification.

Local reports under `dist/windows-testing/results/` (ignored, retained):

| Report | SHA-256 |
| --- | --- |
| `final-0e9-arm64-normal-39a4b0e13cfd4824a44909dd639bcfe3/normal-launch-report.json` | `a6fe1ae1148494fed850115f05f5692cab97927d0f77e10224ec97decbe1c961` |
| `final-0e9-arm64-live-a02281f791084c05a9d9dc6fb255d5b8/live-edit-series.json` | `31a8fc7d48b606fed56de2b9e9ab2b6d9b347d14c38d6028c7510fc28a7dd186` |
| `final-0e9-arm64-publishing-0e7f0dabd85a414ba5ed492758df7749/publishing-supplement-report.json` | `dc8ebb4de71abd41c653873a1f331048c48f5ae0a9ac33e8fc5c2ed765d78b03` |
| Same publishing directory, `root-publishing-review.json` | `3155c7be86f70d9f3bf4f1661aaebb0563f83bf693a7a5a414cecd8923d13370` |

The maintained r7 driver handles Exit in an overflowing native popup by physical scrolling and stable owned-popup OCR. Earlier failed observations are retained; the correction does not change application behavior or weaken exit/restore checks. [Cycle 154](../../research/cycle-154/README.md).

## Open supplemental failures

- **Selected formatting followed by Save:** the first supplemental run inserted empty bold markers instead of proving a selected word; disk remained old and the title dirty. Neither selection setup nor selection preservation was witnessed. This is a rejected run, not proof that the repaired persistence deadline is still defective. A separate diagnostic must witness the exact selection before and after opening Format. Report: `final-0e9-arm64-selection-save-be07a6f8a8f349f18014a7178afbd573/evidence/selection-save-report.json`, SHA-256 `735138cf85a4fe705c39b7ea457b671b9a24d936afa36f508b08ee8731869042`.
- **Clipboard:** Copy Markdown comparison failed. Review found that the driver incorrectly required Windows UnicodeText to preserve LF, although Qt converts it to CRLF; custom Markdown must still be compared as exact UTF-8. The run did not record enough individual comparisons to qualify the app. Separately, restoration returned `0x800401D4` after partially changing clipboard ownership; preservation is unknown. No successful restoration is claimed. Report: `final-0e9-arm64-clipboard-a4a5f674d99b4408bed33863799d8e32/evidence/clipboard-menu-report.json`, SHA-256 `f1636713418e140fb23cf003715fba154d1a2e59d4e3d6d00532e1b576662699`.
- **Nested View:** the driver timed out reopening Sort Files By after its first Name selection. Attribution remains open; strict verification rejected it. Report: `final-0e9-arm64-view-ed813b0a86d145709c2ea1a005c8532e/evidence/nested-menu-report.json`, SHA-256 `e8e0b9eeab012ed8a5bac6458cd029e474f5c4bb25c4d3f069f9549252555a79`.

Actual input testing uses disposable public fixtures and isolated app profiles. These open supplements are distinct from the passing core workflow cases. No physical x64-machine test, arbitrary theme coverage, typed emoji/IME coverage, installer qualification or complete desktop parity is claimed.
