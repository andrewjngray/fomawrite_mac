#!/usr/bin/env python3
"""Strict offline verification of the Windows ordinary-startup SendInput gate.

Run run-normal-launch-guest.ps1 in an unlocked guest desktop first, then copy
its entire new output directory and the exact package to this machine.
"""
import argparse
from datetime import datetime
import hashlib
import json
import re
import struct
from pathlib import Path, PureWindowsPath
import sys

STAGES = (
    "ordinary-startup", "folder-picker-and-navigation", "file-menu-and-native-picker",
    "edit-and-save", "view-buttons", "quit-and-restore",
)
EXPECTED_SAMPLE = b"# Normal launch acceptance\n\nSaved with Windows input.\n"
RENDER_FAILURE = re.compile(
    r"Live editor page failed to load|fomawrite editor failed to start|"
    r"The Live editor (?:could not load|stopped unexpectedly)|"
    r"(?:renderer|render process|QtWebEngineProcess)[^\r\n]{0,160}(?:crash|terminated|failed|fatal)|"
    r"(?:crash|terminated|failed|fatal)[^\r\n]{0,160}(?:renderer|QtWebEngineProcess)|"
    r"RenderProcessGone|STATUS_ACCESS_VIOLATION|\bFATAL[:\]]", re.IGNORECASE)



def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def same_executable(actual, expected):
    try:
        paths = [PureWindowsPath(value) for value in (actual, expected)]
    except (TypeError, ValueError):
        return False
    return all(p.is_absolute() and ".." not in p.parts for p in paths) and paths[0] == paths[1]


def parse_timestamp(value):
    """Read .NET round-trip UTC timestamps on Python 3.9 without losing order.

    datetime stores microseconds; retain the seventh fractional digit as a
    separate 100 ns tick so comparisons still detect sub-microsecond reversals.
    Only six digits reach fromisoformat, including on older Python versions.
    """
    match = re.fullmatch(r"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})(?:\.(\d{1,7}))?(Z|[+-]\d{2}:\d{2})", value)
    if not match:
        raise ValueError("Expected an ISO timestamp with an explicit timezone and up to seven fractional digits")
    whole, fraction, zone = match.groups()
    fraction = (fraction or "").ljust(7, "0")
    normalized = whole + "." + fraction[:6] + ("+00:00" if zone == "Z" else zone)
    return datetime.fromisoformat(normalized), int(fraction[6])


