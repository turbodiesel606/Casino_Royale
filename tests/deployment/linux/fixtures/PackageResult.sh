#!/usr/bin/env bash
set -euo pipefail
bash "$(dirname -- "$0")/RealPackage.sh" "$@"
python3 "$FIXTURE_ROOT/corrupt-result.py" "$2/result.txt" "$FIXTURE_RESULT"
