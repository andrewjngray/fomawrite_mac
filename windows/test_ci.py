"""Regression guards against falsely accepting broken deployment evidence."""
import copy
import importlib.util
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


if __name__ == "__main__":
    unittest.main()
