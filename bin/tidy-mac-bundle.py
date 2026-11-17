#!/usr/bin/env python3
"""Remove Qt modules Fomawrite does not use from a macdeployqt bundle, then every
framework or dylib nothing left loads, then gather licence files for the
remaining non-Qt Homebrew libraries.

macdeployqt copies QML modules it finds through dependency scanning, including
Qt Virtual Keyboard (GPL-3.0-only), Qt 3D, Timeline, Particles and others that
Fomawrite never imports. Usage: tidy-mac-bundle.py <Fomawrite.app> <licenses dir>
"""
import re
import shutil
import subprocess
import sys
from pathlib import Path

# QML modules and plug-in folders for features Fomawrite never uses. Its own
# imports are QtCore, QtQuick (Controls, Dialogs, Layouts, Pdf, Window),
# Qt.labs.platform, QtWebChannel and QtWebEngine.
UNUSED_QML = ["QtQuick/VirtualKeyboard", "QtQuick/Timeline", "QtQuick/Scene2D", "QtQuick/Scene3D",
              "QtQuick/Particles", "QtQuick/LocalStorage", "QtQml/XmlListModel", "QtQml/StateMachine"]
UNUSED_PLUGINS = ["platforminputcontexts", "geometryloaders", "sceneparsers", "renderers",
                  "multimedia", "sqldrivers"]
REPO_LICENSES = Path(__file__).resolve().parent.parent / "licenses"
MACHO_MAGIC = {b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe", b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca"}
LICENSE_NAME = re.compile(r"^(licen[cs]e|copying|notice|copyright)([.-].*)?$", re.I)


def is_macho(path):
    try:
        with open(path, "rb") as f:
            return f.read(4) in MACHO_MAGIC
    except OSError:
        return False


def dependencies(binary):
    out = subprocess.run(["otool", "-L", str(binary)], capture_output=True, text=True, check=True).stdout
    names = set()
    for line in out.splitlines()[1:]:
        dep = line.strip().split(" (compatibility")[0]
        match = re.search(r"([^/]+)\.framework/", dep)
        if match:
            names.add(match.group(1) + ".framework")
        elif dep.endswith(".dylib") and not dep.startswith("/usr/lib/") and not dep.startswith("/System/"):
            names.add(dep.rsplit("/", 1)[-1])
    return names


def main():
    app, licenses = Path(sys.argv[1]), Path(sys.argv[2])
    contents = app / "Contents"
    frameworks = contents / "Frameworks"
    # macdeployqt keeps QML plug-in libraries in PlugIns/quick, apart from the
    # module folders; remove the ones each unused module's qmldir names.
    for module in UNUSED_QML:
        folder = contents / "Resources/qml" / module
        for qmldir in folder.rglob("qmldir"):
            for line in qmldir.read_text().splitlines():
                words = line.split()
                if words[:1] == ["plugin"] or words[:2] == ["optional", "plugin"]:
                    plugin = words[words.index("plugin") + 1]
                    (contents / "PlugIns/quick" / f"lib{plugin}.dylib").unlink(missing_ok=True)
        shutil.rmtree(folder, ignore_errors=True)
    for folder in UNUSED_PLUGINS:
        shutil.rmtree(contents / "PlugIns" / folder, ignore_errors=True)

    # Everything outside Frameworks is a root; a reached framework brings all
    # of its own Mach-O files (WebEngine's helper app lives inside one).
    pending = [p for p in contents.rglob("*") if p.is_file() and frameworks not in p.parents and is_macho(p)]
    reached, seen = set(), set()
    while pending:
        binary = pending.pop()
        if binary in seen:
            continue
        seen.add(binary)
        for name in dependencies(binary) - reached:
            reached.add(name)
            item = frameworks / name
            if item.is_dir():
                pending += [p for p in item.rglob("*") if p.is_file() and not p.is_symlink() and is_macho(p)]
            elif item.is_file():
                pending.append(item)
    removed = []
    for item in sorted(frameworks.iterdir()):
        if item.name not in reached:
            shutil.rmtree(item) if item.is_dir() and not item.is_symlink() else item.unlink()
            removed.append(item.name)
    print(f"Removed {len(removed)} unused libraries: {' '.join(removed)}")

    # Licence files for the non-Qt libraries that remain, from their Homebrew kegs.
    target = licenses / "homebrew"
    target.mkdir(parents=True, exist_ok=True)
    missing = []
    for dylib in sorted(p.name for p in frameworks.glob("*.dylib")):
        matches = sorted(Path("/opt/homebrew/opt").glob(f"*/lib/{dylib}"))
        if not matches:
            missing.append(dylib)
            continue
        keg = matches[0].resolve().parent.parent
        formula = keg.parent.name
        files = [p for p in keg.iterdir() if p.is_file() and LICENSE_NAME.match(p.name)]
        files += [p for p in (keg / "share/doc").rglob("*") if p.is_file() and LICENSE_NAME.match(p.name)] \
            if not files and (keg / "share/doc").is_dir() else []
        # A keg without its own licence file (glib) uses the copy committed in
        # licenses/ for that exact version; a newer Homebrew version fails here.
        files += sorted(REPO_LICENSES.glob(f"{formula}-{keg.name}-*")) if not files else []
        if not files:
            missing.append(f"{dylib} ({formula} {keg.name})")
        for f in files:
            name = f.name if f.parent == REPO_LICENSES else f"{formula}-{keg.name}-{f.name}"
            shutil.copyfile(f, target / f"{name}{'' if f.suffix else '.txt'}")
    if missing:
        sys.exit("No licence file found for bundled: " + ", ".join(missing))
    print(f"Gathered {len(list(target.iterdir()))} Homebrew licence files")


if __name__ == "__main__":
    main()
