#!/usr/bin/env python3
"""Run native Windows acceptance against a deployed portable directory.

Only the compiled --check-document-views harness is launched. It owns isolated
settings/recovery/IPC and disposable writing; normal startup is a separate gate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(report, manifest, executable):
    failures = []
    if not report.get("passed"):
        failures.append("The executable reported failed view checks")
    for name, minimum in {"dailyWritingChecks": 5, "paneZoomChecks": 5,
                          "editorAcceptanceChecks": 7, "paneChromeChecks": 3,
                          "editingLayoutChecks": 2, "publishingThemeChecks": 12}.items():
        checks = report.get(name, [])
        if len(checks) < minimum or any(not check.get("passed") for check in checks):
            failures.append(f"Missing or failed {name} (minimum {minimum})")
    live = {"live-window-typing-source-switch-save-and-reopen", "live-window-keyboard-undo-and-redo"}
    if not live.issubset({c.get("name") for c in report.get("editorAcceptanceChecks", [])}):
        failures.append("Live keyboard input, persistence and history were not all exercised")
    observations = report.get("publishingPreviewCorrectness", {}).get("observations", [])
    painted = [entry for entry in observations if "actualPanePainted" in entry]
    if len(painted) < 8 or any(not entry.get("actualPanePainted") or not entry.get("contentCorrect")
            or entry.get("actualMarkerSamples", 0) <= 40
            or entry.get("naturalFramesBeforeCapture", 0) < 1
            or not entry.get("selectedDocumentIdentity")
            or entry.get("selectedDocumentIdentity") != entry.get("displayedDocumentIdentity")
            or not entry.get("outputUrl")
            or (entry.get("format") == "pdf" and (entry.get("actualPdfSource") != entry.get("outputUrl")
                or entry.get("referenceMarkerSamples", 0) <= 40
                or entry.get("overlappingMarkerSamples", 0) < entry.get("referenceMarkerSamples", 0) * 0.65))
            for entry in painted):
        failures.append("Publishing pane failed document identity, marker or natural-paint checks")
    if report.get("platform") != "windows":
        failures.append("Expected native Windows QPA; offscreen is insufficient for deployed acceptance")
    if Path(report.get("executable", "")).resolve() != executable.resolve():
        failures.append("Report came from a different executable")
    if report.get("executableSha256") != sha(executable) or sha(executable) != manifest["executableSha256"]:
        failures.append("Executable hash differs from manifest/report")
    if report.get("applicationVersion") != manifest["applicationVersion"]:
        failures.append("Application version differs from package manifest")
    required = {"PublishingPreview.qml", "LiveEditorPane.qml", "NativeMenuBar.qml", "WindowsMenuBar.qml",
                "Main.qml", "DocumentFooter.qml", "WorkspaceLayout.qml", "WorkspaceCommands.qml",
                "editor/index.html", "editor/editor.css", "editor/blocks.css", "editor/dist/editor.js",
                "themes/claude-like.css", "themes/omawrite.css", "DocumentFindBar.qml", "DocumentOutline.qml",
                "PaneZoomState.qml", "PaneZoomControls.qml", "WorkspaceHeader.qml", "LibraryPane.qml",
                "LinkEditor.qml", "LinkSyntax.js", "ExportHub.qml", "SquareDialogButton.qml"}
    reported = report.get("resourceSha256", {})
    if not required.issubset(reported):
        failures.append("Missing embedded hashes: " + ", ".join(sorted(required - reported.keys())))
    for name, actual in reported.items():
        if manifest["resourceSha256"].get(name) != actual:
            failures.append(f"Embedded resource differs from package source: {name}")
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, required=True, help="Deployed Fomawrite directory")
    parser.add_argument("--output", type=Path, required=True, help="New evidence directory")
    args = parser.parse_args()
    package, output = args.package.resolve(), args.output.resolve()
    executable = package / "Fomawrite.exe"
    manifest = json.loads((package / "manifest.json").read_text(encoding="utf-8"))
    if output.exists():
        parser.error("Use a new output directory, so old reports cannot satisfy acceptance")
    output.mkdir(parents=True)
    for name, expected in manifest["files"].items():
        if sha(package / name) != expected:
            raise SystemExit(f"Deployed file hash mismatch: {name}")
    environment = dict(os.environ)
    for name in list(environment):
        if name.startswith("QT_") or name.startswith("QML_") or name.startswith("FOMAWRITE_"):
            environment.pop(name)
    # Prove deployment without Qt/developer tools on PATH. Keep Windows itself.
    system_root = Path(os.environ.get("SystemRoot", r"C:\Windows"))
    environment["PATH"] = os.pathsep.join([str(package), str(system_root / "System32"), str(system_root)])
    environment["QT_FORCE_STDERR_LOGGING"] = "1"
    started = time.time_ns()
    with (output / "process.log").open("wb") as log:
        result = subprocess.run([str(executable), "--check-document-views", str(output)],
                                cwd=package, env=environment, stdout=log, stderr=subprocess.STDOUT, timeout=360)
    report_path = output / "report.json"
    if not report_path.is_file() or report_path.stat().st_mtime_ns < started:
        raise SystemExit(f"No fresh native acceptance report (exit {result.returncode}); see {output}")
    report = json.loads(report_path.read_text(encoding="utf-8"))
    failures = verify(report, manifest, executable)
    if result.returncode:
        failures.append(f"Application exited with status {result.returncode}")
    verification = {"passed": not failures, "failures": failures, "commit": manifest["commit"],
                    "architecture": manifest["architecture"], "executableSha256": sha(executable),
                    "platform": report.get("platform"), "report": str(report_path)}
    (output / "package-verification.json").write_text(json.dumps(verification, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(verification, indent=2))
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