def verify(report, package, evidence, expected_executable, driver_root=None, *,
           gate_id="windows-normal-launch", stages=STAGES, expected_sample=EXPECTED_SAMPLE):
    """Fail closed on incomplete workflows, stale/missing evidence and identity."""
    errors = []
    manifest_path = package / "manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    executable = package / "Fomawrite.exe"
    if (report.get("schemaVersion") != 1 or report.get("gate") != gate_id
            or report.get("passed") is not True):
        errors.append("Gate did not report an unconditional normal-launch pass")
    if (not same_executable(report.get("executable"), expected_executable)
            or report.get("executableSha256") != sha(executable)
            or manifest.get("executableSha256") != sha(executable)
            or report.get("manifestSha256") != sha(manifest_path)):
        errors.append("Executable path, binary or manifest identity differs")
    for name, expected in manifest["files"].items():
        path = (package / name).resolve()
        if not path.is_relative_to(package.resolve()) or not path.is_file() or sha(path) != expected:
            errors.append(f"Package file is missing, modified or unsafe: {name}")
    if (report.get("inputMethod") != "Win32-SendInput"
            or report.get("discoveryMethod") != "read-only-UIAutomation"):
        errors.append("Required Windows input and read-only discovery were not used")
    checks = report.get("checks", [])
    if ([check.get("name") for check in checks] != list(stages)
            or any(check.get("passed") is not True for check in checks)):
        errors.append("Required workflow stage is missing, failed, reordered or duplicated")
    launches = report.get("launches", [])
    if (len(launches) != 2 or any(launch.get("arguments") != [] or launch.get("exitCode") != 0
                                or not isinstance(launch.get("processId"), int) or launch["processId"] <= 0
                                for launch in launches)):
        errors.append("Expected two argument-free ordinary launches and clean exits")
    try:
        start = parse_timestamp(report["startedUtc"])
        finish = parse_timestamp(report["finishedUtc"])
        observations = [parse_timestamp(check["observedUtc"]) for check in checks]
        if (start >= finish
                or any(not start <= t <= finish for t in observations)
                or observations != sorted(observations)):
            raise ValueError("invalid timing")
    except (KeyError, TypeError, ValueError):
        errors.append("Invalid run interval or stale stage observation")
    hashes = report.get("evidenceSha256", {})
    required = {f"{stage}{suffix}" for stage in stages for suffix in (".png", "-uia.txt")}
    required |= {f"launch-{n}-{stream}.log" for n in (1, 2) for stream in ("stdout", "stderr")}
    required |= {f"{stage}-picker-uia.txt" for stage in ("folder-picker-and-navigation", "file-menu-and-native-picker")}
    required |= {"live-editor-content.png", "live-editor-ocr.txt", "live-mode-rendered.png", "live-mode-rendered-uia.txt"}
    if not required.issubset(hashes):
        errors.append("Screenshots, UIA observations, picker or launch logs are missing")
    for name, expected in hashes.items():
        path = (evidence / name).resolve()
        if not path.is_relative_to(evidence.resolve()) or not path.is_file() or sha(path) != expected:
            errors.append(f"Evidence is missing, modified or unsafe: {name}")
    for stage in stages:
        screenshot = evidence / f"{stage}.png"
        tree = evidence / f"{stage}-uia.txt"
        if screenshot.is_file() and not screenshot.read_bytes().startswith(b"\x89PNG\r\n\x1a\n"):
            errors.append(f"Invalid screenshot: {stage}")
        if tree.is_file() and not tree.read_text(encoding="utf-8-sig").strip():
            errors.append(f"Empty UIA observation: {stage}")
    for launch in (1, 2):
        for stream in ("stdout", "stderr"):
            log = evidence / f"launch-{launch}-{stream}.log"
            if log.is_file() and RENDER_FAILURE.search(log.read_text(encoding="utf-8-sig", errors="replace")):
                errors.append(f"Renderer or Live page-load failure in {log.name}")
    view = report.get("viewObservations") or {}
    if (any(view.get(name) is not True for name in ("sourceBeforeVisible", "liveSourceAbsent", "sourceAfterVisible", "liveHeadingRendered", "liveBodyRendered"))
            or not str(view.get("liveModeLabel", "")).startswith("Live follows the output style:")):
        errors.append("Actual Source/Live visibility and rendered content were not established")
    ocr = evidence / "live-editor-ocr.txt"
    if (not ocr.is_file() or not all(phrase in " ".join(ocr.read_text(encoding="utf-8-sig").casefold().split())
                                   for phrase in ("normal launch acceptance", "saved with windows input"))):
        errors.append("Live writing-region OCR does not contain the sample heading and saved sentence")
    content = evidence / "live-editor-content.png"
    region = view.get("renderRegion") or {}
    try:
        data = content.read_bytes()
        if (not data.startswith(b"\x89PNG\r\n\x1a\n") or data[12:16] != b"IHDR"
                or struct.unpack(">II", data[16:24]) != (region["width"], region["height"])
                or region["width"] < 100 or region["height"] < 60):
            raise ValueError("wrong render region")
    except (OSError, ValueError, TypeError, KeyError, struct.error):
        errors.append("Live writing-region screenshot is missing or differs from the observed render region")
    sample = evidence / "writing-normal-launch/normal-launch-sample.md"
    if (not sample.is_file() or sample.read_bytes().replace(b"\r\n", b"\n") != expected_sample
            or report.get("sampleSha256") != sha(sample)):
        errors.append("Saved sample differs from exact expected Markdown")
    profile_receipt = evidence / "profile/profile.json"
    try:
        isolation = json.loads(profile_receipt.read_text(encoding="utf-8-sig"))
        if (isolation.get("normalStartup") is not True
                or not same_executable(isolation.get("profileRoot"), report.get("profileRoot"))
                or not same_executable(isolation.get("executable"), expected_executable)
                or report.get("profileReceiptSha256") != sha(profile_receipt)
                or not same_executable(report.get("profileRoot"), str(PureWindowsPath(report.get("outputRoot", "")) / "profile"))
                or any(not isolation.get(key) for key in ("settingsPath", "dataPath", "cachePath", "workspacePath", "identity"))):
            raise ValueError("wrong isolation identity")
        canonical_root = isolation["profileRoot"]
        namespace = "FomawriteAcceptance-" + hashlib.sha256(canonical_root.encode("utf-8")).hexdigest()[:20]
        if isolation.get("applicationName") != namespace:
            raise ValueError("profile namespace is not derived from canonical root")
        for key in ("settingsPath", "dataPath", "cachePath"):
            if namespace.casefold() not in {part.casefold() for part in isolation[key].replace("\\", "/").split("/")}:
                raise ValueError("settings/data/cache escaped acceptance namespace")
        identity = isolation["identity"]
        workspace = PureWindowsPath(isolation["workspacePath"])
        if (not re.fullmatch(r"[0-9a-f]{20}", identity)
                or workspace.parent != PureWindowsPath(isolation["dataPath"])
                or workspace.name != f"workspace-{identity}.json"):
            raise ValueError("workspace path or IPC identity differs from isolated profile")
    except (OSError, ValueError, TypeError):
        errors.append("App-written isolated ordinary-startup profile receipt is missing or invalid")
    if driver_root is None:
        driver_root = Path(__file__).resolve().parent
    for field, name in (("driverSha256", "run-normal-launch-guest.ps1"), ("inputHelperSha256", "normal-launch-input.cs")):
        if report.get(field) != sha(driver_root / name):
            errors.append(f"Evidence came from a different Windows input driver: {name}")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, required=True)
    parser.add_argument("--verify-report", type=Path, required=True)
    parser.add_argument("--expected-executable", required=True)
    args = parser.parse_args()
    if not same_executable(args.expected_executable, args.expected_executable):
        parser.error("--expected-executable must be an absolute Windows path without parent traversal")
    report_path = args.verify_report.resolve()
    report = json.loads(report_path.read_text(encoding="utf-8-sig"))
    errors = verify(report, args.package.resolve(), report_path.parent, args.expected_executable)
    result = {"passed": not errors, "failures": errors, "report": str(report_path), "reportSha256": sha(report_path)}
    (report_path.parent / "normal-launch-verification.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    return bool(errors)


if __name__ == "__main__":
    sys.exit(main())
