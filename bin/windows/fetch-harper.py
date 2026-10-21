#!/usr/bin/env python3
"""Fetch only the bundled Harper runtime, checking the repository's npm integrity."""
import base64
import hashlib
import io
import json
from pathlib import Path
import re
import tarfile
import urllib.request

root = Path(__file__).resolve().parents[2]
lock = json.loads((root / "harper.lock.json").read_text())
data = urllib.request.urlopen(lock["tarball"], timeout=120).read()
actual = "sha512-" + base64.b64encode(hashlib.sha512(data).digest()).decode("ascii")
if actual != lock["integrity"]:
    raise SystemExit("Harper tarball integrity mismatch")
with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as archive:
    glue = [p.name for p in archive.getmembers()
            if re.fullmatch(r"package/dist/BinaryModule-[^/]+\.js", p.name)]
    if len(glue) != 1:
        raise SystemExit("Unexpected Harper glue layout")
    output = root / "src/editor/dist/harper"
    output.mkdir(parents=True, exist_ok=True)
    # Read named regular files; never extract paths supplied by a tarball.
    for source, target in [("package/dist/index.js", "index.js"),
                           ("package/dist/binary.js", "binary.js"),
                           (glue[0], "BinaryModule.js"),
                           ("package/dist/harper_wasm_bg.wasm", "harper_wasm_bg.wasm")]:
        member = archive.getmember(source)
        if not member.isfile():
            raise SystemExit(f"Harper entry is not a regular file: {source}")
        content = archive.extractfile(member).read()
        if source.endswith(".js"):
            content = content.replace(("./" + Path(glue[0]).name).encode(), b"./BinaryModule.js")
        (output / target).write_bytes(content)
    licenses = [m for m in archive.getmembers() if m.name.startswith("package/LICENSE") and m.isfile()]
    if not licenses:
        raise SystemExit("Harper license missing")
    (output / "LICENSE").write_bytes(archive.extractfile(licenses[0]).read())
(output / "VERSION").write_text(lock["version"] + "\n", encoding="utf-8")
print(f"Verified Harper {lock['version']}: {output}")
