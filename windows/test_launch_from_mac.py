import base64
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('mac_launcher', Path(__file__).resolve().parents[1] / 'bin/windows/launch-from-mac.py')
launcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(launcher)

class MacLauncherTests(unittest.TestCase):
    def test_shared_home_path_and_outside_rejection(self):
        with tempfile.TemporaryDirectory() as temp:
            home = Path(temp).resolve()
            self.assertEqual(launcher.guest_path(home / 'project with spaces/a&b.md', home), r'C:\Mac\Home\project with spaces\a&b.md')
            with self.assertRaisesRegex(ValueError, 'shared Mac Home'):
                launcher.guest_path(home.parent, home)

    def test_powershell_quote_preserves_apostrophe(self):
        self.assertEqual(launcher.ps_string("C:\\Andrew's notes\\a.md"), "'C:\\Andrew''s notes\\a.md'")

    def test_dirty_source_is_never_fetched_or_replaced(self):
        with patch.object(launcher.subprocess, 'check_output', side_effect=['windows/port\n', ' M writing.md\n']), patch.object(launcher, 'run') as run:
            launcher.sync_source()
            run.assert_not_called()

    def test_clean_source_only_fast_forwards(self):
        with patch.object(launcher.subprocess, 'check_output', side_effect=['windows/port\n', '']), patch.object(launcher, 'run') as run:
            launcher.sync_source()
            self.assertEqual(run.call_args_list[1].args[0], ['git', 'merge', '--ff-only', 'FETCH_HEAD'])

    def test_offline_update_never_fetches_source(self):
        with patch.object(launcher, 'sync_source') as sync, patch.object(launcher, 'run') as run:
            launcher.update_packages(True)
            sync.assert_not_called()
            self.assertEqual(run.call_args.args[0][-1], '--offline')

    def test_running_vm_is_not_restarted_and_payload_is_encoded(self):
        listing = json.dumps([{'name': 'Windows 11', 'status': 'running'}])
        with patch.object(launcher, 'guest_path', return_value=r'C:\Mac\Home\project with spaces'), patch.object(launcher.subprocess, 'check_output', return_value=listing), patch.object(launcher.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0)), patch.object(launcher, 'run') as run:
            launcher.launch('Windows 11')
            commands = [call.args[0] for call in run.call_args_list]
            self.assertFalse(any(c[:2] == ['prlctl', 'start'] for c in commands))
            command = commands[0]
            decoded = base64.b64decode(command[-1]).decode('utf-16le')
            self.assertIn("-RepositoryRoot 'C:\\Mac\\Home\\project with spaces'", decoded)
            self.assertEqual(command[-2], '-EncodedCommand')

    def test_unknown_launch_outcome_is_not_replayed(self):
        listing = json.dumps([{'name': 'Windows 11', 'status': 'running'}])
        with patch.object(launcher, 'guest_path', return_value=r'C:\Mac\Home\project'), patch.object(launcher.subprocess, 'check_output', return_value=listing), patch.object(launcher.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0)), patch.object(launcher, 'run', side_effect=subprocess.TimeoutExpired('prlctl', 300)) as run:
            with self.assertRaises(subprocess.TimeoutExpired):
                launcher.launch('Windows 11')
            self.assertEqual(run.call_count, 1)

if __name__ == '__main__':
    unittest.main()
