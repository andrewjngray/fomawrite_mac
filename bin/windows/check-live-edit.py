#!/usr/bin/env python3
"""Offline verification of actual Windows Live input; never launches the app."""
import argparse
import base64
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("live_normal_contract", ROOT / "check-normal-launch.py")
NORMAL = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(NORMAL)
CASES = {"settled": -1, "immediate": 0, "50ms": 50, "100ms": 100, "150ms": 150}
SOURCES = ("run-live-edit-guest.ps1", "run-normal-launch-guest.ps1", "normal-launch-input.cs",
           "live-edit-steps.ps1", "live-edit-input.cs")
STAGES = (*NORMAL.STAGES[:4], "live-edit-workflow", *NORMAL.STAGES[4:])
ORIGINAL = NORMAL.EXPECTED_SAMPLE
EDITED = ORIGINAL + b"Live writing with Windows keys."
CHECKPOINTS = ("live-typed-saved", "live-undone-saved", "live-redone-saved")


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def raw_text(path):
    return path.read_bytes().decode("utf-8-sig")


def composed(root, case):
    body = raw_text(root / "run-normal-launch-guest.ps1")
    configuration = "$ErrorActionPreference = 'Stop'"
    hook = "    Capture 'live-mode-rendered'"
    gate = "gate = 'windows-normal-launch'"
    failure_anchor = "    $failure = $_.Exception.Message"
    for anchor in (configuration, hook, gate, failure_anchor):
        if body.count(anchor) != 1:
            raise ValueError(f"Normal driver composition anchor changed: {anchor}")
    body = body.replace(configuration, configuration + f"\n$script:LiveEditCase = '{case}'\n$script:LiveEditGapMs = {CASES[case]}")
    body = body.replace(hook, hook + "\n" + raw_text(root / "live-edit-steps.ps1"))
    body = body.replace(failure_anchor, failure_anchor + "\n    $_.Exception.ToString() | Set-Content -LiteralPath (Join-Path $outputRoot 'failure-exception.txt') -Encoding UTF8")
    body = body.replace(gate, "gate = 'windows-live-edit'").replace("normal-launch-report.json", "live-edit-report.json")
    return {"run-normal-launch-guest.ps1": body.encode(),
            "normal-launch-input.cs": (raw_text(root / "normal-launch-input.cs") + "\n" + raw_text(root / "live-edit-input.cs")).encode()}


def decoded(value):
    return base64.b64decode(value, validate=True)


def verify_case(report, package, evidence, expected_executable, case, source_root=ROOT):
    try:
        errors = NORMAL.verify(report, package, evidence, expected_executable, evidence.parent / "driver",
                               gate_id="windows-live-edit", stages=STAGES, expected_sample=EDITED)
    except (OSError, KeyError, TypeError, ValueError) as error:
        errors = [f"Incomplete normal-startup evidence: {error}"]
    try:
        generated = composed(source_root, case)
        composition = read_json(evidence / "driver-composition.json")
        if (composition["schemaVersion"] != 1 or composition["case"] != case
                or composition["requestedGapMs"] != CASES[case]
                or composition["sourceSha256"] != {name: NORMAL.sha(source_root / name) for name in SOURCES}):
            errors.append("Supplemental source composition identity differs")
        for name, contents in generated.items():
            if (evidence.parent / "driver" / name).read_bytes() != contents:
                errors.append(f"Supplemental generated driver differs: {name}")
        hashes = report["evidenceSha256"]
        required = {"driver-composition.json", "live-edit-observations.json", "live-text-focus.png",
                    "live-text-focus-uia.txt", "live-text-entered.png", "live-text-entered-uia.txt"}
        required |= {name + suffix for name in CHECKPOINTS for suffix in (".md", ".png", "-uia.txt")}
        if not required.issubset(hashes):
            errors.append("Supplemental input, focus or checkpoint evidence is missing")
        observation = read_json(evidence / "live-edit-observations.json")
        if (observation["schemaVersion"] != 1 or observation["case"] != case
                or observation["requestedGapMs"] != CASES[case] or observation["completed"] is not True
                or observation["failure"] is not None or observation["stableSamples"] != 3
                or decoded(observation["originalBase64"]) != ORIGINAL or decoded(observation["editedBase64"]) != EDITED):
            errors.append("Live workflow is incomplete or its expected text differs")
        target = observation["target"]
        word, region = target["word"], target["region"]
        if (word["text"] != "Saved" or word["width"] <= 0 or word["height"] <= 0
                or min(word["left"], word["top"]) < 0
                or word["left"] + word["width"] > region["width"]
                or word["top"] + word["height"] > region["height"]
                or abs(target["x"] - (region["left"] + word["left"] + word["width"] / 2)) > 1
                or abs(target["y"] - (region["top"] + word["top"] + word["height"] / 2)) > 1
                or any(region[k] != report["viewObservations"]["renderRegion"][k] for k in ("left", "top", "width", "height"))):
            errors.append("Precise OCR body-word target was not established inside the writing pane")
        focus = observation["focus"]
        if (focus["processId"] != report["launches"][0]["processId"] or focus["keyboardFocus"] is not True
                or focus["offscreen"] is not False or focus["controlType"] != "ControlType.Edit"):
            errors.append("Owned focused Live text input was not established")
        checkpoints = observation["checkpoints"]
        if [c["name"] for c in checkpoints] != list(CHECKPOINTS):
            errors.append("Saved, undone or redone checkpoint is missing or reordered")
        previous = NORMAL.parse_timestamp(report["checks"][3]["observedUtc"])
        finish = NORMAL.parse_timestamp(report["checks"][4]["observedUtc"])
        for checkpoint, wanted in zip(checkpoints, (EDITED, ORIGINAL, EDITED)):
            name = checkpoint["name"]
            path = evidence / (name + ".md")
            if (checkpoint["snapshot"] != name + ".md" or path.read_bytes() != wanted
                    or NORMAL.sha(path) != checkpoint["sha256"]):
                errors.append(f"Exact UTF-8 disk snapshot differs: {name}")
            stable = checkpoint["stableStates"]
            if len(stable) != 3:
                errors.append(f"Missing three stable saved observations: {name}")
            times = []
            for state in [*stable, checkpoint["state"]]:
                timestamp = NORMAL.parse_timestamp(state["observedUtc"])
                if (state["dirty"] is not False or state["title"] != "normal-launch-sample.md - Fomawrite"
                        or decoded(state["diskBase64"]) != wanted or not previous <= timestamp <= finish):
                    errors.append(f"Dirty, stale or wrong saved state: {name}")
                previous = timestamp
                times.append(timestamp)
            if len(stable) == 3 and (times[2][0] - times[0][0]).total_seconds() < .25:
                errors.append(f"Saved state was not observed across a stable interval: {name}")
        sample = evidence / "writing-normal-launch/normal-launch-sample.md"
        if sample.read_bytes() != EDITED:
            errors.append("Final/reopened Markdown differs from exact edited UTF-8 bytes")
        timing = observation["rapidTiming"]
        if case == "settled":
            if timing is not None:
                errors.append("Settled baseline unexpectedly substitutes a rapid case")
        else:
            if (timing["requestedGapMs"] != CASES[case] or timing["frequency"] <= 0
                    or not timing["undoBegin"] < timing["undoEnd"] <= timing["saveBegin"] < timing["saveEnd"]):
                errors.append("Rapid Undo/Save timing is missing or invalid")
            gap = (timing["saveBegin"] - timing["undoEnd"]) * 1000 / timing["frequency"]
            maximum = 10 if CASES[case] == 0 else CASES[case] + 40
            if not CASES[case] <= gap <= maximum:
                errors.append("Actual rapid shortcut interval differs from the required gap")
            stamp = NORMAL.parse_timestamp(timing["observedUtc"])
            if not NORMAL.parse_timestamp(report["startedUtc"]) <= stamp <= NORMAL.parse_timestamp(report["finishedUtc"]):
                errors.append("Rapid timing observation is stale")
    except (OSError, KeyError, TypeError, ValueError, ZeroDivisionError, IndexError) as error:
        errors.append(f"Incomplete or invalid supplemental evidence: {error}")
    return errors


