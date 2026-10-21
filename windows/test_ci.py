"""Regression guards against falsely accepting broken deployment evidence."""
import copy
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "bin/windows"))


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / "bin/windows" / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


acceptance = load("check-document-views")
packaging = load("package-manifest")
identity = load("build_identity")


class DeploymentEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.exe = Path(self.temporary.name) / "Fomawrite.exe"
        self.exe.write_bytes(b"synthetic executable identity")
        resources = {name: "sha256" for name in (
            "PublishingPreview.qml LiveEditorPane.qml NativeMenuBar.qml WindowsMenuBar.qml Main.qml "
            "DocumentFooter.qml WorkspaceLayout.qml WorkspaceCommands.qml editor/index.html editor/editor.css "
            "editor/blocks.css editor/dist/editor.js themes/claude-like.css themes/omawrite.css "
            "DocumentFindBar.qml DocumentOutline.qml PaneZoomState.qml PaneZoomControls.qml WorkspaceHeader.qml "
            "LibraryPane.qml LinkEditor.qml LinkSyntax.js ExportHub.qml SquareDialogButton.qml").split()}
        self.manifest = {"applicationVersion": "test", "executableSha256": acceptance.sha(self.exe),
                         "resourceSha256": resources}
        observation = {"actualPanePainted": True, "contentCorrect": True, "actualMarkerSamples": 100,
                       "naturalFramesBeforeCapture": 1, "selectedDocumentIdentity": "B", "displayedDocumentIdentity": "B",
                       "outputUrl": "B.pdf", "format": "pdf", "actualPdfSource": "B.pdf",
                       "referenceMarkerSamples": 100, "overlappingMarkerSamples": 90}
        scenarios = [("preview-pdf-document-a", "pdf"), ("preview-pdf-document-b", "pdf"),
                     ("preview-pdf-rapid-final-c", "pdf"), ("preview-web-after-hidden-switch", "web"),
                     ("preview-pdf-after-theme-mode-switch", "pdf"), ("preview-pdf-after-window-show", "pdf"),
                     ("preview-web-after-pdf-return", "web"), ("preview-cold-pdf-component", "pdf")]
        observations = [{**copy.deepcopy(observation), "scenario": name, "format": format} for name, format in scenarios]
        observations.append({"scenario": "rapid-selection-in-flight-request", "observedPendingRequest": True})
        self.report = {"passed": True, "platform": "windows", "executable": str(self.exe),
                       **self.manifest, "publishingPreviewCorrectness": {"observations": observations}}
        for key, count in {"dailyWritingChecks": 5, "paneZoomChecks": 5, "editorAcceptanceChecks": 7,
                           "paneChromeChecks": 3, "editingLayoutChecks": 2, "publishingThemeChecks": 12}.items():
            self.report[key] = [{"passed": True} for _ in range(count)]
        self.report["editorAcceptanceChecks"][0]["name"] = "live-window-typing-source-switch-save-and-reopen"
        self.report["editorAcceptanceChecks"][1]["name"] = "live-window-keyboard-undo-and-redo"

    def failures(self):
        return acceptance.verify(self.report, self.manifest, self.exe)

    def test_complete_native_evidence_passes(self):
        self.assertEqual([], self.failures())

    def test_cross_host_guest_path_mapping_accepts_exact_windows_executable(self):
        expected = r"C:\Users\Andrew\Fomawrite Guest\Fomawrite.exe"
        self.report["executable"] = "C:/Users/Andrew/Fomawrite Guest/Fomawrite.exe"
        original = self.report["executable"]
        self.assertEqual([], acceptance.verify(self.report, self.manifest, self.exe, expected))
        self.assertEqual(original, self.report["executable"])

    def test_cross_host_wrong_expected_executable_is_rejected(self):
        self.report["executable"] = r"C:\Guest\Fomawrite\Fomawrite.exe"
        self.assertIn("Report came from a different executable", acceptance.verify(
            self.report, self.manifest, self.exe, r"C:\Other\Fomawrite.exe"))

    def test_cross_host_relative_expected_executable_is_rejected(self):
        self.report["executable"] = r"C:Guest\Fomawrite.exe"
        self.assertFalse(acceptance.same_windows_executable(self.report["executable"], self.report["executable"]))

    def test_offline_cli_verifies_copied_guest_evidence_without_launch(self):
        expected = r"C:\Guest\Fomawrite\Fomawrite.exe"
        self.report["executable"] = expected
        package = self.exe.parent
        manifest = {**self.manifest, "commit": "synthetic", "architecture": "arm64",
                    "files": {"Fomawrite.exe": acceptance.sha(self.exe)}}
        manifest_path = package / "manifest.json"
        manifest_path.write_text(json.dumps(manifest))
        evidence = package / "copied-evidence"
        evidence.mkdir()
        report_path = evidence / "report.json"
        report_path.write_text(json.dumps(self.report))
        original_report = report_path.read_bytes()
        receipt = {"executable": expected, "executableSha256": acceptance.sha(self.exe),
                   "manifestSha256": acceptance.sha(manifest_path), "reportSha256": acceptance.sha(report_path),
                   "exitCode": 0, "freshReport": True}
        (evidence / "guest-run.json").write_text(json.dumps(receipt))
        arguments = ["check-document-views.py", "--package", str(package), "--verify-report", str(report_path),
                     "--expected-executable", expected]
        with patch.object(sys, "argv", arguments), contextlib.redirect_stdout(io.StringIO()), \
                patch.object(acceptance.subprocess, "run", side_effect=AssertionError("Offline mode must not launch")):
            self.assertEqual(0, acceptance.main())
        self.assertEqual(original_report, report_path.read_bytes())
        verification = json.loads((evidence / "package-verification.json").read_text())
        self.assertTrue(verification["passed"])
        self.assertEqual(expected, verification["expectedExecutable"])

    def test_offscreen_is_rejected(self):
        self.report["platform"] = "offscreen"
        self.assertTrue(self.failures())

    def test_stale_document_pixels_are_rejected(self):
        self.report["publishingPreviewCorrectness"]["observations"][3]["displayedDocumentIdentity"] = "A"
        self.assertTrue(self.failures())

    def test_blank_paint_is_rejected(self):
        self.report["publishingPreviewCorrectness"]["observations"][3]["actualPanePainted"] = False
        self.assertTrue(self.failures())

    def test_inadequate_pdf_pixel_overlap_is_rejected(self):
        self.report["publishingPreviewCorrectness"]["observations"][2]["overlappingMarkerSamples"] = 64
        self.assertTrue(self.failures())

    def test_missing_embedded_hash_is_rejected(self):
        self.report["resourceSha256"] = dict(self.report["resourceSha256"])
        del self.report["resourceSha256"]["WindowsMenuBar.qml"]
        self.assertTrue(self.failures())

    def test_modified_executable_is_rejected(self):
        self.exe.write_bytes(b"another build")
        self.assertTrue(self.failures())

    def test_missing_live_input_check_is_rejected(self):
        self.report["editorAcceptanceChecks"][1].pop("name")
        self.assertTrue(self.failures())

    def test_failed_group_is_rejected(self):
        self.report["publishingThemeChecks"][3]["passed"] = False
        self.assertTrue(self.failures())

    def test_missing_web_is_rejected(self):
        for item in self.report["publishingPreviewCorrectness"]["observations"]:
            if item.get("format") == "web":
                item["format"] = "pdf"
        self.assertTrue(self.failures())

    def test_missing_pdf_is_rejected(self):
        for item in self.report["publishingPreviewCorrectness"]["observations"]:
            if item.get("format") == "pdf":
                item["format"] = "web"
        self.assertTrue(self.failures())

    def test_duplicate_observation_is_rejected(self):
        observations = self.report["publishingPreviewCorrectness"]["observations"]
        observations[1] = copy.deepcopy(observations[0])
        self.assertTrue(self.failures())

    def test_unknown_format_is_rejected(self):
        self.report["publishingPreviewCorrectness"]["observations"][1]["format"] = "unknown"
        self.assertTrue(self.failures())

    def test_missing_inflight_request_is_rejected(self):
        self.report["publishingPreviewCorrectness"]["observations"][-1]["observedPendingRequest"] = False
        self.assertTrue(self.failures())

    def test_inherited_webengine_paths_and_sandbox_flags_are_removed(self):
        inherited = {"PATH": "/developer/Qt/bin", "QTWEBENGINEPROCESS_PATH": "/other/helper.exe",
                     "QTWEBENGINE_DISABLE_SANDBOX": "1", "QTWEBENGINE_CHROMIUM_FLAGS": "--no-sandbox",
                     "QTWEBENGINE_RESOURCES_PATH": "/other/resources", "QTWEBENGINE_LOCALES_PATH": "/other/locales",
                     "QT_PLUGIN_PATH": "/other/plugins", "QML_IMPORT_PATH": "/other/qml", "FOMAWRITE_TEST_ROOT": "/other/profile"}
        result = acceptance.deployment_environment(self.exe.parent, inherited)
        self.assertFalse(set(inherited.keys()) - {"PATH"} & result.keys())
        self.assertNotIn("/developer/Qt/bin", result["PATH"])

    def test_changed_source_rejects_stale_build(self):
        build = self.exe.parent
        (build / "fomawrite.exe").write_bytes(b"built executable")
        old = {"commit": "old", "sourceTreeSha256": "old", "sourceFiles": {"Main.qml": "old"}}
        record = {**old, "architecture": "arm64", "executableSha256": identity.sha(build / "fomawrite.exe")}
        (build / "build-record.json").write_text(json.dumps(record))
        with patch.object(identity, "snapshot", return_value={**old, "sourceTreeSha256": "changed"}):
            with self.assertRaises(SystemExit):
                identity.verify(build, "arm64")

    def test_replaced_binary_rejects_stale_build(self):
        build = self.exe.parent
        (build / "fomawrite.exe").write_bytes(b"built executable")
        source = {"commit": "same", "sourceTreeSha256": "same", "sourceFiles": {"Main.qml": "same"}}
        record = {**source, "architecture": "arm64", "executableSha256": identity.sha(build / "fomawrite.exe")}
        (build / "build-record.json").write_text(json.dumps(record))
        (build / "fomawrite.exe").write_bytes(b"another executable")
        with patch.object(identity, "snapshot", return_value=source):
            with self.assertRaises(SystemExit):
                identity.verify(build, "arm64")

    def test_pe_machine_identifies_real_target(self):
        for machine, architecture in [(0xAA64, "arm64"), (0x8664, "x64"), (0xA641, "unknown-a641")]:
            data = bytearray(256)
            data[:2] = b"MZ"
            struct.pack_into("<I", data, 0x3C, 128)
            data[128:132] = b"PE\0\0"
            struct.pack_into("<H", data, 132, machine)
            self.exe.write_bytes(data)
            self.assertEqual(architecture, packaging.pe_architecture(self.exe))

    def runtime_candidate(self, family, version, architecture="arm64"):
        return {"path": family, "family": family, "files": {
            name: {"architecture": architecture, "version": version}
            for name in ("vcruntime140.dll", "msvcp140.dll")}}

    def test_matching_v143_runtime_is_preferred_to_latest_v145(self):
        matching = self.runtime_candidate("Microsoft.VC143.CRT", [14, 44, 35211, 0])
        latest = self.runtime_candidate("Microsoft.VC145.CRT", [14, 51, 36231, 0])
        self.assertEqual(matching["path"], packaging.select_runtime([latest, matching], [14, 44, 35207, 0], "arm64")["path"])

    def test_actual_newer_v14_runtime_is_compatible(self):
        latest = self.runtime_candidate("Microsoft.VC145.CRT", [14, 51, 36231, 0])
        self.assertEqual(latest["path"], packaging.select_runtime([latest], [14, 44, 35207, 0], "arm64")["path"])

    def test_older_runtime_cannot_satisfy_selected_compiler(self):
        old = self.runtime_candidate("Microsoft.VC143.CRT", [14, 44, 35112, 0])
        with self.assertRaises(SystemExit):
            packaging.select_runtime([old], [14, 44, 35207, 0], "arm64")

    def test_wrong_architecture_runtime_is_rejected(self):
        wrong = self.runtime_candidate("Microsoft.VC145.CRT", [14, 51, 36231, 0], "x64")
        with self.assertRaises(SystemExit):
            packaging.select_runtime([wrong], [14, 44, 35207, 0], "arm64")

    def test_mixed_runtime_architectures_are_rejected(self):
        mixed = self.runtime_candidate("Microsoft.VC145.CRT", [14, 51, 36231, 0])
        mixed["files"]["msvcp140.dll"]["architecture"] = "unknown-a641"
        with self.assertRaises(SystemExit):
            packaging.select_runtime([mixed], [14, 44, 35207, 0], "arm64")

    def test_unknown_runtime_family_is_rejected(self):
        unknown = self.runtime_candidate("Microsoft.VC999.CRT", [14, 51, 36231, 0])
        with self.assertRaises(SystemExit):
            packaging.select_runtime([unknown], [14, 44, 35207, 0], "arm64")

    def test_partial_runtime_is_rejected(self):
        partial = self.runtime_candidate("Microsoft.VC145.CRT", [14, 51, 36231, 0])
        del partial["files"]["msvcp140.dll"]
        with self.assertRaises(SystemExit):
            packaging.select_runtime([partial], [14, 44, 35207, 0], "arm64")

    def test_arm64_runtime_excludes_unimported_x64_sidecar(self):
        candidate = self.runtime_candidate("Microsoft.VC143.CRT", [14, 44, 35211, 0])
        candidate["files"]["vcruntime140_1.dll"] = {"architecture": "x64", "version": [14, 44, 35211, 0]}
        selected = packaging.select_runtime([candidate], [14, 44, 35207, 0], "arm64", {"vcruntime140.dll", "msvcp140.dll"})
        self.assertNotIn("vcruntime140_1.dll", selected["files"])
        self.assertEqual("x64", selected["excludedFiles"]["vcruntime140_1.dll"]["architecture"])

    def test_x64_runtime_keeps_legitimately_imported_vcruntime140_1(self):
        candidate = self.runtime_candidate("Microsoft.VC143.CRT", [14, 44, 35211, 0], "x64")
        candidate["files"]["vcruntime140_1.dll"] = {"architecture": "x64", "version": [14, 44, 35211, 0]}
        selected = packaging.select_runtime([candidate], [14, 44, 35207, 0], "x64", {"vcruntime140_1.dll"})
        self.assertIn("vcruntime140_1.dll", selected["files"])

    def test_arm64_import_requiring_foreign_runtime_is_rejected(self):
        candidate = self.runtime_candidate("Microsoft.VC143.CRT", [14, 44, 35211, 0])
        candidate["files"]["vcruntime140_1.dll"] = {"architecture": "x64", "version": [14, 44, 35211, 0]}
        with self.assertRaises(SystemExit):
            packaging.select_runtime([candidate], [14, 44, 35207, 0], "arm64", {"vcruntime140_1.dll"})

    def test_native_crt_transitive_import_requiring_foreign_runtime_is_rejected(self):
        candidate = self.runtime_candidate("Microsoft.VC143.CRT", [14, 44, 35211, 0])
        candidate["files"]["vcruntime140_1.dll"] = {"architecture": "x64", "version": [14, 44, 35211, 0]}
        candidate["files"]["msvcp140.dll"]["imports"] = ["vcruntime140_1.dll"]
        with self.assertRaises(SystemExit):
            packaging.select_runtime([candidate], [14, 44, 35207, 0], "arm64")

    def test_missing_required_native_crt_is_rejected(self):
        candidate = self.runtime_candidate("Microsoft.VC143.CRT", [14, 44, 35211, 0])
        with self.assertRaises(SystemExit):
            packaging.select_runtime([candidate], [14, 44, 35207, 0], "arm64", {"msvcp140_atomic_wait.dll"})

    def test_dumpbin_reads_normal_and_delay_import_sections(self):
        listing = """Dump of file synthetic.exe
  Section contains the following imports:
    MSVCP140.dll
  Section contains the following delay load imports:
    VCRUNTIME140_1.dll
"""
        with patch.object(packaging.subprocess, "check_output", return_value=listing) as command:
            self.assertEqual(["msvcp140.dll", "vcruntime140_1.dll"], packaging.dumpbin_imports(self.exe))
            self.assertIn("/imports", command.call_args.args[0])

    def test_empty_dumpbin_output_cannot_prove_foreign_runtime_unused(self):
        with patch.object(packaging.subprocess, "check_output", return_value="Unexpected dumpbin output"):
            with self.assertRaises(ValueError):
                packaging.dumpbin_imports(self.exe)


if __name__ == "__main__":
    unittest.main()
