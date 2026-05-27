#!/bin/bash
# Benchmark: measure QMLE reconstruction wall-clock time per event.
#
# Usage:
#   bash scripts/benchmark.sh [N=100] [run_id=1427] [file_id=001]
#
# Output: benchmarks/speed.csv (appended)

set -euo pipefail

PROJ="$(cd "$(dirname "$0")/.." && pwd)"
ENV_SH="$PROJ/scripts/env.sh"
N_EVENTS="${1:-100}"
RUN_ID="${2:-1427}"
FILE_ID="${3:-001}"

INPUT_DIR="/eos/juno/tao-reprod/TaoPP26A/mix_stream/00001000/00001400_CalibData/${RUN_ID}"

# Find input file
ESD_FILE=$("$ENV_SH" bash -c "eos ls $INPUT_DIR" 2>/dev/null \
    | grep "RUN.${RUN_ID}.*${FILE_ID}" | head -1 | tr -d '\r')
if [ -z "$ESD_FILE" ]; then
    echo "ERROR: No input file found at $INPUT_DIR"
    exit 1
fi

TMPDIR=$(mktemp -d -t tao_bench_XXXX)
echo "Copying $ESD_FILE from EOS..."
"$ENV_SH" bash -c "eos cp $INPUT_DIR/$ESD_FILE $TMPDIR/" || { echo "EOS copy failed"; exit 1; }

INPUT_FILE="$TMPDIR/$ESD_FILE"
OUTPUT_FILE="$TMPDIR/output.root"

echo "Running benchmark: $N_EVENTS events..."
START=$(date +%s.%N)

"$ENV_SH" bash -c "
    python \"$PROJ/RecQMLEAlg/share/run.py\" \
        --evtmax $N_EVENTS \
        --use_true_vertex false \
        --input \"$INPUT_FILE\" \
        --output \"$OUTPUT_FILE\" \
        --charge_template_file charge_template \
        --loglevel WARN
"

END=$(date +%s.%N)
ELAPSED=$(echo "$END - $START" | bc)
MS_PER_EVT=$(echo "scale=2; $ELAPSED * 1000 / $N_EVENTS" | bc)
CPU="$(lscpu | grep 'Model name' | cut -d: -f2 | xargs)"

echo ""
echo "================================================"
echo "  Events:     $N_EVENTS"
echo "  Wall clock: ${ELAPSED}s"
echo "  ms/evt:     $MS_PER_EVT"
echo "  CPU:        $CPU"
echo "================================================"

# Append to speed.csv
COMMIT=$(git -C "$PROJ" rev-parse --short HEAD 2>/dev/null || echo "unknown")
DATE=$(date +%Y-%m-%d)
MACHINE_TAG=$(echo "$CPU" | tr ' ' '_' | tr -d '()@')
mkdir -p "$PROJ/benchmarks"
echo "$COMMIT,$MS_PER_EVT,$MACHINE_TAG,$N_EVENTS,$DATE" >> "$PROJ/benchmarks/speed.csv"
echo "Recorded: benchmarks/speed.csv"

rm -rf "$TMPDIR"