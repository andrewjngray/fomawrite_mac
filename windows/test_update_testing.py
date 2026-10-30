"""Offline regression fixtures for the Mac Windows-package updater."""
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import stat
import struct
import tempfile
import unittest
from unittest import mock
import zipfile

SPEC = importlib.util.spec_from_file_location("update_testing", Path(__file__).resolve().parents[1] / "bin/windows/update-testing.py")
updater = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(updater)
COMMIT = "a" * 40


def package_zip(arch, commit=COMMIT):
    exe = bytearray(80)
    exe[:2] = b"MZ"
    struct.pack_into("<I", exe, 0x3c, 64)
    exe[64:68] = b"PE\0\0"
    struct.pack_into("<H", exe, 68, {"arm64": 0xaa64, "x64": 0x8664}[arch])
    digest = hashlib.sha256(exe).hexdigest()
    manifest = {"schemaVersion": 1, "application": "Fomawrite", "applicationVersion": "B-test",
                "commit": commit, "architecture": arch, "trackedSourceDirty": False,
                "executableSha256": digest, "files": {"Fomawrite.exe": digest},
                "nativeBuild": {"commit": commit, "architecture": arch, "executableSha256": digest}}
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w") as archive:
        archive.writestr("Fomawrite/Fomawrite.exe", exe)
        archive.writestr("Fomawrite/manifest.json", json.dumps(manifest))
    return buffer.getvalue()


class UpdateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.run = {"id": 123, "head_sha": COMMIT, "head_branch": "windows/port",
                    "status": "completed", "conclusion": "success", "event": "push",
                    "head_repository": {"full_name": updater.REPO}}
        self.payloads = {}
        self.artifacts = {}
        for index, arch in enumerate(updater.ARCHES, 1):
            # GitHub's artifact wraps the portable application ZIP and checksum.
            buffer = io.BytesIO()
            with zipfile.ZipFile(buffer, "w") as archive:
                archive.writestr(f"Fomawrite-test-{arch}.zip", package_zip(arch))
                archive.writestr(f"Fomawrite-test-{arch}.zip.sha256", "unused")
            data = buffer.getvalue()
            self.payloads[index] = data
            self.artifacts[arch] = {"id": index, "name": f"Fomawrite-internal-diagnostic-windows-{arch}-{COMMIT}",
                                    "digest": "sha256:" + hashlib.sha256(data).hexdigest(),
                                    "size_in_bytes": len(data), "expired": False}

    def github(self, endpoint, destination=None):
        if destination:
            destination.write_bytes(self.payloads[int(endpoint.split("/")[2])])
        elif endpoint.startswith("actions/workflows/"):
            return {"workflow_runs": [self.run]}
        else:
            return {"artifacts": list(self.artifacts.values())}

    def update(self):
        with mock.patch.object(updater, "gh", side_effect=self.github):
            return updater.update(self.root)

    def test_skips_diagnostic_only_run(self):
        diagnostic = {**self.run, "id": 124}
        def github(endpoint, destination=None):
            if endpoint.startswith("actions/workflows/"):
                return {"workflow_runs": [diagnostic, self.run]}
            if "/124/" in endpoint:
                return {"artifacts": [self.artifacts["arm64"]]}
            return self.github(endpoint, destination)
        with mock.patch.object(updater, "gh", side_effect=github):
            run, artifacts = updater.select_run()
        self.assertEqual(run["id"], 123)
        self.assertEqual(set(artifacts), {"arm64", "x64"})

    def test_rejects_fork_or_pull_request_runs_before_reading_artifacts(self):
        untrusted = [
            {**self.run, "event": "pull_request"},
            {**self.run, "head_repository": {"full_name": "someone/fomawrite_mac"}},
            {**self.run, "event": "workflow_dispatch", "head_repository": {"full_name": "someone/fomawrite_mac"}},
            {**self.run, "head_repository": None},
            {**self.run, "head_repository": {}},
            {**self.run, "head_repository": "unexpected"},
            {key: value for key, value in self.run.items() if key != "head_repository"},
            {key: value for key, value in self.run.items() if key != "event"},
        ]
        for run in untrusted:
            with self.subTest(run=run):
                def github(endpoint, destination=None):
                    self.assertTrue(endpoint.startswith("actions/workflows/"), "untrusted run artifacts were read")
                    return {"workflow_runs": [run]}
                with mock.patch.object(updater, "gh", side_effect=github):
                    with self.assertRaisesRegex(ValueError, "No successful complete"):
                        updater.select_run()

    def test_accepts_same_repository_workflow_dispatch(self):
        self.run["event"] = "workflow_dispatch"
        with mock.patch.object(updater, "gh", side_effect=self.github):
            selected, _ = updater.select_run()
        self.assertEqual(selected["id"], self.run["id"])

    def test_nested_update_and_offline_verification(self):
        current = self.update()
        with mock.patch.object(updater, "gh", side_effect=AssertionError("offline used GitHub")):
            self.assertEqual(updater.offline(self.root), current)
        for arch in updater.ARCHES:
            self.assertTrue((self.root / current["packages"][arch]["path"] / "Fomawrite.exe").is_file())
        self.assertEqual(list((self.root / updater.BASE / "downloads").iterdir()),
                         [self.root / updater.BASE / "downloads/123"])

    def test_partial_failure_preserves_current_and_old_packages(self):
        old = self.update()
        pointer = self.root / updater.BASE / "current.json"
        before = pointer.read_bytes()
        self.run = {**self.run, "id": 124}
        self.payloads[2] = b"corrupted second architecture"
        with self.assertRaisesRegex(ValueError, "digest mismatch"):
            self.update()
        self.assertEqual(pointer.read_bytes(), before)
        self.assertEqual(updater.offline(self.root), old)
        self.assertFalse((self.root / updater.BASE / "downloads/124").exists())
        self.assertFalse(list((self.root / updater.BASE / "downloads").glob(".staging-*")))

    def test_failed_download_preserves_current(self):
        self.update()
        pointer = self.root / updater.BASE / "current.json"
        before = pointer.read_bytes()
        self.run = {**self.run, "id": 124}
        original = self.github
        def github(endpoint, destination=None):
            if destination and "/2/" in endpoint:
                raise ValueError("network interrupted")
            return original(endpoint, destination)
        with mock.patch.object(updater, "gh", side_effect=github), self.assertRaisesRegex(ValueError, "network"):
            updater.update(self.root)
        self.assertEqual(pointer.read_bytes(), before)

    def test_cache_reuse_never_redownloads_or_replaces(self):
        current = self.update()
        executable = self.root / current["packages"]["arm64"]["path"] / "Fomawrite.exe"
        inode = executable.stat().st_ino
        def github(endpoint, destination=None):
            self.assertIsNone(destination, "existing cache was downloaded again")
            return self.github(endpoint)
        with mock.patch.object(updater, "gh", side_effect=github):
            updater.update(self.root)
        self.assertEqual(executable.stat().st_ino, inode)

    def test_corrupt_cache_rejected_without_overwrite(self):
        current = self.update()
        executable = self.root / current["packages"]["x64"]["path"] / "Fomawrite.exe"
        executable.write_bytes(b"corrupt")
        with self.assertRaisesRegex(ValueError, "digest mismatch"):
            self.update()
        self.assertEqual(executable.read_bytes(), b"corrupt")
        with self.assertRaisesRegex(ValueError, "digest mismatch"):
            updater.offline(self.root)

    def test_stale_identity_rejected_before_promoting(self):
        # A matching artifact name may still wrap an older native package.
        data = package_zip("arm64", "b" * 40)
        self.payloads[1] = data
        self.artifacts["arm64"].update(digest="sha256:" + hashlib.sha256(data).hexdigest(), size_in_bytes=len(data))
        with self.assertRaisesRegex(ValueError, "identity"):
            self.update()
        self.assertFalse((self.root / updater.BASE / "current.json").exists())

    def test_manifest_and_executable_architecture_are_checked(self):
        current = self.update()
        package = self.root / current["packages"]["arm64"]["path"]
        manifest = json.loads((package / "manifest.json").read_text())
        manifest["architecture"] = "x64"
        (package / "manifest.json").write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, "identity"):
            updater.verify(package, "arm64", COMMIT)
        manifest["architecture"] = "arm64"
        exe = package / "Fomawrite.exe"
        data = bytearray(exe.read_bytes())
        struct.pack_into("<H", data, 68, 0x8664)
        exe.write_bytes(data)
        digest = updater.sha(exe)
        manifest["files"]["Fomawrite.exe"] = digest
        manifest["executableSha256"] = digest
        manifest["nativeBuild"]["executableSha256"] = digest
        (package / "manifest.json").write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, "architecture"):
            updater.verify(package, "arm64", COMMIT)

    def test_unsafe_archive_members_rejected(self):
        for name in ("../escape", "/escape", "a/../../escape", "C:/escape", "a\\escape", "a./escape"):
            with self.subTest(name=name):
                archive = self.root / "bad.zip"
                with zipfile.ZipFile(archive, "w") as stream:
                    stream.writestr(name, b"bad")
                with self.assertRaises(ValueError):
                    updater.extract(archive, self.root / "unpack")
        self.assertFalse((self.root / "escape").exists())

    def test_symlinks_duplicates_and_bombs_rejected(self):
        for kind in ("symlink", "duplicate", "bomb"):
            with self.subTest(kind=kind):
                archive = self.root / "bad.zip"
                with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as stream:
                    if kind == "symlink":
                        member = zipfile.ZipInfo("linked")
                        member.create_system = 3
                        member.external_attr = (stat.S_IFLNK | 0o777) << 16
                        stream.writestr(member, "../escape")
                    elif kind == "duplicate":
                        stream.writestr("same", "one")
                        stream.writestr("SAME", "two")
                    else:
                        stream.writestr("bomb", b"0" * 2_000_000)
                with self.assertRaises(ValueError):
                    updater.extract(archive, self.root / "unpack")

    def test_missing_gh_is_reported_without_auth_details(self):
        with mock.patch.object(updater.subprocess, "run", side_effect=FileNotFoundError("secret")):
            with self.assertRaisesRegex(ValueError, "GitHub CLI unavailable") as error:
                updater.gh("actions/workflows/windows.yml/runs")
        self.assertNotIn("secret", str(error.exception))

    def test_offline_pointer_path_escape_rejected(self):
        self.update()
        pointer = self.root / updater.BASE / "current.json"
        current = json.loads(pointer.read_text())
        current["packages"]["arm64"]["path"] = "../elsewhere"
        pointer.write_text(json.dumps(current))
        with self.assertRaisesRegex(ValueError, "reference"):
            updater.offline(self.root)


if __name__ == "__main__":
    unittest.main()
