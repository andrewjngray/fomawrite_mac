#!/bin/bash
export PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH"
cd "$(dirname "$0")" || exit 1
python3 bin/windows/launch-from-mac.py  "$@"
result=$?
if [ "$result" -ne 0 ]; then
  printf "\nThe operation did not finish. Existing builds and documents were kept.\n"
fi
if [ -t 0 ]; then read -r -p "Press Return to close this window."; fi
exit "$result"