def verify_series(series, package, output, expected_executable, source_root=ROOT):
    errors = []
    if (series.get("schemaVersion") != 1 or series.get("gate") != "windows-live-edit-series"
            or series.get("passed") is not True or series.get("allCases") is not True
            or [c.get("case") for c in series.get("cases", [])] != list(CASES)):
        errors.append("Require all five ordered settled/immediate/50/100/150ms cases to pass")
    profiles = set()
    for case, entry in zip(CASES, series.get("cases", [])):
        try:
            name = f"{case}/evidence/live-edit-report.json"
            path = output / name
            if (entry.get("case") != case or entry.get("requestedGapMs") != CASES[case]
                    or entry.get("passed") is not True or entry.get("exitCode") != 0
                    or entry.get("report") != name or entry.get("reportSha256") != NORMAL.sha(path)):
                errors.append(f"Case result/report identity differs: {case}")
            report = read_json(path)
            profile = str(report.get("profileRoot", "")).replace("\\", "/").casefold()
            if not profile or profile in profiles:
                errors.append(f"Fresh isolated profile was reused: {case}")
            profiles.add(profile)
            errors.extend(f"{case}: {failure}" for failure in verify_case(
                report, package, path.parent, expected_executable, case, source_root))
        except (OSError, ValueError, TypeError) as error:
            errors.append(f"Missing/invalid {case} report: {error}")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, required=True)
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument("--verify-series", type=Path)
    target.add_argument("--verify-case", type=Path)
    parser.add_argument("--case", choices=CASES)
    parser.add_argument("--expected-executable", required=True)
    args = parser.parse_args()
    if not NORMAL.same_executable(args.expected_executable, args.expected_executable):
        parser.error("--expected-executable must be an absolute Windows path without parent traversal")
    if bool(args.verify_case) != bool(args.case):
        parser.error("--verify-case requires --case; --case cannot reduce a full series")
    path = (args.verify_series or args.verify_case).resolve()
    if args.verify_series:
        failures = verify_series(read_json(path), args.package.resolve(), path.parent, args.expected_executable)
        filename = "live-edit-verification.json"
    else:
        failures = verify_case(read_json(path), args.package.resolve(), path.parent, args.expected_executable, args.case)
        filename = "live-edit-case-verification.json"
    receipt = {"passed": not failures, "fullAcceptance": args.verify_series is not None,
               "scope": "all-five-cases" if args.verify_series else "single-case:" + args.case,
               "failures": failures, "report": str(path), "reportSha256": NORMAL.sha(path)}
    (path.parent / filename).write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
