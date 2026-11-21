# Cycle 168 — Mac PDF toolbar

Build 168 (`0.3.0-dev63`).

- Rotate Left and Rotate Right are available from one toolbar button. The current page is the target until pages are selected in the thumbnails. Rotation stays pending and uses the existing Undo and Save workflow.
- The active pen or note button has an accent outline and an instruction in the reader. Escape disarms the tool. A note still opens beside its pin.
- The menu beside Fit Width offers Fit Page. The scale fits the largest page dimensions and stays consistent as you scroll mixed page sizes. The reader remembers the mode per PDF and adapts it to window size. The toolbar overflow includes both Fit Page and rotation when their controls fold.

## Performance

Three fresh native Cocoa test processes per build, interleaved on the same Mac. Each run opened a synthetic 120-page PDF made from a reusable scan-like raster and jumped to every third page, waiting for its render. Time to first page starts with the open request in an exposed app window. These are medians across runs.

| Measurement | 167 | 168 |
| --- | ---: | ---: |
| Open request to first render | 161 ms | 149 ms |
| Median jump to rendered page | 127.8 ms | 127.8 ms |
| 95th-percentile jump to rendered page | 131.7 ms | 133.7 ms |
| Process RSS after scrolling | 509.3 MiB | 510.5 MiB |

Opening ranged from 151–284 ms in 167 and 140–154 ms in 168. The run exercises the existing Fit Width reading path and uses a repeated raster, so it gives a controlled comparison of the toolbar change on one workload. [Local benchmark log](fomawrite-168-bench.log)

## Verification

- The new Rotate, active-tool and Fit Page regressions failed against 167 at the missing controls. The first run on 168 passed all eight QtTest checks, including existing page edit and zoom checks.
- The full Mac suite passed with 426 passes, zero failures and zero skips. A separate Cocoa test verified the toolbar fold order and overflow at 320, 420, 600 and 900 logical pixels.
- The signed Dev app built and passed strict signature verification. The native Cocoa regression run passed all six QtTest checks, including the existing note popover workflow. [Toolbar capture](captures/cycle168-pdf-toolbar.png) · [Armed Note capture](captures/cycle168-pdf-note-armed.png)
- The shipped Dev bundle passed the native document-view harness, including both PDF reader and highlighting checks. The bundle verification matched the embedded QML to the checkout. [Native report](native-bundle/report.json)
- [Hands-on checklist](../usability/cycle-168.md).
