#!/usr/bin/env python3
"""Download and verify latest complete Windows CI packages using existing gh auth."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import struct
import subprocess
import sys
import tempfile
import zipfile

REPO = "andrewjngray/fomawrite_mac"
BRANCH = "windows/port"
ARCHES = ("arm64", "x64")
BASE = Path("dist/windows-testing")
MAX_ARCHIVE = 3 * 1024**3
MAX_EXTRACTED = 6 * 1024**3
MAX_FILES = 20000
SHA = re.compile(r"[0-9a-f]{64}")


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def gh(endpoint, destination=None):
    """Let gh manage credentials; never inspect or print authentication material."""
    command = ["gh", "api", f"repos/{REPO}/{endpoint}"]
    try:
        if destination:
            with destination.open("wb") as stream:
                result = subprocess.run(command, stdout=stream, stderr=subprocess.PIPE, timeout=1200)
        else:
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
        if result.returncode:
            raise ValueError("GitHub request failed; check connectivity and gh authentication")
        return None if destination else json.loads(result.stdout)
    except (FileNotFoundError, subprocess.TimeoutExpired) as error:
        raise ValueError("GitHub CLI unavailable or timed out; install/authenticate gh and retry") from error


def select_run():
    runs = gh("actions/workflows/windows.yml/runs?branch=windows%2Fport&status=success&per_page=20")
    for run in runs["workflow_runs"][:20]:
        commit = run.get("head_sha", "")
        repository = run.get("head_repository")
        if (run.get("event") not in ("push", "workflow_dispatch")
                or not isinstance(repository, dict) or repository.get("full_name") != REPO
                or run.get("status") != "completed" or run.get("conclusion") != "success"
                or run.get("head_branch") != BRANCH or not re.fullmatch(r"[0-9a-f]{40}", commit)
                or not isinstance(run.get("id"), int) or run["id"] <= 0):
            continue
        artifacts = gh(f"actions/runs/{run['id']}/artifacts?per_page=100")
        found = {}
        for arch in ARCHES:
            name = f"Fomawrite-internal-diagnostic-windows-{arch}-{commit}"
            matches = [a for a in artifacts["artifacts"] if a["name"] == name and not a.get("expired")]
            if len(matches) == 1:
                found[arch] = matches[0]
        if len(found) == 2:
            return run, found
    raise ValueError("No successful complete ARM64 + x64 build found in the latest 20 successful runs")


def safe_name(name):
    path = PurePosixPath(name)
    if (not name or "\\" in name or ":" in name or name.startswith("/")
            or any(part in ("", ".", "..") for part in name.rstrip("/").split("/"))
            or any(part.endswith((" ", ".")) for part in path.parts)):
        raise ValueError(f"Unsafe package path: {name!r}")
    return path


def extract(archive_path, destination):
    """Inspect every member before writing anything; never follow archive links."""
    if archive_path.stat().st_size > MAX_ARCHIVE:
        raise ValueError("Archive exceeds download size limit")
    with zipfile.ZipFile(archive_path) as archive:
        members = archive.infolist()
        if len(members) > MAX_FILES or sum(m.file_size for m in members) > MAX_EXTRACTED:
            raise ValueError("Archive exceeds extraction limits")
        seen = set()
        for member in members:
            path = safe_name(member.filename)
            key = str(path).casefold()
            mode = member.external_attr >> 16
            if (member.flag_bits & 1 or key in seen or stat.S_IFMT(mode) not in (0, stat.S_IFREG, stat.S_IFDIR)
                    or member.file_size > max(member.compress_size, 1) * 1000):
                raise ValueError("Archive has duplicate paths, links, special files or excessive compression")
            seen.add(key)
        archive.extractall(destination)


def verify(package, arch, commit, expected_manifest=None):
    if package.is_symlink() or not package.is_dir():
        raise ValueError("Missing package or linked package directory")
    paths = list(package.rglob("*"))
    if len(paths) > MAX_FILES or any(p.is_symlink() for p in paths):
        raise ValueError("Package contains links or too many files")
    manifest_path = package / "manifest.json"
    if manifest_path.stat().st_size > 16 * 1024**2:
        raise ValueError("Package manifest is too large")
    manifest_hash = sha(manifest_path)
    if expected_manifest and manifest_hash != expected_manifest:
        raise ValueError("Cached manifest digest mismatch")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if (not isinstance(manifest, dict) or manifest.get("schemaVersion") != 1 or manifest.get("application") != "Fomawrite"
            or manifest.get("architecture") != arch or manifest.get("commit") != commit
            or manifest.get("trackedSourceDirty") is not False
            or not isinstance(manifest.get("applicationVersion"), str) or not manifest["applicationVersion"]):
        raise ValueError("Package identity does not match the selected build")
    files = manifest.get("files", {})
    if not isinstance(files, dict) or not files or "Fomawrite.exe" not in files:
        raise ValueError("Package file manifest is incomplete")
    names = set()
    for name, digest in files.items():
        safe_name(name)
        if name.casefold() in names or not isinstance(digest, str) or not SHA.fullmatch(digest):
            raise ValueError("Invalid or duplicate manifest file")
        names.add(name.casefold())
        path = package / name
        if not path.is_file() or sha(path) != digest:
            raise ValueError(f"Package file digest mismatch: {name}")
    actual = {p.relative_to(package).as_posix() for p in paths if p.is_file()}
    if actual != set(files) | {"manifest.json"}:
        raise ValueError("Package contains unlisted or missing files")
    executable_hash = files["Fomawrite.exe"]
    native = manifest.get("nativeBuild", {})
    if (not isinstance(native, dict) or manifest.get("executableSha256") != executable_hash or native.get("commit") != commit
            or native.get("architecture") != arch or native.get("executableSha256") != executable_hash):
        raise ValueError("Executable identity differs from the native build")
    with (package / "Fomawrite.exe").open("rb") as stream:
        if stream.read(2) != b"MZ":
            raise ValueError("Executable has no PE header")
        stream.seek(0x3c)
        offset = struct.unpack("<I", stream.read(4))[0]
        stream.seek(offset)
        if stream.read(4) != b"PE\0\0" or struct.unpack("<H", stream.read(2))[0] != {"arm64": 0xaa64, "x64": 0x8664}[arch]:
            raise ValueError("Executable architecture differs from package identity")
    return {"manifestSha256": manifest_hash, "version": manifest["applicationVersion"]}


def cache_path(root, run_id, arch):
    relative = BASE / "downloads" / str(run_id) / arch / "Fomawrite"
    path = root / relative
    if any(p.is_symlink() for p in (path, *path.parents)):
        raise ValueError("Package cache may not use linked directories")
    return relative, path


def offline(root):
    current = json.loads((root / BASE / "current.json").read_text(encoding="utf-8"))
    if (not isinstance(current, dict) or current.get("schemaVersion") != 1 or not isinstance(current.get("runId"), int)
            or current["runId"] <= 0 or not re.fullmatch(r"[0-9a-f]{40}", current.get("commit", ""))):
        raise ValueError("Invalid current build identity")
    for arch in ARCHES:
        record = current["packages"][arch]
        relative, package = cache_path(root, current["runId"], arch)
        if record["path"] != relative.as_posix() or not SHA.fullmatch(record["manifestSha256"]):
            raise ValueError("Invalid current package reference")
        result = verify(package, arch, current["commit"], record["manifestSha256"])
        if result["version"] != record["version"]:
            raise ValueError("Current package version mismatch")
    return current


def update(root):
    run, artifacts = select_run()
    cache = root / BASE / "downloads"
    cache_path(root, run["id"], "arm64")
    cache.mkdir(parents=True, exist_ok=True)
    destination = cache / str(run["id"])
    records = {}
    # Existing immutable caches are verified and reused, never overwritten.
    if destination.exists():
        for arch in ARCHES:
            relative, package = cache_path(root, run["id"], arch)
            records[arch] = {"path": relative.as_posix(), **verify(package, arch, run["head_sha"])}
    else:
        with tempfile.TemporaryDirectory(prefix=".staging-", dir=cache) as temp:
            staging = Path(temp)
            for arch in ARCHES:
                artifact = artifacts[arch]
                digest = artifact.get("digest", "")
                if (not isinstance(digest, str) or not re.fullmatch(r"sha256:[0-9a-f]{64}", digest)
                        or not isinstance(artifact.get("id"), int) or artifact["id"] <= 0
                        or not 0 < artifact.get("size_in_bytes", 0) <= MAX_ARCHIVE):
                    raise ValueError("Artifact is missing a valid SHA-256 or size")
                archive = staging / f"{arch}.zip"
                gh(f"actions/artifacts/{artifact['id']}/zip", archive)
                if sha(archive) != digest.removeprefix("sha256:"):
                    raise ValueError("Downloaded artifact digest mismatch")
                unpacked = staging / f"unpacked-{arch}"
                extract(archive, unpacked)
                candidates = list(unpacked.rglob("manifest.json"))
                if not candidates:
                    nested = list(unpacked.rglob("*.zip"))
                    if len(nested) != 1:
                        raise ValueError("Artifact must contain exactly one application package ZIP")
                    extract(nested[0], staging / f"payload-{arch}")
                    candidates = list((staging / f"payload-{arch}").rglob("manifest.json"))
                if len(candidates) != 1 or candidates[0].parent.name != "Fomawrite":
                    raise ValueError("Artifact must contain exactly one Fomawrite manifest")
                package = candidates[0].parent
                result = verify(package, arch, run["head_sha"])
                (staging / arch).mkdir()
                package.rename(staging / arch / "Fomawrite")
                relative, _ = cache_path(root, run["id"], arch)
                records[arch] = {"path": relative.as_posix(), **result}
                archive.unlink()
                shutil.rmtree(unpacked)
                shutil.rmtree(staging / f"payload-{arch}", ignore_errors=True)
            staging.rename(destination)
    current = {"schemaVersion": 1, "runId": run["id"], "commit": run["head_sha"],
               "updatedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
               "runUrl": f"https://github.com/{REPO}/actions/runs/{run['id']}", "packages": records}
    pointer = root / BASE / "current.json"
    with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", prefix=".current-", dir=pointer.parent, delete=False) as stream:
        temporary = Path(stream.name)
        json.dump(current, stream, indent=2)
        stream.write("\n")
    try:
        temporary.replace(pointer)
    finally:
        temporary.unlink(missing_ok=True)
    return current


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--offline", action="store_true", help="verify current packages without GitHub access")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    try:
        current = (offline if args.offline else update)(args.root.resolve())
    except (OSError, ValueError, KeyError, TypeError, struct.error, zipfile.BadZipFile, NotImplementedError, RuntimeError) as error:
        print(f"Windows package update: {error}", file=sys.stderr)
        return 1
    print(f"Verified ARM64 + x64 build {current['commit'][:12]} (run {current['runId']})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
