#!/usr/bin/env python3
"""Offline Review/Center evidence verification; never launches an application."""
import argparse
import copy
import importlib.util
import json
import math
from pathlib import Path, PureWindowsPath
import re
import struct
import sys

ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('desktop_normal_contract', ROOT / 'check-normal-launch.py')
NORMAL = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(NORMAL)
SOURCES = ('run-desktop-supplement-guest.ps1', 'run-normal-launch-guest.ps1', 'normal-launch-input.cs',
           'desktop-supplement-steps.ps1', 'desktop-supplement-input.cs')
CHECKS = ('open-path', 'palette', 'review-toggle', 'next-issue', 'previous-issue', 'center')
STAGES = (*NORMAL.STAGES[:4], 'desktop-supplement-workflow', *NORMAL.STAGES[4:])
FIXTURE = '<!-- Unicode witness: café 😀 -->\n\nA flomptoid waits.\n\nWe recieve the mail.\n\nA quorbleflat waits.\n'.encode()
CAPTURES = tuple('desktop-' + name for name in CHECKS) + (
    'desktop-review-open', 'desktop-review-open-initial', 'desktop-review-open-middle', 'desktop-open-path-open',
    'desktop-palette-open', 'desktop-review-open-path-complete', 'desktop-automatic-before', 'desktop-automatic-off-reopened')
GEOMETRY = ('left', 'top', 'width', 'height', 'visibleLeft', 'visibleTop', 'visibleWidth', 'visibleHeight',
            'workLeft', 'workTop', 'workWidth', 'workHeight', 'monitor', 'dpi')


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def text(path):
    return path.read_bytes().decode('utf-8-sig')


def composed(root=ROOT):
    body = text(root / 'run-normal-launch-guest.ps1')
    config = "$ErrorActionPreference = 'Stop'"
    hook = "    Capture 'live-mode-rendered'"
    report = '        viewObservations = $viewObservations'
    gate = "gate = 'windows-normal-launch'"
    passed = 'passed = ($null -eq $failure)'
    startup = "    Passed $stage 'Deployed executable launched without command-line arguments and acknowledged isolated profile.'"
    for anchor in (config, hook, report, gate, passed, startup):
        if body.count(anchor) != 1:
            raise ValueError('Maintained composition anchor differs: ' + anchor)
    parts = text(root / 'desktop-supplement-steps.ps1').split('# DESKTOP-SUPPLEMENT-WORKFLOW')
    if len(parts) != 2:
        raise ValueError('Supplement startup/workflow marker differs')
    body = body.replace(config, config + '\n$script:DesktopSupplementObservation=[ordered]@{completed=$false;qualified=$false;nativeVisualReviewPending=$true}\n')
    body = body.replace(startup, parts[0] + '\n' + startup).replace(hook, hook + '\n' + parts[1])
    body = body.replace(report, report + '\n        desktopSupplementObservations = $script:DesktopSupplementObservation')
    body = body.replace(gate, "gate = 'windows-desktop-supplement'").replace('normal-launch-report.json', 'desktop-supplement-report.json')
    body = body.replace(passed, 'passed = $false; workflowPassed = ($null -eq $failure -and $script:DesktopSupplementObservation.completed); qualified = $false; qualificationPending = $true')
    body = body.replace('Normal-launch Windows input gate passed; independently verify', 'Desktop supplemental workflows completed; independently review')
    return {'run-normal-launch-guest.ps1': body.encode(),
            'normal-launch-input.cs': (text(root / 'normal-launch-input.cs') + '\n' + text(root / 'desktop-supplement-input.cs')).encode()}


def number(value):
    try:
        return type(value) in (int, float) and math.isfinite(value)
    except OverflowError:
        return False


def seconds(first, last):
    a, at = NORMAL.parse_timestamp(first)
    b, bt = NORMAL.parse_timestamp(last)
    return (b - a).total_seconds() + (bt - at) / 10000000


