#!/bin/bash
# Benchmark several builds of the opendaq Python module against each other.
#
# Usage: run_variants.sh <bin dir> <results dir> <rounds> <label>=<module.so> [<label>=<module.so> ...]
#
# Each variant's module is copied over the one in <bin dir> in turn, so all variants share
# the same core libraries and device modules. Rounds are interleaved to spread out drift.
# The original module is restored at the end.
# Environment: PYTHON (interpreter, default python3), BUDGET (seconds per measurement, default 2).
set -euo pipefail

if [ $# -lt 4 ]; then
    sed -n '2,9p' "$0"
    exit 1
fi

BIN=$(realpath "$1")
RESULTS=$(realpath -m "$2")
ROUNDS=$3
shift 3
HERE=$(dirname "$(realpath "$0")")
PYTHON=${PYTHON:-python3}
BUDGET=${BUDGET:-2}

MODULE=$(ls "$BIN"/opendaq.cpython-*.so)
BACKUP=$(mktemp)
cp "$MODULE" "$BACKUP"
trap 'cp "$BACKUP" "$MODULE"; rm -f "$BACKUP"' EXIT

mkdir -p "$RESULTS"
cd "$BIN"

for variant in "$@"; do
    label=${variant%%=*}
    cp "${variant#*=}" "$MODULE"
    PYTHONPATH="$BIN:$HERE" "$PYTHON" "$HERE/count_releases.py" --label "$label" --out "$RESULTS/$label-releases.json"
done

for round in $(seq "$ROUNDS"); do
    for variant in "$@"; do
        label=${variant%%=*}
        cp "${variant#*=}" "$MODULE"
        # openDAQ logs to stdout; keep only the benchmark's lines
        PYTHONPATH="$BIN:$HERE" "$PYTHON" "$HERE/bench_time.py" --label "$label" --out "$RESULTS/$label-time-$round.json" --budget "$BUDGET" \
            2>/dev/null | grep -E "^$label "
    done
done

"$PYTHON" "$HERE/summarize.py" "$RESULTS" --baseline "${1%%=*}"
