#!/usr/bin/env python3
"""Run native Windows acceptance against a deployed portable directory.

Only the compiled --check-document-views harness is launched. It owns isolated
settings/recovery/IPC and disposable writing; normal startup is a separate gate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PureWindowsPath
import subprocess
import sys
import time

EXPECTED_PAINT = {
    "preview-pdf-document-a": "pdf",
    "preview-pdf-document-b": "pdf",
    "preview-pdf-rapid-final-c": "pdf",
    "preview-web-after-hidden-switch": "web",
    "preview-pdf-after-theme-mode-switch": "pdf",
    "preview-pdf-after-window-show": "pdf",
    "preview-web-after-pdf-return": "web",
    "preview-cold-pdf-component": "pdf",
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def same_windows_executable(actual, expected):
    """Compare guest paths lexically, without resolving them on the host OS."""
    paths = [PureWindowsPath(value) for value in (actual, expected)]
    if any(not path.is_absolute() or ".." in path.parts for path in paths):
        return False
    return paths[0] == paths[1]


def verify(report, manifest, executable, expected_executable=None):
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
    names = [entry.get("scenario") for entry in painted]
    if (not set(EXPECTED_PAINT).issubset(names) or len(names) != len(set(names))
            or any(entry.get("format") not in ("web", "pdf")
                   or (entry.get("scenario") in EXPECTED_PAINT
                       and entry.get("format") != EXPECTED_PAINT[entry["scenario"]]) for entry in painted)):
        failures.append("Missing, duplicated or incorrectly formatted required Web/PDF paint scenario")
    if not any(entry.get("scenario") == "rapid-selection-in-flight-request"
               and entry.get("observedPendingRequest") for entry in observations):
        failures.append("Rapid selection did not supersede an observed in-flight PDF request")
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
    executable_matches = (same_windows_executable(report.get("executable", ""), expected_executable)
                          if expected_executable is not None else
                          Path(report.get("executable", "")).resolve() == executable.resolve())
    if not executable_matches:
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


def deployment_environment(package, inherited):
    environment = dict(inherited)
    for name in list(environment):
        if name.upper().startswith(("QT_", "QTWEBENGINE", "QML_", "FOMAWRITE_")):
            environment.pop(name)
    system_root = Path(inherited.get("SystemRoot", r"C:\Windows"))
    environment["PATH"] = os.pathsep.join([str(package), str(system_root / "System32"), str(system_root)])
    environment["QT_FORCE_STDERR_LOGGING"] = "1"
    return environment


def verify_package_files(package, manifest):
    for name, expected in manifest["files"].items():
        path = (package / name).resolve()
        if not path.is_relative_to(package) or sha(path) != expected:
            raise SystemExit(f"Deployed file hash mismatch or invalid path: {name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, required=True, help="Deployed Fomawrite directory")
    parser.add_argument("--output", type=Path, help="New evidence directory")
    parser.add_argument("--verify-only", action="store_true", help="Check deployed file integrity without launch")
    parser.add_argument("--verify-report", type=Path, help="Verify a copied guest report offline, without launch")
    parser.add_argument("--expected-executable", help="Exact absolute Windows executable path used in the guest")
    args = parser.parse_args()
    if args.verify_report and (not args.expected_executable or args.output or args.verify_only):
        parser.error("--verify-report requires --expected-executable and cannot combine with --output/--verify-only")
    if args.expected_executable and not args.verify_report:
        parser.error("--expected-executable is only valid with --verify-report")
    if args.expected_executable and not same_windows_executable(args.expected_executable, args.expected_executable):
        parser.error("--expected-executable must be an absolute Windows path without parent traversal")
    package = args.package.resolve()
    executable = package / "Fomawrite.exe"
    manifest = json.loads((package / "manifest.json").read_text(encoding="utf-8"))
    verify_package_files(package, manifest)
    if args.verify_only:
        print("Deployed package hashes verified")
        return 0
    if args.verify_report:
        report_path = args.verify_report.resolve()
        output = report_path.parent
        receipt = json.loads((output / "guest-run.json").read_text(encoding="utf-8-sig"))
        report = json.loads(report_path.read_text(encoding="utf-8-sig"))
        failures = verify(report, manifest, executable, args.expected_executable)
        if (receipt.get("exitCode") != 0 or receipt.get("freshReport") is not True
                or receipt.get("reportSha256") != sha(report_path)
                or receipt.get("manifestSha256") != sha(package / "manifest.json")
                or receipt.get("executableSha256") != sha(executable)
                or not same_windows_executable(receipt.get("executable", ""), args.expected_executable)):
            failures.append("Guest launch receipt failed freshness, process exit, path or package/report identity")
    elif args.output is None:
        parser.error("--output is required for native acceptance")
    else:
        output = args.output.resolve()
        if output.exists():
            parser.error("Use a new output directory, so old reports cannot satisfy acceptance")
        output.mkdir(parents=True)
        # Prove deployment without Qt/developer tools on PATH. Keep Windows itself.
        environment = deployment_environment(package, os.environ)
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
                    "manifestSha256": sha(package / "manifest.json"),
                    "platform": report.get("platform"), "report": str(report_path)}
    if args.verify_report:
        verification["expectedExecutable"] = args.expected_executable
        verification["verificationMode"] = "copied-guest-report"
    (output / "package-verification.json").write_text(json.dumps(verification, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(verification, indent=2))
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