def verify(report, package, evidence, expected_executable, source_root=ROOT, review=None, report_path=None):
    """Valid workflow without independent visual review remains pending."""
    errors = []
    def need(condition, message):
        if not condition:
            errors.append(message)
    try:
        need(report.get('failure') is None, 'Raw report retains a failure')
        need(report['passed'] is False and report['qualified'] is False and report['qualificationPending'] is True
             and report['workflowPassed'] is True, 'Raw workflow flags must retain pending qualification')
        normalized = copy.deepcopy(report)
        normalized['passed'] = True  # Memory only, after checking unchanged raw flags.
        drivers = evidence.parent / 'driver'
        errors.extend(NORMAL.verify(normalized, package, evidence, expected_executable, drivers,
                                    gate_id='windows-desktop-supplement', stages=STAGES))
        manifest = read_json(package / 'manifest.json')
        composition = read_json(evidence / 'driver-composition.json')
        need(composition == read_json(drivers / 'driver-composition.json'), 'Copied composition differs')
        need(composition['schemaVersion'] == 1 and composition['preparedOnly'] is False
             and composition['nativeParsePassed'] is True and composition['helperCompilePending'] is True,
             'Execution composition differs from maintained contract')
        need(composition['sourceSha256'] == {n: NORMAL.sha(source_root / n) for n in SOURCES}, 'Maintained source identity differs')
        for name, contents in composed(source_root).items():
            need((drivers / name).read_bytes() == contents, 'Generated driver differs: ' + name)
        need(composition['generatedDriverSha256'] == NORMAL.sha(drivers / 'run-normal-launch-guest.ps1')
             and composition['generatedInputSha256'] == NORMAL.sha(drivers / 'normal-launch-input.cs'), 'Generated composition hash differs')
        need(bool(re.fullmatch('[0-9a-f]{40}', composition['commit'])) and composition['architecture'] in ('arm64', 'x64')
             and composition['commit'] == manifest['commit'] == manifest['nativeBuild']['commit']
             and composition['architecture'] == manifest['architecture'] and manifest['trackedSourceDirty'] is False,
             'Exact clean package commit or architecture differs')
        for field in ('manifestSha256', 'executableSha256'):
            need(composition[field] == report[field], 'Composition package binding differs: ' + field)
        for field, name in (('backendSourceSha256', 'src/backend.cpp'), ('menuSourceSha256', 'src/WindowsMenuBar.qml')):
            need(bool(re.fullmatch('[0-9a-f]{64}', composition[field]))
                 and composition[field] == manifest['nativeBuild']['sourceFiles'][name], 'Native source identity differs: ' + name)
        need(composition['localStageReceipt'] == 'local-stage-receipt.json', 'Unexpected staged receipt name')
        receipt = read_json(evidence / 'local-stage-receipt.json')
        need((drivers / 'local-stage-receipt.json').read_bytes() == (evidence / 'local-stage-receipt.json').read_bytes()
             and NORMAL.sha(evidence / 'local-stage-receipt.json') == composition['localStageReceiptSha256'], 'StageOnly receipt bytes/hash differ')
        need(receipt['stageOnly'] is True and all(receipt[k] == composition[k] for k in
             ('commit', 'architecture', 'manifestSha256', 'executableSha256'))
             and NORMAL.same_executable(str(PureWindowsPath(receipt['localPackage']) / 'Fomawrite.exe'), expected_executable),
             'StageOnly receipt does not bind exact local executable')
        need(composition['clipboardTouched'] is False, 'Supplement touched clipboard')
        hashes = report['evidenceSha256']
        required = {'driver-composition.json', 'local-stage-receipt.json', 'desktop-supplement-observations.json',
                    'center-native-observations.json', 'desktop-supplement-updater-setup.json'}
        required |= {n + suffix for n in CAPTURES for suffix in ('.png', '-uia.txt')}
        need(required.issubset(hashes), 'Required supplemental evidence missing from hash index')
        for name in hashes:
            need(not PureWindowsPath(name).is_absolute() and '..' not in PureWindowsPath(name).parts, 'Unsafe Windows evidence path: ' + name)
        for name in CAPTURES:
            data = (evidence / (name + '.png')).read_bytes()
            need(len(data) >= 24 and data[:8] == b'\x89PNG\r\n\x1a\n' and data[12:16] == b'IHDR'
                 and all(100 <= v <= 65535 for v in struct.unpack('>II', data[16:24])), 'Invalid capture dimensions: ' + name)
            need(bool(text(evidence / (name + '-uia.txt')).strip()), 'Empty UIA capture: ' + name)
        need((evidence / 'writing-normal-launch/desktop-public-review.md').read_bytes() == FIXTURE, 'Exact UTF-8 Review fixture differs')
        obs = read_json(evidence / 'desktop-supplement-observations.json')
        need(obs == report['desktopSupplementObservations'], 'Serialized observations differ')
        need(obs['schemaVersion'] == 1 and obs['completed'] is True and obs['qualified'] is False
             and obs['nativeVisualReviewPending'] is True and obs['visualReviewRequired'] is True
             and obs['clipboardTouched'] is False and obs['failure'] is None and obs['binding'] == composition,
             'Supplement incomplete or bypasses visual qualification')
        updater = read_json(evidence / 'desktop-supplement-updater-setup.json')
        need(updater == obs['updaterSetup'] and updater['completed'] is True and updater['initialChecked'] is True
             and updater['reopenedChecked'] is False and updater['failure'] is None
             and number(updater['offElapsedMs']) and 0 <= updater['offElapsedMs'] < 9000, 'Updater prerequisite invalid or late')
        checks = obs['checks']
        need([c['id'] for c in checks] == list(CHECKS), 'Require six ordered unique checks')
        first = NORMAL.parse_timestamp(report['checks'][3]['observedUtc'])
        finish = NORMAL.parse_timestamp(report['checks'][4]['observedUtc'])
        previous = first
        pid = report['launches'][0]['processId']
        def selection(value, wanted):
            return (value['processId'] == pid and value['rangeCount'] == 1 and value['keyboardFocus'] is True
                    and value['text'] == wanted and first <= NORMAL.parse_timestamp(value['observedUtc']) <= finish)
        for check in checks:
            instant = NORMAL.parse_timestamp(check['observedUtc'])
            need(check['passed'] is True and check['capture'] == 'desktop-' + check['id'] + '.png'
                 and previous <= instant <= finish, 'Failed, stale or misbound check: ' + check['id'])
            previous = instant
        details = {c['id']: c['detail'] for c in checks}
        for name, chord in (('open-path', 'Ctrl+Shift+O'), ('palette', 'Ctrl+Shift+P')):
            d = details[name]
            need(d['physicalKeys'] == chord and d['opened'] is True and d['canceled'] is True, 'Physical cancellation incomplete: ' + name)
        palette = details['palette']['ownedQuery']
        need(palette['processId'] == pid and palette['present'] is True and palette['ready'] is True
             and palette['keyboardFocus'] is True and palette['hwnd'] > 0 and palette['queryAutomationId'].endswith('.commandQuery')
             and NORMAL.parse_timestamp(checks[0]['observedUtc']) <= NORMAL.parse_timestamp(palette['observedUtc']) <= NORMAL.parse_timestamp(checks[1]['observedUtc']), 'Owned focused Palette missing')
        d = details['review-toggle']
        need(d['physicalKeys'] == 'Ctrl+Shift+OEM_1' and d['openingChordSent'] is True and d['closingChordSent'] is True
             and d['selectedText'] == d['closedSelectedText'] == '' and d['visualReviewPending'] is True
             and d['visibleCardCount'] is None and d['engineObserved'] is None
             and d['openCapture'] == 'desktop-review-open.png' and d['closeCapture'] == 'desktop-review-toggle.png', 'Review toggle/pending state differs')
        for name, chord, wanted in (('next-issue', 'Ctrl+OEM_1', 'flomptoid'), ('previous-issue', 'Ctrl+Alt+OEM_1', 'quorbleflat')):
            d = details[name]
            need(d['physicalKeys'] == chord and selection(d['selection'], wanted)
                 and NORMAL.parse_timestamp(checks[CHECKS.index(name)-1]['observedUtc']) <= NORMAL.parse_timestamp(d['selection']['observedUtc']) <= NORMAL.parse_timestamp(checks[CHECKS.index(name)]['observedUtc']), 'Focused owned issue selection differs: ' + name)
        center = read_json(evidence / 'center-native-observations.json')
        d = details['center']
        need(center['assertionPassed'] is True and center['failure'] is None and center['initial'] == d['initial']
             and center['before'] == d['before'] and d['movementObserved'] is True and selection(d['selection'], 'quorbleflat'), 'Center assertion/selection binding differs')
        initial, before, after = d['initial'], d['before'], d['after']
        samples = center['afterSamples']
        def geometry(w):
            return (all(number(w[k]) for k in GEOMETRY) and all(w[k] > 0 for k in
                    ('width', 'height', 'visibleWidth', 'visibleHeight', 'workWidth', 'workHeight', 'dpi'))
                    and w['visibleFrameHresult'] == 0 and type(w['processId']) is int and w['processId'] == pid
                    and type(w['hwnd']) is int and 0 < w['hwnd'] <= 2**63-1 and w['monitor'] > 0
                    and w['minimized'] is False and w['maximized'] is False and w['keyboardLayout'] == '04090409'
                    and first <= NORMAL.parse_timestamp(w['observedUtc']) <= finish)
        need(len(samples) >= 3 and all(geometry(w) for w in (initial, before, after, *samples)), 'Valid owned DWM geometry/DPI or samples missing')
        need(NORMAL.parse_timestamp(after['observedUtc']) <= NORMAL.parse_timestamp(d['selection']['observedUtc']) <= NORMAL.parse_timestamp(checks[-1]['observedUtc']), 'Center selection witness is stale or follows its check')
        need(after == samples[-1] and all(w['hwnd'] == initial['hwnd'] and w['monitor'] == initial['monitor'] for w in (before, after, *samples))
             and palette['hwnd'] == initial['hwnd'], 'Center changed owned window or monitor')
        need(all(initial[k] == before[k] == after[k] for k in ('width', 'height', 'visibleWidth', 'visibleHeight'))
             and (initial['left'], initial['top']) != (before['left'], before['top'])
             and (before['left'], before['top']) != (after['left'], after['top']), 'Center failed movement or size preservation')
        times = [NORMAL.parse_timestamp(w['observedUtc']) for w in samples]
        need(times == sorted(times) and NORMAL.parse_timestamp(initial['observedUtc']) <= NORMAL.parse_timestamp(before['observedUtc']) <= times[0], 'Center samples reordered')
        stable = []
        for w in reversed(samples):
            if tuple(w[k] for k in GEOMETRY) != tuple(after[k] for k in GEOMETRY):
                break
            stable.insert(0, w)
        need(len(stable) >= 3 and seconds(stable[0]['observedUtc'], stable[-1]['observedUtc']) >= .3, 'Center lacks three stable observations over 300ms')
        need(abs(after['visibleLeft'] + after['visibleWidth']/2 - after['workLeft'] - after['workWidth']/2) <= 3
             and abs(after['visibleTop'] + after['visibleHeight']/2 - after['workTop'] - after['workHeight']/2) <= 3, 'DWM frame exceeds three-pixel center allowance')
        need(type(center['stableSamples']) is int and center['stableSamples'] == len(stable) and center['stableSamples'] >= 3
             and number(center['stableIntervalMs']) and center['stableIntervalMs'] >= 300, 'Center stability receipt invalid')
    except (OSError, KeyError, TypeError, ValueError, AttributeError, IndexError, OverflowError, struct.error) as error:
        errors.append('Incomplete or invalid supplemental evidence: ' + str(error))
    workflow = not errors
    if review is not None:
        try:
            path = report_path or evidence / 'desktop-supplement-report.json'
            need(read_json(path) == report, 'Visual review binds a different raw report')
            need(review['schemaVersion'] == 1 and review['gate'] == 'windows-desktop-supplement-visual-review'
                 and bool(review['reviewedBy'].strip()) and review['reportSha256'] == NORMAL.sha(path)
                 and NORMAL.parse_timestamp(review['reviewedUtc']) >= NORMAL.parse_timestamp(report['finishedUtc']), 'Visual review report identity/reviewer invalid')
            names = ('desktop-review-open.png', 'desktop-review-toggle.png')
            need(review['captures'] == {n: NORMAL.sha(evidence / n) for n in names}, 'Visual review capture identity differs')
            need(review['engine'] == 'Harper 2.10.0 (built-in)' and review['cards'] == ['flomptoid', 'recieve', 'quorbleflat']
                 and review['reviewClosed'] is True, 'Visual review lacks engine, three exact cards or closed Review')
        except (OSError, KeyError, TypeError, ValueError, AttributeError) as error:
            errors.append('Incomplete or invalid visual review: ' + str(error))
    passed = workflow and review is not None and not errors
    return {'scope': 'review-center-supplement', 'fullM2Acceptance': False, 'passed': passed,
            'workflowPassed': workflow, 'qualificationPending': workflow and review is None, 'failures': errors}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--verify-report', type=Path, required=True)
    parser.add_argument('--expected-executable', required=True)
    parser.add_argument('--visual-review', type=Path)
    parser.add_argument('--source-root', type=Path, default=ROOT,
                        help='Exact archived driver source directory; bytes and line endings remain binding')
    args = parser.parse_args()
    if not NORMAL.same_executable(args.expected_executable, args.expected_executable):
        parser.error('--expected-executable must be an absolute Windows path without parent traversal')
    path = args.verify_report.resolve()
    try:
        result = verify(read_json(path), args.package.resolve(), path.parent, args.expected_executable,
                        source_root=args.source_root.resolve(),
                        review=read_json(args.visual_review) if args.visual_review else None, report_path=path)
        result.update(report=str(path), reportSha256=NORMAL.sha(path))
    except (OSError, ValueError, TypeError) as error:
        result = {'scope': 'review-center-supplement', 'fullM2Acceptance': False, 'passed': False,
                  'workflowPassed': False, 'qualificationPending': False, 'failures': [str(error)]}
    (path.parent / 'desktop-supplement-verification.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    return 0 if result['passed'] else 2 if result['qualificationPending'] else 1


if __name__ == '__main__':
    sys.exit(main())
