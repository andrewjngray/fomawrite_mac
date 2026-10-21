"""Bind a successful native build to its exact source input and executable hash."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def snapshot():
    inputs = subprocess.check_output(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z", "--",
         "src", "fonts", "fomawrite.pro", "harper.lock.json", "windows", "bin/windows"], cwd=ROOT)
    names = [name.decode("utf-8") for name in sorted(set(inputs.split(b"\0")) - {b""})]
    files = {name: sha(ROOT / name) for name in names
             if "__pycache__" not in Path(name).parts and not name.endswith(".pyc")}
    # RCC also embeds ignored, integrity-checked Harper runtime files. Git's
    # tracked-file inventory alone would fail to bind those build inputs.
    for element in ET.parse(ROOT / "src/resources.qrc").iter("file"):
        path = (ROOT / "src" / element.text).resolve()
        files[path.relative_to(ROOT).as_posix()] = sha(path)
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    fingerprint = hashlib.sha256(json.dumps(files, sort_keys=True).encode("utf-8")).hexdigest()
    return {"commit": commit, "sourceTreeSha256": fingerprint, "sourceFiles": files}


def verify(build, architecture):
    record = json.loads((build / "build-record.json").read_text(encoding="utf-8"))
    current = snapshot()
    for key in ("commit", "sourceTreeSha256", "sourceFiles"):
        if record[key] != current[key]:
            raise SystemExit(f"Source changed since native build ({key}); rebuild before packaging")
    if record["architecture"] != architecture:
        raise SystemExit("Native build record architecture mismatch")
    if record["executableSha256"] != sha(build / "fomawrite.exe"):
        raise SystemExit("Native executable changed since successful build; rebuild before packaging")
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["begin", "finish", "verify"])
    parser.add_argument("--architecture", choices=["arm64", "x64"], required=True)
    parser.add_argument("--build", type=Path, required=True)
    args = parser.parse_args()
    build = args.build.resolve()
    if args.command == "begin":
        record = {**snapshot(), "architecture": args.architecture,
                  "compilerToolset": os.environ.get("VCToolsVersion"),
                  "windowsSdk": os.environ.get("WindowsSDKVersion")}
        (build / "build-input.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
        (build / "build-record.json").unlink(missing_ok=True)
    elif args.command == "finish":
        record = json.loads((build / "build-input.json").read_text(encoding="utf-8"))
        current = snapshot()
        if any(record[key] != current[key] for key in current):
            raise SystemExit("Source changed during native compilation; rebuild from stable inputs")
        if record["architecture"] != args.architecture:
            raise SystemExit("Native build input architecture mismatch")
        record["executableSha256"] = sha(build / "fomawrite.exe")
        (build / "build-record.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    else:
        verify(build, args.architecture)


if __name__ == "__main__":
    main()
