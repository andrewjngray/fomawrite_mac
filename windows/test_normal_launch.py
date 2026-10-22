"""Fail-closed report verification; these tests do not substitute for the guest UI run."""
import hashlib
import importlib.util
import json
import struct
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("normal_launch", ROOT / "bin/windows/check-normal-launch.py")
GATE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GATE)


class NormalLaunchEvidenceTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.package = self.root / "package"
        self.output = self.root / "evidence"
        self.package.mkdir()
        self.output.mkdir()
        self.exe = r"C:\Acceptance\Fomawrite\Fomawrite.exe"
        (self.package / "Fomawrite.exe").write_bytes(b"fixture executable")
        executable_hash = GATE.sha(self.package / "Fomawrite.exe")
        manifest = {"executableSha256": executable_hash, "files": {"Fomawrite.exe": executable_hash}}
        (self.package / "manifest.json").write_text(json.dumps(manifest))
        hashes = {}
        for stage in GATE.STAGES:
            for suffix, contents in ((".png", b"\x89PNG\r\n\x1a\nfixture pixels"), ("-uia.txt", b"ControlType.Button | fixture")):
                path = self.output / (stage + suffix)
                path.write_bytes(contents)
                hashes[path.name] = GATE.sha(path)
        for name in ("folder-picker-and-navigation-picker-uia.txt", "file-menu-and-native-picker-picker-uia.txt",
                     "launch-1-stdout.log", "launch-1-stderr.log", "launch-2-stdout.log", "launch-2-stderr.log"):
            path = self.output / name
            path.write_text("fixture")
            hashes[name] = GATE.sha(path)
        for name, contents in (("live-editor-content.png", b"\x89PNG\r\n\x1a\n" + struct.pack(">I4sII", 13, b"IHDR", 800, 300)),
                               ("live-editor-ocr.txt", b"Normal launch acceptance Saved with Windows input."),
                               ("live-mode-rendered.png", b"\x89PNG\r\n\x1a\nfixture screenshot"),
                               ("live-mode-rendered-uia.txt", b"visible Live editor fixture")):
            path = self.output / name
            path.write_bytes(contents)
            hashes[name] = GATE.sha(path)
        profile = self.output / "profile"
        profile.mkdir()
        profile_root = r"C:\Acceptance\new-run\profile"
        # Qt's canonical profile paths use '/' separators even on Windows.
        canonical_root = profile_root.replace("\\", "/")
        namespace = "FomawriteAcceptance-" + hashlib.sha256(canonical_root.encode()).hexdigest()[:20]
        data = "C:/Users/fixture/AppData/Roaming/AndrewGray/" + namespace
        receipt = {"profileRoot": canonical_root, "normalStartup": True, "executable": self.exe,
                   "applicationName": namespace,
                   "settingsPath": "\\HKEY_CURRENT_USER\\Software\\AndrewGray\\" + namespace,
                   "dataPath": data, "cachePath": "C:/Users/fixture/AppData/Local/AndrewGray/" + namespace + "/cache",
                   "workspacePath": data + "/workspace-" + "a" * 20 + ".json", "identity": "a" * 20}
        (profile / "profile.json").write_text(json.dumps(receipt))
        sample = self.output / "writing-normal-launch/normal-launch-sample.md"
        sample.parent.mkdir()
        sample.write_bytes(GATE.EXPECTED_SAMPLE)
        driver = ROOT / "bin/windows"
        self.report = {
            "schemaVersion": 1, "gate": "windows-normal-launch", "passed": True,
            "executable": self.exe, "executableSha256": executable_hash,
            "manifestSha256": GATE.sha(self.package / "manifest.json"),
            "inputMethod": "Win32-SendInput", "discoveryMethod": "read-only-UIAutomation",
            "checks": [{"name": stage, "passed": True, "observedUtc": "2026-10-22T00:00:01Z"} for stage in GATE.STAGES],
            "launches": [{"arguments": [], "processId": n, "exitCode": 0} for n in (1, 2)],
            "startedUtc": "2026-10-22T00:00:00Z", "finishedUtc": "2026-10-22T00:00:02Z",
            "viewObservations": {"sourceBeforeVisible": True, "liveSourceAbsent": True, "sourceAfterVisible": True,
                                 "liveHeadingRendered": True, "liveBodyRendered": True,
                                 "liveModeLabel": "Live follows the output style: Modern (Sans)",
                                 "renderRegion": {"left": 0, "top": 0, "width": 800, "height": 300}},
            "evidenceSha256": hashes, "sampleSha256": GATE.sha(sample), "profileRoot": profile_root,
            "outputRoot": r"C:\Acceptance\new-run", "profileReceiptSha256": GATE.sha(profile / "profile.json"),
            "driverSha256": GATE.sha(driver / "run-normal-launch-guest.ps1"),
            "inputHelperSha256": GATE.sha(driver / "normal-launch-input.cs"),
        }

    def failures(self):
        return GATE.verify(self.report, self.package, self.output, self.exe)

    def test_complete_evidence_is_accepted(self):
        self.assertEqual([], self.failures())

    def test_diagnostic_launch_cannot_satisfy_ordinary_startup(self):
        self.report["launches"][0]["arguments"] = ["--check-document-views"]
        self.assertTrue(self.failures())

    def test_menu_open_without_successful_save_is_rejected(self):
        self.report["checks"][3]["passed"] = False
        self.assertTrue(self.failures())

    def test_missing_or_duplicated_stage_is_rejected(self):
        self.report["checks"][4] = self.report["checks"][3]
        self.assertTrue(self.failures())

    def test_relaunch_must_cleanly_exit(self):
        self.report["launches"][1]["exitCode"] = None
        self.assertTrue(self.failures())

    def test_uia_invoke_cannot_substitute_for_windows_input(self):
        self.report["inputMethod"] = "UIAutomation-InvokePattern"
        self.assertTrue(self.failures())

    def test_stale_observation_is_rejected(self):
        self.report["checks"][0]["observedUtc"] = "2026-10-21T23:59:59Z"
        self.assertTrue(self.failures())

    def test_missing_native_picker_evidence_is_rejected(self):
        del self.report["evidenceSha256"]["file-menu-and-native-picker-picker-uia.txt"]
        self.assertTrue(self.failures())

    def test_modified_screenshot_is_rejected(self):
        (self.output / "edit-and-save.png").write_bytes(b"other pixels")
        self.assertTrue(self.failures())

    def test_escape_from_evidence_directory_is_rejected(self):
        escape = self.root / "unrelated.txt"
        escape.write_text("fixture")
        self.report["evidenceSha256"]["../unrelated.txt"] = GATE.sha(escape)
        self.assertTrue(self.failures())

    def test_source_must_match_even_when_report_hash_matches_modified_file(self):
        sample = self.output / "writing-normal-launch/normal-launch-sample.md"
        sample.write_bytes(b"wrong document")
        self.report["sampleSha256"] = GATE.sha(sample)
        self.assertTrue(self.failures())

    def test_missing_profile_receipt_prevents_acceptance(self):
        (self.output / "profile/profile.json").unlink()
        self.assertTrue(self.failures())

    def test_modified_profile_receipt_is_rejected(self):
        path = self.output / "profile/profile.json"
        receipt = json.loads(path.read_text())
        receipt["dataPath"] = "user-data"
        path.write_text(json.dumps(receipt))
        self.assertTrue(self.failures())

    def test_profile_must_belong_to_new_output(self):
        self.report["outputRoot"] = r"C:\Users\Andrew\real-profile"
        self.assertTrue(self.failures())

    def test_real_user_paths_cannot_satisfy_isolation_even_with_matching_receipt_hash(self):
        path = self.output / "profile/profile.json"
        receipt = json.loads(path.read_text())
        receipt["dataPath"] = "C:/Users/fixture/AppData/Roaming/AndrewGray/fomawrite"
        path.write_text(json.dumps(receipt))
        self.report["profileReceiptSha256"] = GATE.sha(path)
        self.assertTrue(self.failures())

    def test_wrong_root_derived_namespace_is_rejected(self):
        path = self.output / "profile/profile.json"
        receipt = json.loads(path.read_text())
        receipt["applicationName"] = "FomawriteAcceptance-" + "b" * 20
        path.write_text(json.dumps(receipt))
        self.report["profileReceiptSha256"] = GATE.sha(path)
        self.assertTrue(self.failures())

    def test_renderer_and_page_load_failures_reject_otherwise_complete_evidence(self):
        path = self.output / "launch-1-stderr.log"
        for failure in ("qml: Live editor page failed to load:", "QtWebEngineProcess.exe renderer crashed",
                        "fomawrite editor failed to start", "The Live editor stopped unexpectedly."):
            path.write_text(failure)
            self.report["evidenceSha256"][path.name] = GATE.sha(path)
            self.assertTrue(any("Renderer or Live page-load failure" in error for error in self.failures()), failure)

    def test_benign_gpu_vendor_warning_is_accepted(self):
        path = self.output / "launch-1-stderr.log"
        path.write_text("Unknown Vendor ID.\n")
        self.report["evidenceSha256"][path.name] = GATE.sha(path)
        self.assertEqual([], self.failures())

    def test_mode_label_alone_cannot_prove_live_rendering(self):
        path = self.output / "live-editor-ocr.txt"
        path.write_text("Live editor could not load")
        self.report["evidenceSha256"][path.name] = GATE.sha(path)
        self.assertTrue(self.failures())

    def test_live_visibility_must_hide_source_editor(self):
        self.report["viewObservations"]["liveSourceAbsent"] = False
        self.assertTrue(self.failures())

    def test_live_render_screenshot_must_match_observed_writing_region(self):
        self.report["viewObservations"]["renderRegion"]["width"] += 1
        self.assertTrue(self.failures())

    def test_wrong_executable_and_replaced_driver_are_rejected(self):
        self.report["executable"] = r"C:\Other\Fomawrite.exe"
        self.report["driverSha256"] = "different"
        self.assertTrue(self.failures())


if __name__ == "__main__":
    unittest.main()
