"""Offline installer trust regressions; no Qt download or extraction is performed."""
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import ssl
import tempfile
import unittest
from unittest.mock import patch
import urllib.error

SPEC = importlib.util.spec_from_file_location('qt_installer', Path(__file__).resolve().parents[1] / 'bin/windows/install-qt.py')
INSTALLER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(INSTALLER)
ORIGIN = 'https://download.qt.io/online/qtsdkrepository/windows_arm64/desktop/qt6_6113/kit/qtbase.7z'
PAYLOAD = b'controlled pinned archive fixture'


class Response(io.BytesIO):
    def __init__(self, data=PAYLOAD, url=ORIGIN):
        super().__init__(data)
        self.url = url

    def geturl(self):
        return self.url


def http(code, url=ORIGIN):
    return urllib.error.HTTPError(url, code, 'fixture', {}, None)


class QtDownloadTrustTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.archive = self.root / 'archive.7z'
        self.entry = {'url': ORIGIN, 'sha1': hashlib.sha1(PAYLOAD).hexdigest(),
                      'package': 'fixture', 'packageVersion': '6.11.3-0-fixed'}
        self.mirrors = [m + ORIGIN.removeprefix('https://download.qt.io') for m in INSTALLER.QT_MIRRORS]

    def download(self, responses):
        with patch.object(INSTALLER.urllib.request, 'urlopen', side_effect=responses) as request:
            url = INSTALLER.download_archive(self.entry, self.archive)
        return url, [c.args[0] for c in request.call_args_list]

    def test_origin_success_records_actual_response_url(self):
        actual = 'https://download.qt.io/exact-response'
        url, calls = self.download([Response(url=actual)])
        self.assertEqual((url, calls), (actual, [ORIGIN]))
        self.assertEqual(self.archive.read_bytes(), PAYLOAD)

    def test_original_404_uses_fixed_mirror_exact_path_and_original_pin(self):
        before = dict(self.entry)
        url, calls = self.download([http(404), Response(url=self.mirrors[0])])
        self.assertEqual(calls, [ORIGIN, self.mirrors[0]])
        self.assertEqual(url, self.mirrors[0])
        self.assertEqual(self.entry, before)
        self.assertEqual(hashlib.sha1(self.archive.read_bytes()).hexdigest(), before['sha1'])

    def test_second_fixed_mirror_only_after_first_mirror_404(self):
        url, calls = self.download([http(404), http(404), Response(url=self.mirrors[1])])
        self.assertEqual(calls, [ORIGIN, *self.mirrors[:2]])
        self.assertEqual(url, self.mirrors[1])

    def test_all_404_exhausts_all_fixed_mirrors(self):
        candidates = [ORIGIN, *self.mirrors]
        with patch.object(INSTALLER.urllib.request, 'urlopen', side_effect=[http(404)] * len(candidates)) as request:
            with self.assertRaises(urllib.error.HTTPError):
                INSTALLER.download_archive(self.entry, self.archive)
        self.assertEqual([c.args[0] for c in request.call_args_list], candidates)

    def test_verified_additional_mirrors_preserve_exact_path_and_pin(self):
        roots = ('https://ftp.jaist.ac.jp/pub/qtproject', 'https://ftp.nluug.nl/languages/qt')
        for index, root in enumerate(roots, start=3):
            target = root + ORIGIN.removeprefix('https://download.qt.io')
            before = dict(self.entry)
            with self.subTest(root=root):
                url, calls = self.download([http(404)] * index + [Response(url=target)])
                self.assertEqual(calls, [ORIGIN, *self.mirrors[:index]])
                self.assertEqual(calls[-1], target)
                self.assertEqual(url, target)
                self.assertEqual(self.entry, before)
                self.assertEqual(hashlib.sha1(self.archive.read_bytes()).hexdigest(), before['sha1'])

    def test_additional_mirror_errors_stop_without_further_fallback(self):
        for index in (3, 4):
            for error in (http(403), http(429), http(503), ssl.SSLCertVerificationError('fixture'), urllib.error.URLError('fixture')):
                with self.subTest(candidate=index, error=type(error).__name__), patch.object(
                        INSTALLER.urllib.request, 'urlopen', side_effect=[http(404)] * index + [error]) as request:
                    with self.assertRaises(type(error)) as raised:
                        INSTALLER.download_archive(self.entry, self.archive)
                    self.assertIs(raised.exception, error)
                    self.assertEqual(request.call_count, index + 1)

    def test_other_origin_http_errors_do_not_fallback(self):
        for code in (401, 403, 429, 500, 503):
            with self.subTest(code=code), patch.object(INSTALLER.urllib.request, 'urlopen', side_effect=http(code)) as request:
                with self.assertRaises(urllib.error.HTTPError):
                    INSTALLER.download_archive(self.entry, self.archive)
                self.assertEqual(request.call_count, 1)

    def test_origin_tls_and_network_errors_do_not_fallback(self):
        for error in (ssl.SSLCertVerificationError('fixture'), urllib.error.URLError('fixture')):
            with self.subTest(error=type(error).__name__), patch.object(INSTALLER.urllib.request, 'urlopen', side_effect=error) as request:
                with self.assertRaises(type(error)):
                    INSTALLER.download_archive(self.entry, self.archive)
                self.assertEqual(request.call_count, 1)

    def test_mirror_non404_error_does_not_try_second_mirror(self):
        for error in (http(503), ssl.SSLCertVerificationError('fixture'), urllib.error.URLError('fixture')):
            with self.subTest(error=type(error).__name__), patch.object(INSTALLER.urllib.request, 'urlopen', side_effect=[http(404), error]) as request:
                with self.assertRaises(type(error)):
                    INSTALLER.download_archive(self.entry, self.archive)
                self.assertEqual(request.call_count, 2)

    def test_checksum_mismatch_never_falls_back(self):
        for index in range(len(INSTALLER.QT_MIRRORS) + 1):
            prefix = [http(404)] * index
            with self.subTest(candidate=index), patch.object(INSTALLER.urllib.request, 'urlopen', side_effect=prefix + [Response(b'wrong revision')]) as request:
                with self.assertRaisesRegex(SystemExit, 'checksum mismatch'):
                    INSTALLER.download_archive(self.entry, self.archive)
                self.assertEqual(request.call_count, len(prefix) + 1)

    def test_invalid_origin_is_rejected_before_network(self):
        for url in ('http://download.qt.io/online/qtsdkrepository/kit',
                    ORIGIN.replace('download.qt.io', 'download.qt.io.example.org'),
                    ORIGIN + '?revision=new', ORIGIN + '#replacement'):
            self.entry['url'] = url
            with self.subTest(url=url), patch.object(INSTALLER.urllib.request, 'urlopen') as request:
                with self.assertRaisesRegex(SystemExit, 'Unexpected archive origin'):
                    INSTALLER.download_archive(self.entry, self.archive)
                request.assert_not_called()

    def test_invalid_pin_is_rejected_before_network(self):
        self.entry['sha1'] = 'not-a-pinned-hash'
        with patch.object(INSTALLER.urllib.request, 'urlopen') as request:
            with self.assertRaisesRegex(SystemExit, 'Invalid pinned Qt checksum'):
                INSTALLER.download_archive(self.entry, self.archive)
            request.assert_not_called()

    def test_install_records_acquisition_url_without_changing_locked_url(self):
        output = self.root / 'sdk'
        lock = self.root / 'lock.json'
        lock.write_text(json.dumps({'qtVersion': '6.11.3', 'architectures': {'arm64': [self.entry]}}))
        required = ('bin/windeployqt.exe', 'bin/Qt6WebEngineCore.dll', 'bin/Qt6Pdf.dll',
                    'lib/Qt6WebEngineQuick.lib', 'qml/QtQuick/Controls/qmldir')

        def extraction(*args, **kwargs):
            for name in required:
                file = output / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_bytes(b'fixture')

        query = f'QT_VERSION:6.11.3\nQT_INSTALL_PREFIX:{output}\n'
        with patch.object(INSTALLER.urllib.request, 'urlopen', side_effect=[http(404), Response(url=self.mirrors[0])]), \
                patch.object(INSTALLER.subprocess, 'run', side_effect=extraction), \
                patch.object(INSTALLER.subprocess, 'check_output', return_value=query):
            INSTALLER.install(lock, 'arm64', output, 'controlled-extraction-stub')
        installed = json.loads((output / 'fomawrite-qt-install.json').read_text())['archives'][0]
        self.assertEqual(installed['url'], ORIGIN)
        self.assertEqual(installed['acquisitionUrl'], self.mirrors[0])
        self.assertEqual(installed['sha1'], self.entry['sha1'])
        self.assertEqual(installed['sha256'], hashlib.sha256(PAYLOAD).hexdigest())

    def test_checksum_mismatch_prevents_extraction(self):
        lock = self.root / 'lock.json'
        lock.write_text(json.dumps({'qtVersion': '6.11.3', 'architectures': {'arm64': [self.entry]}}))
        with patch.object(INSTALLER.urllib.request, 'urlopen', return_value=Response(b'wrong revision')), \
                patch.object(INSTALLER.subprocess, 'run') as extract:
            with self.assertRaisesRegex(SystemExit, 'checksum mismatch'):
                INSTALLER.install(lock, 'arm64', self.root / 'sdk', 'never-extract')
            extract.assert_not_called()


if __name__ == '__main__':
    unittest.main()
