"""Offline rejection tests only; fixture reports are not Windows UI acceptance."""
import base64
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import unittest

from windows import test_normal_launch as normal_fixtures

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('live_edit_gate', ROOT / 'bin/windows/check-live-edit.py')
GATE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GATE)


class LiveEditEvidenceTests(unittest.TestCase):
    def make_case(self, case):
        fixture = normal_fixtures.NormalLaunchEvidenceTests('test_complete_evidence_is_accepted')
        fixture.setUp()
        self.addCleanup(fixture.doCleanups)
        evidence, report = fixture.output, fixture.report
        report['gate'] = 'windows-live-edit'
        report['checks'] = [{'name': stage, 'passed': True,
                             'observedUtc': f'2026-10-22T00:00:{second:09.6f}Z'}
                            for stage, second in zip(GATE.STAGES, (.01, .02, .03, .04, 1.5, 1.6, 1.8))]
        # Each case must also prove its own output-derived profile namespace.
        profile = f'C:/Acceptance/{case}/evidence/profile'
        receipt_path = evidence / 'profile/profile.json'
        receipt = json.loads(receipt_path.read_text())
        previous = receipt['applicationName']
        namespace = 'FomawriteAcceptance-' + hashlib.sha256(profile.encode()).hexdigest()[:20]
        receipt['profileRoot'] = profile
        receipt['applicationName'] = namespace
        for name in ('settingsPath', 'dataPath', 'cachePath', 'workspacePath'):
            receipt[name] = receipt[name].replace(previous, namespace)
        receipt_path.write_text(json.dumps(receipt))
        report['profileRoot'] = profile
        report['outputRoot'] = f'C:/Acceptance/{case}/evidence'
        report['profileReceiptSha256'] = GATE.NORMAL.sha(receipt_path)
        sample = evidence / 'writing-normal-launch/normal-launch-sample.md'
        sample.write_bytes(GATE.EDITED)
        report['sampleSha256'] = GATE.NORMAL.sha(sample)
        driver = evidence.parent / 'driver'
        driver.mkdir()
        for name, body in GATE.composed(GATE.ROOT, case).items():
            (driver / name).write_bytes(body)
        report['driverSha256'] = GATE.NORMAL.sha(driver / 'run-normal-launch-guest.ps1')
        report['inputHelperSha256'] = GATE.NORMAL.sha(driver / 'normal-launch-input.cs')
        composition = {'schemaVersion': 1, 'case': case, 'requestedGapMs': GATE.CASES[case],
                       'sourceSha256': {n: GATE.NORMAL.sha(GATE.ROOT / n) for n in GATE.SOURCES}}
        (evidence / 'driver-composition.json').write_text(json.dumps(composition))
        for name in ('live-text-focus', 'live-text-entered', 'live-edit-workflow', *GATE.CHECKPOINTS):
            (evidence / (name + '.png')).write_bytes(b'\x89PNG\r\n\x1a\nfixture pixels')
            (evidence / (name + '-uia.txt')).write_text('fixture visible owned control')
        encode = lambda value: base64.b64encode(value).decode()
        def state(wanted, second):
            return {'title': 'normal-launch-sample.md - Fomawrite', 'dirty': False,
                    'diskBase64': encode(wanted), 'observedUtc': f'2026-10-22T00:00:{second:09.6f}Z'}
        checkpoints = []
        for name, wanted, start in zip(GATE.CHECKPOINTS, (GATE.EDITED, GATE.ORIGINAL, GATE.EDITED), (.1, .7, 1.1)):
            (evidence / (name + '.md')).write_bytes(wanted)
            checkpoints.append({'name': name, 'snapshot': name + '.md',
                                'sha256': GATE.NORMAL.sha(evidence / (name + '.md')),
                                'stableStates': [state(wanted, start + shift) for shift in (0, .15, .3)],
                                'state': state(wanted, start + .31)})
        timing = None if case == 'settled' else {
            'requestedGapMs': GATE.CASES[case], 'frequency': 1000000, 'undoBegin': 1000, 'undoEnd': 1100,
            'saveBegin': 1100 + GATE.CASES[case] * 1000 + 10,
            'saveEnd': 1300 + GATE.CASES[case] * 1000, 'observedUtc': '2026-10-22T00:00:00.42Z'}
        observation = {'schemaVersion': 1, 'case': case, 'requestedGapMs': GATE.CASES[case], 'completed': True,
                       'failure': None, 'stableSamples': 3, 'originalBase64': encode(GATE.ORIGINAL),
                       'editedBase64': encode(GATE.EDITED), 'checkpoints': checkpoints, 'rapidTiming': timing,
                       'focus': {'processId': 1, 'keyboardFocus': True, 'offscreen': False, 'controlType': 'ControlType.Edit'},
                       'target': {'word': {'text': 'Saved', 'left': 10, 'top': 100, 'width': 50, 'height': 20},
                                  'region': {'left': 0, 'top': 0, 'width': 800, 'height': 300}, 'x': 35, 'y': 110}}
        fixture.observation = observation
        self.save_observation(fixture)
        for path in evidence.iterdir():
            if path.is_file():
                report['evidenceSha256'][path.name] = GATE.NORMAL.sha(path)
        return fixture

    def setUp(self):
        self.fixture = self.make_case('immediate')

    def save_observation(self, fixture=None):
        fixture = fixture or self.fixture
        path = fixture.output / 'live-edit-observations.json'
        path.write_text(json.dumps(fixture.observation))
        fixture.report['evidenceSha256'][path.name] = GATE.NORMAL.sha(path)

    def failures(self):
        f = self.fixture
        return GATE.verify_case(f.report, f.package, f.output, f.exe, 'immediate')

    def test_complete_contract_is_accepted(self):
        self.assertEqual([], self.failures())

    def test_original_instead_of_edited_save_is_rejected_even_if_rehashed(self):
        f = self.fixture
        path = f.output / 'live-typed-saved.md'
        path.write_bytes(GATE.ORIGINAL)
        f.report['evidenceSha256'][path.name] = GATE.NORMAL.sha(path)
        f.observation['checkpoints'][0]['sha256'] = GATE.NORMAL.sha(path)
        self.save_observation()
        self.assertTrue(self.failures())

    def test_dirty_state_cannot_pass_with_correct_bytes(self):
        self.fixture.observation['checkpoints'][1]['stableStates'][1]['dirty'] = True
        self.save_observation()
        self.assertTrue(self.failures())

    def test_unsaved_title_cannot_pass_with_false_dirty_claim(self):
        self.fixture.observation['checkpoints'][1]['state']['title'] = '* normal-launch-sample.md - Fomawrite'
        self.save_observation()
        self.assertTrue(self.failures())

    def test_instant_polling_cannot_claim_stability(self):
        points = self.fixture.observation['checkpoints'][0]['stableStates']
        for point in points:
            point['observedUtc'] = points[0]['observedUtc']
        self.save_observation()
        self.assertTrue(self.failures())

    def test_ocr_target_cannot_be_outside_writing_pane(self):
        self.fixture.observation['target']['x'] = 900
        self.save_observation()
        self.assertTrue(self.failures())

    def test_focus_cannot_belong_to_another_process(self):
        self.fixture.observation['focus']['processId'] = 999
        self.save_observation()
        self.assertTrue(self.failures())

    def test_settled_wait_cannot_satisfy_immediate_case(self):
        timing = self.fixture.observation['rapidTiming']
        timing['saveBegin'] += 100000
        timing['saveEnd'] += 100000
        self.save_observation()
        self.assertTrue(self.failures())

    def test_missing_rapid_timing_is_rejected(self):
        self.fixture.observation['rapidTiming'] = None
        self.save_observation()
        self.assertTrue(self.failures())

    def test_modified_generated_driver_rejected_even_if_report_rehashed(self):
        f = self.fixture
        path = f.output.parent / 'driver/run-normal-launch-guest.ps1'
        path.write_text('fabricated pass')
        f.report['driverSha256'] = GATE.NORMAL.sha(path)
        self.assertTrue(self.failures())

    def test_failed_live_stage_cannot_pass(self):
        self.fixture.report['checks'][4]['passed'] = False
        self.assertTrue(self.failures())

    def test_only_settled_or_missing_gap_cases_cannot_qualify_series(self):
        series = {'schemaVersion': 1, 'gate': 'windows-live-edit-series', 'passed': True, 'allCases': False, 'cases': []}
        self.assertTrue(GATE.verify_series(series, self.fixture.package, self.fixture.output, self.fixture.exe))

    def test_all_five_cases_are_required_and_accepted(self):
        output = self.fixture.root / 'series'
        output.mkdir()
        entries = []
        for case, gap in GATE.CASES.items():
            f = self.make_case(case)
            destination = output / case
            destination.mkdir()
            shutil.copytree(f.output, destination / 'evidence')
            shutil.copytree(f.output.parent / 'driver', destination / 'driver')
            path = destination / 'evidence/live-edit-report.json'
            path.write_text(json.dumps(f.report))
            entries.append({'case': case, 'requestedGapMs': gap, 'passed': True, 'exitCode': 0,
                            'report': case + '/evidence/live-edit-report.json', 'reportSha256': GATE.NORMAL.sha(path)})
        series = {'schemaVersion': 1, 'gate': 'windows-live-edit-series', 'passed': True, 'allCases': True, 'cases': entries}
        self.assertEqual([], GATE.verify_series(series, self.fixture.package, output, self.fixture.exe))
        series['cases'][2] = copy.deepcopy(series['cases'][1])
        self.assertTrue(GATE.verify_series(series, self.fixture.package, output, self.fixture.exe))


if __name__ == '__main__':
    unittest.main()
