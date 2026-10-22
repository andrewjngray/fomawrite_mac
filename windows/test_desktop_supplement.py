"""Synthetic rejection guards; these do not qualify an interactive desktop run."""
import contextlib
import copy
from datetime import datetime, timedelta
import importlib.util
import io
import json
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch
from windows import test_normal_launch as BASE

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('desktop_supplement', ROOT / 'bin/windows/check-desktop-supplement.py')
GATE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GATE)


def instant(seconds):
    return (datetime(2026, 10, 22) + timedelta(seconds=seconds)).isoformat() + 'Z'


class DesktopSupplementTests(unittest.TestCase):
    def setUp(self):
        BASE.NormalLaunchEvidenceTests.setUp(self)
        self.driver = self.output.parent / 'driver'
        self.driver.mkdir()
        for name, contents in GATE.composed().items():
            (self.driver / name).write_bytes(contents)
        manifest = GATE.read_json(self.package / 'manifest.json')
        manifest.update(commit='a' * 40, architecture='arm64', trackedSourceDirty=False,
                        nativeBuild={'commit': 'a' * 40, 'sourceFiles': {
                            'src/backend.cpp': 'b' * 64, 'src/WindowsMenuBar.qml': 'c' * 64}})
        (self.package / 'manifest.json').write_text(json.dumps(manifest))
        self.report.update(gate='windows-desktop-supplement', passed=False, qualified=False,
                           qualificationPending=True, workflowPassed=True, finishedUtc=instant(12),
                           manifestSha256=GATE.NORMAL.sha(self.package / 'manifest.json'),
                           driverSha256=GATE.NORMAL.sha(self.driver / 'run-normal-launch-guest.ps1'),
                           inputHelperSha256=GATE.NORMAL.sha(self.driver / 'normal-launch-input.cs'))
        self.report['checks'] = [{'name': s, 'passed': True, 'observedUtc': instant(t)}
                                 for s, t in zip(GATE.STAGES, (.5, .6, .7, 1, 9, 10, 11))]
        png = b'\x89PNG\r\n\x1a\n' + struct.pack('>I4sII', 13, b'IHDR', 800, 600)
        for name in (*GATE.CAPTURES, 'desktop-supplement-workflow'):
            (self.output / (name + '.png')).write_bytes(png)
            (self.output / (name + '-uia.txt')).write_text('Owned visible fixture')
        (self.output / 'writing-normal-launch/desktop-public-review.md').write_bytes(GATE.FIXTURE)
        self.receipt = {'stageOnly': True, 'commit': manifest['commit'], 'architecture': 'arm64',
                        'manifestSha256': self.report['manifestSha256'], 'executableSha256': self.report['executableSha256'],
                        'localPackage': r'C:\Acceptance\Fomawrite'}
        self.composition = {'schemaVersion': 1, 'preparedOnly': False, 'commit': manifest['commit'],
                            'architecture': 'arm64', 'manifestSha256': self.report['manifestSha256'],
                            'executableSha256': self.report['executableSha256'], 'backendSourceSha256': 'b' * 64,
                            'menuSourceSha256': 'c' * 64, 'localStageReceipt': 'local-stage-receipt.json',
                            'sourceSha256': {n: GATE.NORMAL.sha(GATE.ROOT / n) for n in GATE.SOURCES},
                            'generatedDriverSha256': self.report['driverSha256'],
                            'generatedInputSha256': self.report['inputHelperSha256'], 'nativeParsePassed': True,
                            'helperCompilePending': True, 'clipboardTouched': False, 'observedUtc': instant(0)}
        self.updater = {'completed': True, 'initialChecked': True, 'reopenedChecked': False,
                        'offElapsedMs': 1000, 'failure': None}
        initial = self.geometry(100, 100, 6.2)
        before = self.geometry(200, 200, 6.5)
        samples = [self.geometry(500, 300, t) for t in (7, 7.15, 7.3)]
        self.center = {'initial': initial, 'before': before, 'afterSamples': samples, 'assertionPassed': True,
                       'failure': None, 'stableSamples': 3, 'stableIntervalMs': 300}
        selection = lambda word, t: {'processId': 1, 'rangeCount': 1, 'keyboardFocus': True,
                                    'text': word, 'observedUtc': instant(t)}
        details = [
            {'physicalKeys': 'Ctrl+Shift+O', 'opened': True, 'canceled': True},
            {'physicalKeys': 'Ctrl+Shift+P', 'opened': True, 'canceled': True,
             'ownedQuery': {'processId': 1, 'hwnd': 99, 'present': True, 'ready': True,
                            'keyboardFocus': True, 'queryAutomationId': 'fixture.commandQuery', 'observedUtc': instant(3)}},
            {'physicalKeys': 'Ctrl+Shift+OEM_1', 'openingChordSent': True, 'closingChordSent': True,
             'selectedText': '', 'closedSelectedText': '', 'visualReviewPending': True,
             'visibleCardCount': None, 'engineObserved': None,
             'openCapture': 'desktop-review-open.png', 'closeCapture': 'desktop-review-toggle.png'},
            {'physicalKeys': 'Ctrl+OEM_1', 'selection': selection('flomptoid', 5)},
            {'physicalKeys': 'Ctrl+Alt+OEM_1', 'selection': selection('quorbleflat', 6)},
            {'initial': initial, 'before': before, 'after': samples[-1], 'movementObserved': True,
             'selection': selection('quorbleflat', 8)},
        ]
        self.obs = {'schemaVersion': 1, 'completed': True, 'qualified': False, 'nativeVisualReviewPending': True,
                    'visualReviewRequired': True, 'clipboardTouched': False, 'failure': None,
                    'binding': self.composition, 'updaterSetup': self.updater,
                    'checks': [{'id': n, 'passed': True, 'capture': 'desktop-' + n + '.png',
                                'observedUtc': instant(t), 'detail': d}
                               for n, t, d in zip(GATE.CHECKS, (2, 3, 4, 5, 6, 8), details)]}
        self.report_path = self.output / 'desktop-supplement-report.json'
        self.sync()

    def geometry(self, x, y, t):
        return {'processId': 1, 'hwnd': 99, 'monitor': 44, 'left': x, 'top': y,
                'width': 1000, 'height': 600, 'visibleLeft': x, 'visibleTop': y,
                'visibleWidth': 1000, 'visibleHeight': 600, 'workLeft': 0, 'workTop': 0,
                'workWidth': 2000, 'workHeight': 1200, 'dpi': 192, 'visibleFrameHresult': 0,
                'minimized': False, 'maximized': False, 'keyboardLayout': '04090409', 'observedUtc': instant(t)}

    def sync(self):
        for directory in (self.output, self.driver):
            (directory / 'local-stage-receipt.json').write_text(json.dumps(self.receipt))
        self.composition['localStageReceiptSha256'] = GATE.NORMAL.sha(self.output / 'local-stage-receipt.json')
        for directory in (self.output, self.driver):
            (directory / 'driver-composition.json').write_text(json.dumps(self.composition))
        for name, value in (('desktop-supplement-updater-setup.json', self.updater),
                            ('center-native-observations.json', self.center), ('desktop-supplement-observations.json', self.obs)):
            (self.output / name).write_text(json.dumps(value))
        self.report['desktopSupplementObservations'] = self.obs
        self.report['evidenceSha256'] = {p.name: GATE.NORMAL.sha(p) for p in self.output.iterdir()
                                       if p.is_file() and p.name not in (self.report_path.name, 'desktop-supplement-verification.json')}
        self.report_path.write_text(json.dumps(self.report))

    def result(self, review=None):
        return GATE.verify(self.report, self.package, self.output, self.exe, review=review, report_path=self.report_path)

    def visual_review(self):
        return {'schemaVersion': 1, 'gate': 'windows-desktop-supplement-visual-review', 'reviewedBy': 'Independent fixture reviewer',
                'reviewedUtc': instant(13), 'reportSha256': GATE.NORMAL.sha(self.report_path),
                'captures': {n: GATE.NORMAL.sha(self.output / n) for n in ('desktop-review-open.png', 'desktop-review-toggle.png')},
                'engine': 'Harper 2.10.0 (built-in)', 'cards': ['flomptoid', 'recieve', 'quorbleflat'], 'reviewClosed': True}

    def reject(self):
        self.sync()
        result = self.result(self.visual_review())
        self.assertFalse(result['passed'])
        self.assertTrue(result['failures'])

    def test_valid_workflow_without_review_stays_pending(self):
        result = self.result()
        self.assertEqual([], result['failures'])
        self.assertTrue(result['workflowPassed'])
        self.assertTrue(result['qualificationPending'])
        self.assertFalse(result['passed'])
        self.assertFalse(result['fullM2Acceptance'])
        self.assertEqual('review-center-supplement', result['scope'])

    def test_hash_bound_three_card_visual_review_accepts_scoped_workflow(self):
        self.assertTrue(self.result(self.visual_review())['passed'])
        self.assertFalse(self.report['passed'])

    def test_cli_pending_does_not_launch_or_mutate_raw_report(self):
        original = self.report_path.read_bytes()
        argv = ['check-desktop-supplement.py', '--package', str(self.package), '--verify-report', str(self.report_path),
                '--expected-executable', self.exe]
        with patch.object(sys, 'argv', argv), contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(2, GATE.main())
        self.assertEqual(original, self.report_path.read_bytes())

    def test_archived_crlf_sources_compose_exact_mixed_line_endings(self):
        archived = self.root / 'archived-crlf-sources'
        archived.mkdir()
        lf_sources = self.root / 'archived-lf-sources'
        lf_sources.mkdir()
        for name in GATE.SOURCES:
            # Normalize fixture input only; production verification stays byte-exact.
            data = (GATE.ROOT / name).read_bytes().replace(b'\r\n', b'\n')
            (lf_sources / name).write_bytes(data)
            (archived / name).write_bytes(data.replace(b'\n', b'\r\n'))
        generated = GATE.composed(archived)
        self.assertEqual((archived / 'normal-launch-input.cs').read_bytes() + b'\n' +
                         (archived / 'desktop-supplement-input.cs').read_bytes(), generated['normal-launch-input.cs'])
        body = generated['run-normal-launch-guest.ps1']
        self.assertIn(b"$ErrorActionPreference = 'Stop'\n$script:DesktopSupplementObservation=", body)
        self.assertIn(b'nativeVisualReviewPending=$true}\n\r\n', body)
        for name, data in generated.items():
            (self.driver / name).write_bytes(data)
        self.report['driverSha256'] = self.composition['generatedDriverSha256'] = GATE.NORMAL.sha(self.driver / 'run-normal-launch-guest.ps1')
        self.report['inputHelperSha256'] = self.composition['generatedInputSha256'] = GATE.NORMAL.sha(self.driver / 'normal-launch-input.cs')
        self.composition['sourceSha256'] = {n: GATE.NORMAL.sha(archived / n) for n in GATE.SOURCES}
        self.sync()
        result = GATE.verify(self.report, self.package, self.output, self.exe, source_root=archived)
        self.assertEqual([], result['failures'])
        self.assertTrue(result['qualificationPending'])
        self.assertTrue(GATE.verify(self.report, self.package, self.output, self.exe, source_root=lf_sources)['failures'])
        argv = ['check-desktop-supplement.py', '--package', str(self.package), '--verify-report', str(self.report_path),
                '--expected-executable', self.exe, '--source-root', str(archived)]
        with patch.object(sys, 'argv', argv), contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(2, GATE.main())

    def test_raw_pass_or_removed_pending_flags_rejected(self):
        for field, value in (('passed', True), ('qualified', True), ('qualificationPending', False), ('workflowPassed', False)):
            original = self.report[field]
            self.report[field] = value
            self.reject()
            self.report[field] = original

    def test_changed_generated_helper_rejected_even_if_rehashed(self):
        path = self.driver / 'normal-launch-input.cs'
        path.write_bytes(path.read_bytes() + b'\n// changed')
        self.composition['generatedInputSha256'] = self.report['inputHelperSha256'] = GATE.NORMAL.sha(path)
        self.reject()

    def test_changed_generated_driver_rejected_even_if_rehashed(self):
        path = self.driver / 'run-normal-launch-guest.ps1'
        path.write_bytes(path.read_bytes() + b'\n# changed')
        self.composition['generatedDriverSha256'] = self.report['driverSha256'] = GATE.NORMAL.sha(path)
        self.reject()

    def test_wrong_source_identity_or_preparation_only_rejected(self):
        self.composition['sourceSha256']['desktop-supplement-input.cs'] = '0' * 64
        self.composition['preparedOnly'] = True
        self.reject()

    def test_wrong_package_commit_rejected(self):
        self.composition['commit'] = self.receipt['commit'] = 'd' * 40
        self.reject()

    def test_foreign_stage_receipt_rejected_even_if_rehashed(self):
        self.receipt['localPackage'] = r'C:\Other\Fomawrite'
        self.reject()

    def test_changed_copied_receipt_rejected(self):
        (self.driver / 'local-stage-receipt.json').write_text('{}')
        self.assertTrue(self.result()['failures'])

    def test_missing_evidence_hash_rejected(self):
        del self.report['evidenceSha256']['desktop-review-open.png']
        self.assertTrue(self.result()['failures'])

    def test_modified_capture_rejected_without_rehash(self):
        (self.output / 'desktop-review-open.png').write_bytes(b'other pixels')
        self.assertTrue(self.result()['failures'])

    def test_zero_sized_capture_rejected_even_if_rehashed(self):
        (self.output / 'desktop-review-open.png').write_bytes(b'\x89PNG\r\n\x1a\n' + struct.pack('>I4sII', 13, b'IHDR', 0, 0))
        self.reject()

    def test_changed_fixture_rejected(self):
        (self.output / 'writing-normal-launch/desktop-public-review.md').write_bytes(b'other writing')
        self.reject()

    def test_missing_or_duplicate_check_rejected(self):
        self.obs['checks'][4] = copy.deepcopy(self.obs['checks'][3])
        self.reject()

    def test_stale_or_reordered_check_rejected(self):
        self.obs['checks'][3]['observedUtc'] = instant(2)
        self.reject()

    def test_foreign_or_unfocused_issue_selection_rejected(self):
        d = self.obs['checks'][3]['detail']['selection']
        d['processId'] = 2
        d['keyboardFocus'] = False
        self.reject()

    def test_selection_observed_after_its_check_rejected(self):
        self.obs['checks'][3]['detail']['selection']['observedUtc'] = instant(8)
        self.reject()

    def test_wrong_previous_issue_text_rejected(self):
        self.obs['checks'][4]['detail']['selection']['text'] = 'flomptoid'
        self.reject()

    def test_review_cannot_invent_automatic_three_card_readiness(self):
        self.obs['checks'][2]['detail']['visibleCardCount'] = 3
        self.reject()

    def test_updater_late_or_uncompleted_rejected(self):
        self.updater['offElapsedMs'] = 9000
        self.reject()

    def test_failed_dwm_call_or_invalid_numeric_geometry_rejected(self):
        for field, value in (('visibleFrameHresult', -1), ('visibleWidth', float('nan')), ('dpi', 0), ('height', float('inf'))):
            original = self.center['afterSamples'][-1][field]
            self.center['afterSamples'][-1][field] = value
            self.reject()
            self.center['afterSamples'][-1][field] = original

    def test_center_invisible_outer_rectangle_cannot_replace_visible_bounds(self):
        for w in self.center['afterSamples']:
            w['visibleTop'] = 330
        self.reject()

    def test_center_requires_three_stable_samples_over_300ms(self):
        self.center['afterSamples'][1]['observedUtc'] = instant(7.05)
        self.center['afterSamples'][2]['observedUtc'] = instant(7.1)
        self.reject()

    def test_center_movement_stability_monitor_and_size_rejected(self):
        for field, value in (('monitor', 88), ('width', 999), ('hwnd', 77)):
            original = self.center['afterSamples'][-1][field]
            self.center['afterSamples'][-1][field] = value
            self.reject()
            self.center['afterSamples'][-1][field] = original

    def test_valid_visible_center_allows_invisible_resize_border(self):
        states = [self.center['initial'], self.center['before'], *self.center['afterSamples']]
        for w in states:
            w['left'] = w['visibleLeft'] - 11
            w['width'] = 1022
            w['height'] = 611
        self.sync()
        self.assertTrue(self.result(self.visual_review())['passed'])

    def test_center_requires_three_samples_not_two(self):
        self.center['afterSamples'] = self.center['afterSamples'][1:]
        self.reject()

    def test_center_initial_before_order_and_claimed_sample_count_rejected(self):
        self.center['initial']['observedUtc'] = instant(8.5)
        self.center['stableSamples'] = 99
        self.reject()

    def test_geometry_huge_integer_or_boolean_handle_rejected_without_crash(self):
        self.center['afterSamples'][-1]['visibleWidth'] = 10**1000
        self.center['afterSamples'][-1]['hwnd'] = True
        self.reject()

    def test_unsafe_hash_index_path_rejected(self):
        self.report['evidenceSha256']['../outside.txt'] = '0' * 64
        self.assertTrue(self.result()['failures'])

    def test_composition_fails_when_original_unique_anchor_changes(self):
        root = self.root / 'changed-sources'
        root.mkdir()
        for name in GATE.SOURCES:
            (root / name).write_bytes((GATE.ROOT / name).read_bytes())
        path = root / 'run-normal-launch-guest.ps1'
        path.write_text(path.read_text() + "\n    Capture 'live-mode-rendered'\n")
        with self.assertRaisesRegex(ValueError, 'anchor differs'):
            GATE.composed(root)

    def test_nonnull_raw_failure_rejected(self):
        self.report['failure'] = 'Retained native workflow error'
        self.reject()

    def test_zero_monitor_rejected_even_without_monitor_transition(self):
        for w in [self.center['initial'], self.center['before'], *self.center['afterSamples']]:
            w['monitor'] = 0
        self.reject()

    def test_titlebar_preparation_cannot_change_initial_dimensions(self):
        self.center['initial']['width'] += 20
        self.reject()

    def test_initial_after_before_timestamp_rejected(self):
        self.center['initial']['observedUtc'] = instant(6.6)
        self.reject()

    def test_duplicate_base_supplement_stage_rejected(self):
        self.report['checks'][5]['name'] = 'desktop-supplement-workflow'
        self.reject()

    def test_normal_restore_requires_clean_exit(self):
        self.report['launches'][1]['exitCode'] = None
        self.reject()

    def test_two_card_wrong_engine_and_unclosed_visual_reviews_rejected(self):
        for field, value in (('cards', ['flomptoid', 'quorbleflat']), ('engine', 'other'), ('reviewClosed', False)):
            receipt = self.visual_review()
            receipt[field] = value
            result = self.result(receipt)
            self.assertTrue(result['workflowPassed'])
            self.assertFalse(result['qualificationPending'])
            self.assertFalse(result['passed'])
            self.assertTrue(result['failures'])

    def test_review_report_or_capture_identity_mismatch_rejected(self):
        receipt = self.visual_review()
        receipt['reportSha256'] = '0' * 64
        receipt['captures']['desktop-review-open.png'] = '1' * 64
        self.assertFalse(self.result(receipt)['passed'])

    def test_malformed_structural_report_cannot_pass_with_review(self):
        self.report['desktopSupplementObservations'] = None
        self.assertTrue(self.result(self.visual_review())['failures'])


if __name__ == '__main__':
    unittest.main()
