#!/bin/sh
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# On macOS, backdating an older modification time can also move birth time.
# Set both back, then advance only modification time for a visible contrast.
touch -t 202501020304 "$here/sample/First.md"
touch -t 202606150304 "$here/sample/First.md"
