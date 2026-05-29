#!/bin/bash
# Profile the QMLE reconstruction.
#
# Usage:
#   bash scripts/profile.sh [N=1000]
#
# Output:
#   - Terminal: perf stat summary (cycles, instructions, cache misses, ms/evt)
#   - benchmarks/perf_YYYYMMDD.data: raw perf data for flamegraph
#   - benchmarks/perf_YYYYMMDD.txt : perf stat report
#   - benchmarks/speed.csv: updated with latest ms/evt

set -euo pipefail

PROJ="$(cd "$(dirname "$0")/.." && pwd)"
N_FCN_CALLS="${1:-1000}"

source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J26.1.0/setup.sh 2>/dev/null
source /scratchfs/juno/pimin01/taosw_latest/taosw/setup.sh 2>/dev/null
source "$PROJ/InstallArea/setup.sh" 2>/dev/null || true

DATE=$(date +%Y%m%d_%H%M)
mkdir -p "$PROJ/benchmarks"

# --- 1. FCN microbenchmark ---
echo "=== FCN Microbenchmark ==="
echo "Running $N_FCN_CALLS calls..."

"$PROJ/build/bin/test_qmle_fcn.exe" 2>/dev/null  # verify it works

# Write a quick C++ microbenchmark
cat > /tmp/fcn_bench.cc << 'EOF'
#include "RecQMLEAlg/QMLEFCN.h"
#include "RecQMLEAlg/ChargeTemplate.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv) {
    int N = (argc > 1) ? atoi(argv[1]) : 1000;

    // Load template
    setenv("RECQMLEALGROOT", getenv("RECQMLEALGROOT") ? getenv("RECQMLEALGROOT") : ".", 1);
    ChargeTemplate ct("Ge68_charge_template");
    ct.initialize();

    // Build QMLEInput
    QMLEInput inp;
    memset(&inp, 0, sizeof(inp));
    inp.channels.resize(8048);
    inp.charge_template_ge68 = &ct;
    inp.GeEvis = 0.9250;
    inp.PDE = 1.0;
    inp.ESF = 1.0;
    inp.saturation = 1e5;
    // Enable first 1000 channels with some data
    for (int i = 0; i < 1000; ++i) {
        inp.channels[i].bad = false;
        inp.channels[i].pos = TVector3(
            900 * cos(i * 0.1),
            900 * sin(i * 0.1),
            200.0);
        inp.channels[i].rPDE = 0.8;
        inp.channels[i].dcr = 0.001;
        inp.fChannelHit[i] = (i % 5 == 0) ? 0 : (i % 3) * 1.5;
    }
    // Mark rest bad
    for (int i = 1000; i < 8048; ++i) inp.channels[i].bad = true;

    // Test points (energy, radius, theta, phi)
    double pts[][4] = {
        {0.5, 100.0, 0.5, 0.0},
        {1.0, 300.0, 1.0, 1.5},
        {2.0, 800.0, 2.0, 3.0},
        {0.8, 200.0, 0.8, 1.0},
        {1.5, 500.0, 1.2, 2.0},
    };
    const int NPTS = 5;

    // Warmup
    for (int p = 0; p < NPTS; ++p)
        ComputeQMLE(inp, pts[p][0], pts[p][1], pts[p][2], pts[p][3]);

    auto t1 = std::chrono::high_resolution_clock::now();
    long total_calls = 0;
    while (total_calls < N) {
        for (int p = 0; p < NPTS && total_calls < N; ++p) {
            ComputeQMLE(inp, pts[p][0], pts[p][1], pts[p][2], pts[p][3]);
            ++total_calls;
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();

    printf("  FCN calls: %ld\n", total_calls);
    printf("  Time:      %.1f ms\n", ms);
    printf("  us/call:   %.3f\n", ms * 1000.0 / total_calls);
    return 0;
}
EOF

# Compile and run the microbenchmark
cd "$PROJ"
g++ -std=c++17 -O2 -DNDEBUG /tmp/fcn_bench.cc \
    -I"$PROJ" \
    -I/scratchfs/juno/pimin01/taosw_latest/taosw/InstallArea/include \
    -I$ROOTSYS/include \
    -L"$PROJ/build/RecQMLEAlg" \
    -lQMLEFCN \
    $(root-config --libs --cflags) \
    -o /tmp/fcn_bench.exe 2>/dev/null

if [ -x /tmp/fcn_bench.exe ]; then
    RECQMLEALGROOT="$PROJ/RecQMLEAlg" /tmp/fcn_bench.exe "$N_FCN_CALLS" | tee "$PROJ/benchmarks/fcn_bench_${DATE}.txt"
    US_PER_CALL=$(grep "us/call" "$PROJ/benchmarks/fcn_bench_${DATE}.txt" | awk '{print $NF}')
    echo "$US_PER_CALL us/call FCN baseline"
else
    echo "  SKIP: microbenchmark compilation failed (may need ROOT env)"
fi

# --- 2. perf stat on full reconstruction ---
echo ""
echo "=== Full Reconstruction (perf stat) ==="
echo "Running E2E (10 events) with perf stat..."

bash "$PROJ/scripts/env.sh" bash -c "
    cd $PROJ
    export RECQMLEALGROOT=$PROJ/RecQMLEAlg
    export JUNOTOP=/cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J26.1.0
    source InstallArea/setup.sh
    perf stat -e cycles,instructions,cache-references,cache-misses,branches,branch-misses,task-clock \
        python RecQMLEAlg/share/run.py \
            --evtmax 10 \
            --use_true_vertex false \
            --input $TEMP/tao_perf_input.root \
            --output $TEMP/tao_perf_output.root \
            --charge_template_file charge_template \
            --loglevel WARN \
        2>&1 | tee $PROJ/benchmarks/perf_stat_${DATE}.txt
" || echo "  perf stat failed (may not have perf, or input file issue)"

echo ""
echo "Done. Results in benchmarks/"
ls "$PROJ/benchmarks/"