#!/usr/bin/env python3
"""Validate and archive a windeployqt directory, binding all files to its commit."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import struct
import subprocess
import xml.etree.ElementTree as ET
import zipfile
import build_identity


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def pe_architecture(path):
    with path.open("rb") as stream:
        if stream.read(2) != b"MZ":
            raise ValueError(f"Not a PE executable: {path}")
        stream.seek(0x3C)
        offset = struct.unpack("<I", stream.read(4))[0]
        stream.seek(offset)
        if stream.read(4) != b"PE\0\0":
            raise ValueError(f"Invalid PE signature: {path}")
        machine = struct.unpack("<H", stream.read(2))[0]
    return {0xAA64: "arm64", 0x8664: "x64"}.get(machine, f"unknown-{machine:x}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--architecture", choices=["arm64", "x64"], required=True)
    parser.add_argument("--qt-root", type=Path, required=True)
    parser.add_argument("--stage", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    stage = args.stage.resolve()
    build_record = build_identity.verify(root / f"build/windows-{args.architecture}", args.architecture)
    if sha(stage / "Fomawrite.exe") != build_record["executableSha256"]:
        raise SystemExit("Deployed executable differs from the verified native build")
    required = ["Fomawrite.exe", "Qt6Core.dll", "Qt6Gui.dll", "Qt6Quick.dll", "Qt6Qml.dll",
                "Qt6Pdf.dll", "Qt6WebEngineCore.dll", "QtWebEngineProcess.exe",
                "platforms/qwindows.dll", "qml/QtQuick/Controls/qmldir",
                "qml/QtWebEngine/qmldir", "qml/QtQuick/Pdf/qmldir",
                "resources/icudtl.dat", "resources/qtwebengine_resources.pak"]
    for item in required:
        if not (stage / item).is_file():
            raise SystemExit(f"Incomplete deployment: missing {item}")
    locales = list(stage.glob("translations/qtwebengine_locales/*.pak"))
    if not locales:
        raise SystemExit("Qt WebEngine locales missing")
    for path in stage.rglob("*"):
        if path.suffix.lower() in (".exe", ".dll") and pe_architecture(path) != args.architecture:
            raise SystemExit(f"Wrong PE architecture in deployment: {path}")
    # Runtime DLLs must be app-local, so portable use requires no administrator install.
    if not list(stage.glob("vcruntime140*.dll")) or not list(stage.glob("msvcp140*.dll")):
        raise SystemExit("App-local MSVC runtime missing")
    licenses = stage / "licenses"
    licenses.mkdir(exist_ok=True)
    for source, name in [("LICENSE", "Upstream-Omawrite-MIT.txt"),
                         ("fonts/OFL.txt", "Fonts-OFL.txt"),
                         ("fonts/IBMPlexMono-OFL.txt", "IBM-Plex-Mono-OFL.txt"),
                         ("src/vendor/md4c/LICENSE.md", "MD4C-MIT.txt"),
                         ("src/editor/dist/harper/LICENSE", "Harper-Apache-2.0.txt"),
                         ("windows/THIRD-PARTY-NOTICES.txt", "THIRD-PARTY-NOTICES.txt")]:
        shutil.copyfile(root / source, licenses / name)
    for source in (root / "windows/licenses").glob("*.txt"):
        shutil.copyfile(source, licenses / source.name)
    # Some SDK distributions also supply generated Chromium credits outside the
    # engine's built-in chrome://credits resource. Preserve them when supplied.
    credits = list(args.qt_root.rglob("qtwebengine_licenses.html"))
    if credits:
        shutil.copyfile(credits[0], licenses / "QtWebEngine-Chromium-credits.html")
    install = json.loads((args.qt_root / "fomawrite-qt-install.json").read_text())
    if install["architecture"] != args.architecture:
        raise SystemExit("Qt install architecture mismatch")
    resources = {}
    for element in ET.parse(root / "src/resources.qrc").iter("file"):
        relative = element.text
        resources[element.get("alias") or relative] = sha(root / "src" / relative)
    version = re.search(r'setApplicationVersion\(QStringLiteral\("([^"]+)"\)\)',
                        (root / "src/main.cpp").read_text()).group(1)
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    dirty = subprocess.check_output(["git", "status", "--porcelain", "--untracked-files=no"],
                                    cwd=root, text=True).strip()
    manifest = {"schemaVersion": 1, "application": "Fomawrite", "applicationVersion": version,
                "commit": commit, "trackedSourceDirty": bool(dirty), "architecture": args.architecture,
                "qtVersion": install["qtVersion"], "qtInstall": install, "nativeBuild": build_record,
                "compilerToolset": os.environ.get("VCToolsVersion"),
                "distributionQualification": "internal build; full licensing/install acceptance pending",
                "windowsSdk": os.environ.get("WindowsSDKVersion"), "buildOs": platform.platform(),
                "executableSha256": sha(stage / "Fomawrite.exe"),
                "resourceSha256": resources, "files": {
                    p.relative_to(stage).as_posix(): sha(p) for p in sorted(stage.rglob("*")) if p.is_file()
                }}
    # The manifest hashes every other deployed file; it cannot recursively hash itself.
    (stage / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    output = stage.parent / f"Fomawrite-{version}-windows-{args.architecture}-{commit[:12]}.zip"
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for path in sorted(stage.rglob("*")):
            if path.is_file():
                archive.write(path, Path("Fomawrite") / path.relative_to(stage))
    output.with_suffix(".zip.sha256").write_text(f"{sha(output)}  {output.name}\n", encoding="utf-8")
    print(f"Packaged {output}")


if __name__ == "__main__":
    main()
