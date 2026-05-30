#!/bin/bash
# Benchmark: measure per-event reconstruction time (excludes CondDB init).
#
# Runs 110 events: first 10 warmup (CondDB), measures last 100.
# Uses SNiPER profiling if available, otherwise wall-clock difference.

set -euo pipefail
PROJ="$(cd "$(dirname "$0")/.." && pwd)"
N_TOTAL="${1:-110}"
N_WARMUP=10

source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J26.1.0/setup.sh 2>/dev/null
source /scratchfs/juno/pimin01/taosw_latest/taosw/setup.sh 2>/dev/null
export RECQMLEALGROOT="$PROJ/RecQMLEAlg"

INPUT=$(ls /tmp/tao_test_*/RUN.1427*.rec.root 2>/dev/null | head -1)
if [ -z "$INPUT" ]; then
    echo "No cached ESD file. Fetching..."
    FILES=$($PROJ/scripts/env.sh bash -c "eos ls /eos/juno/tao-reprod/TaoPP26A/mix_stream/00001000/00001400_CalibData/1427" 2>/dev/null)
    ESD=$(echo "$FILES" | grep ".001_T25" | head -1)
    TMPDIR=$(mktemp -d -t tao_bench_XXXX)
    $PROJ/scripts/env.sh bash -c "eos cp /eos/juno/tao-reprod/TaoPP26A/mix_stream/00001000/00001400_CalibData/1427/$ESD $TMPDIR/"
    INPUT="$TMPDIR/$ESD"
fi

echo "Benchmark: $N_TOTAL events (warmup=$N_WARMUP)"
echo "Input: $INPUT"

# --- BASELINE (pimin build) ---
echo ""
echo "=== BASELINE (pimin build) ==="
START=$(date +%s.%N)
python "$PROJ/RecQMLEAlg/share/run.py" \
    --evtmax $N_TOTAL --use_true_vertex false \
    --input "$INPUT" --output /tmp/bench_pimin.root \
    --charge_template_file charge_template --loglevel WARN 2>&1 \
    | grep "Vertex Fit Time" > /tmp/pimin_vft.txt
END=$(date +%s.%N)
WALL=$(echo "$END - $START" | bc)

# Extract Vertex Fit Times (minimizer only)
python3 -c "
times = []
with open('/tmp/pimin_vft.txt') as f:
    for l in f:
        times.append(float(l.strip().split()[-2]))
n = len(times)
warmup = min(10, n-1)
meas = times[warmup:]  # skip warmup
print(f'  Events recorded: {n}')
print(f'  Vertex Fit (minimizer only): mean={sum(meas)/len(meas)*1000:.3f} ms/evt  min={min(meas)*1000:.3f}  max={max(meas)*1000:.3f}')
print(f'  Wall clock (incl init): {float('"$WALL"'):.1f}s  →  {float('"$WALL"')*1000/n:.1f} ms/evt')
"

# --- OUR BUILD (minimal overlay) ---
echo ""
echo "=== OUR BUILD ==="
OVERLAY=$(mktemp -d -t tao_overlay_XXXX)
mkdir -p "$OVERLAY/lib64"
cp "$PROJ/InstallArea/lib64/libRecQMLEAlg.so" "$OVERLAY/lib64/"
START=$(date +%s.%N)
CMAKE_PREFIX_PATH="$OVERLAY:$CMAKE_PREFIX_PATH" \
    python "$PROJ/RecQMLEAlg/share/run.py" \
    --evtmax $N_TOTAL --use_true_vertex false \
    --input "$INPUT" --output /tmp/bench_ours.root \
    --charge_template_file charge_template --loglevel WARN 2>&1 \
    | grep "Vertex Fit Time" > /tmp/ours_vft.txt
END=$(date +%s.%N)
WALL=$(echo "$END - $START" | bc)

python3 -c "
times = []
with open('/tmp/ours_vft.txt') as f:
    for l in f:
        times.append(float(l.strip().split()[-2]))
n = len(times)
warmup = min(10, n-1)
meas = times[warmup:]
print(f'  Events recorded: {n}')
print(f'  Vertex Fit (minimizer only): mean={sum(meas)/len(meas)*1000:.3f} ms/evt  min={min(meas)*1000:.3f}  max={max(meas)*1000:.3f}')
print(f'  Wall clock (incl init): {float('"$WALL"'):.1f}s  →  {float('"$WALL"')*1000/n:.1f} ms/evt')
"

rm -rf "$OVERLAY"
echo ""
echo "Done."
