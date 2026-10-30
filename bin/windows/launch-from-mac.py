#!/usr/bin/env python3
"""Refresh Windows test builds and open them in Parallels from the Mac."""
import argparse
import base64
import json
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]

def ps_string(value):
    return "'" + str(value).replace("'", "''") + "'"

def guest_path(path, home=None):
    path = Path(path).resolve()
    try:
        relative = path.relative_to(Path.home() if home is None else home)
    except ValueError as error:
        raise ValueError('The repo and documents must be under your shared Mac Home folder.') from error
    return 'C:\\Mac\\Home\\' + str(relative).replace('/', '\\')

def run(command, **kwargs):
    return subprocess.run(command, check=True, **kwargs)

def sync_source():
    branch = subprocess.check_output(['git', 'branch', '--show-current'], cwd=ROOT, text=True).strip()
    if branch != 'windows/port':
        raise ValueError('Open Launch.command from the windows/port checkout.')
    dirty = subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT, text=True).strip()
    if dirty:
        print('Local source edits are present; preserving them. Checking packaged builds independently.', flush=True)
        return
    run(['git', 'fetch', 'origin', 'windows/port'], cwd=ROOT, timeout=60)
    run(['git', 'merge', '--ff-only', 'FETCH_HEAD'], cwd=ROOT, timeout=30)

def update_packages(offline=False):
    helper = [sys.executable, str(ROOT / 'bin/windows/update-testing.py')]
    if offline:
        run(helper + ['--offline'])
        return
    try:
        sync_source()
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print('Source update unavailable; retaining the checkout: ' + str(error), flush=True)
    run(helper)

def launch(vm, document=None):
    repo = guest_path(ROOT)
    script = "$ErrorActionPreference='Stop'; & ([ScriptBlock]::Create([IO.File]::ReadAllText(" + ps_string(repo + '\\bin\\windows\\launch-current.ps1') + '))) -RepositoryRoot ' + ps_string(repo)
    if document:
        path = Path(document).expanduser().resolve()
        if not path.is_file():
            raise ValueError('Document does not exist: ' + str(path))
        script += ' -DocumentPath ' + ps_string(guest_path(path))
    encoded = base64.b64encode(script.encode('utf-16le')).decode('ascii')
    # Starting a running VM is unnecessary; never stop/restart a user's session.
    listing = json.loads(subprocess.check_output(['prlctl', 'list', '-a', '-j'], text=True, timeout=30))
    target = next((entry for entry in listing if entry.get('name') == vm or entry.get('uuid') == vm), None)
    if target is None:
        raise ValueError('Parallels VM not found: ' + vm)
    if target.get('status') != 'running':
        run(['prlctl', 'start', vm], timeout=180)
    # Poll readiness only. Never replay an app launch if its outcome is unknown.
    deadline = time.monotonic() + 120
    while True:
        ready = subprocess.run(['prlctl', 'exec', vm, '--current-user', 'cmd.exe', '/c', 'exit', '0'],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=20)
        if ready.returncode == 0:
            break
        if time.monotonic() >= deadline:
            raise ValueError('Windows is not ready. Sign in, then click Launch.command again.')
        time.sleep(3)
    run(['prlctl', 'exec', vm, '--current-user', 'powershell.exe', '-NoProfile', '-OutputFormat', 'Text', '-EncodedCommand', encoded], timeout=300)
    run(['open', '-a', 'Parallels Desktop'])

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--update-only', action='store_true')
    parser.add_argument('--offline', action='store_true')
    parser.add_argument('--vm', default=os.environ.get('FOMAWRITE_VM', 'Windows 11'))
    parser.add_argument('document', nargs='?')
    args = parser.parse_args()
    try:
        try:
            update_packages(args.offline)
        except (OSError, ValueError, subprocess.SubprocessError):
            if args.update_only or args.offline:
                raise
            print('Online update unavailable. Verifying the last downloaded build before launch.', flush=True)
            update_packages(True)
        if not args.update_only:
            launch(args.vm, args.document)
        return 0
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print('Fomawrite Windows: ' + str(error), file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())
