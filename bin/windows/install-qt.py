#!/usr/bin/env python3
"""Install the pinned native Qt SDK archives, including extension repositories.

No Qt account or aqt extension support is required. This is a deliberately
bounded installer for the two native kits in windows/qt-6.11.3.json.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import urllib.request


def sha(path, algorithm="sha256"):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, algorithm).hexdigest()


def install(lock_path, architecture, output, seven_zip):
    lock = json.loads(lock_path.read_text(encoding="utf-8"))
    entries = lock["architectures"][architecture]
    # The output is owned by this installer. Do not merge with another SDK.
    if output.exists() and any(output.iterdir()):
        raise SystemExit(f"Qt output must be empty: {output}")
    output.mkdir(parents=True, exist_ok=True)
    records = []
    with tempfile.TemporaryDirectory(prefix="fomawrite-qt-") as temporary:
        for index, entry in enumerate(entries):
            url = entry["url"]
            if not url.startswith("https://download.qt.io/online/qtsdkrepository/"):
                raise SystemExit(f"Unexpected archive origin: {url}")
            if not re.fullmatch(r"[0-9a-f]{40}", entry["sha1"]):
                raise SystemExit("Invalid pinned Qt checksum")
            archive = Path(temporary) / f"{index}.7z"
            print(f"Downloading {entry['package']} ({entry['packageVersion']})", flush=True)
            urllib.request.urlretrieve(url, archive)
            if sha(archive, "sha1") != entry["sha1"]:
                raise SystemExit(f"Qt archive checksum mismatch: {url}")
            records.append({**entry, "sha256": sha(archive)})
            # Official native archives are relative to the kit root (bin/, lib/, ...).
            subprocess.run([seven_zip, "x", "-y", f"-o{output}", str(archive)], check=True,
                           stdout=subprocess.DEVNULL)
            archive.unlink()
    (output / "bin" / "qt.conf").write_text("[Paths]\nPrefix=..\n", encoding="utf-8")
    qmake = output / "bin" / "qmake.exe"
    query = subprocess.check_output([str(qmake), "-query"], text=True)
    values = dict(line.split(":", 1) for line in query.splitlines() if ":" in line)
    if values.get("QT_VERSION") != lock["qtVersion"]:
        raise SystemExit(f"Unexpected qmake version: {values.get('QT_VERSION')}")
    if Path(values["QT_INSTALL_PREFIX"]).resolve() != output.resolve():
        raise SystemExit(f"qmake did not relocate to {output}: {query}")
    # qmake's Windows libraries can still carry the build machine's absolute prefix.
    for path in (output / "lib").glob("*.prl"):
        text = path.read_text(encoding="utf-8")
        text = text.replace("C:/Users/qt/work/install/lib", "$$[QT_INSTALL_LIBS]")
        text = text.replace("c:/Users/qt/work/install/lib", "$$[QT_INSTALL_LIBS]")
        path.write_text(text, encoding="utf-8")
    required = ["bin/windeployqt.exe", "bin/Qt6WebEngineCore.dll", "bin/Qt6Pdf.dll",
                "lib/Qt6WebEngineQuick.lib", "qml/QtQuick/Controls/qmldir"]
    for name in required:
        if not (output / name).is_file():
            raise SystemExit(f"Incomplete Qt kit: missing {name}")
    manifest = {"qtVersion": lock["qtVersion"], "architecture": architecture,
                "lockSha256": sha(lock_path), "qmakeQuery": values, "archives": records}
    (output / "fomawrite-qt-install.json").write_text(json.dumps(manifest, indent=2) + "\n",
                                                   encoding="utf-8")
    print(query)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--architecture", choices=["arm64", "x64"], required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--lock", type=Path,
                        default=Path(__file__).resolve().parents[2] / "windows/qt-6.11.3.json")
    parser.add_argument("--seven-zip", default=shutil.which("7z") or
                        str(Path(os.environ.get("ProgramFiles", r"C:\Program Files")) / "7-Zip/7z.exe"))
    args = parser.parse_args()
    install(args.lock.resolve(), args.architecture, args.output.resolve(), args.seven_zip)


if __name__ == "__main__":
    main()
