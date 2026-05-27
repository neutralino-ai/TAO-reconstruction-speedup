# TAO Reconstruction Speedup — PRD

## 1. Context

TAO (Taishan Antineutrino Observatory) runs reconstruction via `Elec2Rec.py` in TAOSW, processing rtraw → ESD on the IHEP DCI grid. The reconstruction pipeline is the dominant computing cost in TAO production campaigns (currently TaoProd26A, TAOSW T25.7.2).

**Reference project**: [`omilrecv2`](https://code.ihep.ac.cn/neutrino-physics/dr-sai-juno/omilrecv2) — the JUNO OMILREC vertex/energy reconstruction algorithm was optimized from 1525 → 189 ms/evt (8.1x) using a disciplined iterate-profile-validate workflow.

## 2. Goal

Achieve **≥1.5x single-thread speedup** on TAO reconstruction while maintaining acceptable reconstruction resolution (defined in acceptance test, §8).

## 3. Approach

Follow the omilrecv2 optimization playbook adapted to TAO:

| Phase | What | Success Criterion |
|-------|------|-------------------|
| **Step 1**: Test bed | Build TAOSW standalone package + reference output + auto tests | Bit-identical or tolerance-bounded reproduction of reference reco |
| **Step 2**: Refactor | Extract hot-path code into free functions, flatten data layouts | All tests still pass; code is profiler-friendly |
| **Step 3**: Profile | perf/flamegraph, phase timing, roofline analysis | Clear bottleneck ranking (which functions/phases dominate) |
| **Step 4**: Optimize | Apply targeted optimizations, one commit at a time | Each commit passes tests; speed improves or stays flat |
| **Step 5**: Unit test | FCN-level golden-value regression tests | ≤1e-13 relative drift (or physics-based tolerance) |
| **Step 6**: Benchmark | 100-event wall-clock timing | Speed gate: ≥5% regression → reject commit |
| **Step 7**: Iterate | Go back to Step 3 until goal met | Progressive speedup; no rollbacks accumulate |
| **Step 8**: Acceptance | Resolution comparison vs reference on physics metrics | No degradation beyond physics tolerance |

## 4. Step Details

### 4.1 Test Bed

**Input**: TAO rtraw data files (run range 1295–1500), calibration DB (CondDB with Global Tag T25.7.1-c), bad channel list.

**Deliverables**:
- Standalone build of the reconstruction code (TAOSW package extracted, built against CVMFS J25.7.2)
- Reference output file: N events reconstructed with unmodified code, stored as `reference/ref_Nevt.root`
- **Consistency test**: run N events, compare all output fields against reference. Tolerance: bit-identical initially; may relax to physics tolerance after algebraic optimizations.
- **Memory test**: ASan-instrumented build, 10 events, no errors.
- Benchmark script: measures wall-clock ms/evt (single thread, Release build, 100 events).

**Reference from omilrecv2**: `tests/test_consistency.py`, `tests/test_memory.py`, `scripts/benchmark.sh`.

### 4.2 Refactor

Extract computational kernels from algorithm classes into free functions. The goal is to make profiling and unit-testing possible without running the full SNiPER/TAOSW framework.

From omilrecv2 experience, key refactoring patterns:
- Extract likelihood computation into pure free functions (take data arrays, return double)
- Flatten PMT/SiPM data from AoS to SoA (contiguous arrays → SIMD-friendly, cache-friendly)
- Separate I/O (EDM reading/writing) from computation
- Remove dead code paths (compile-time constants, unused branches)

**Files to target** (to be identified from TAOSW source):
- The reconstruction algorithm (analogous to `OMILRECV2.cc`)
- Calibration service (analogous to `OMILRECCalibSvc.cc`)
- Helper/geometry functions

### 4.3 Profile

**Tools**: `perf record` + FlameGraph, `perf stat` (cache misses, branches, instructions), phase-level timing instrumentation.

**Deliverables**:
- Phase breakdown: load / vertex / fit phase 1 / fit phase 2 / ... / output (ms/evt each)
- Hotspot ranking: top 10 functions by self-time
- Roofline: is the code compute-bound or memory-bound?
- Optimization opportunities ranked by estimated gain

Reference: omilrecv2's phase timing table in `benchmarks/RESULTS.md`.

### 4.4 Optimize

Optimization categories (from omilrecv2 history):

| Category | Examples | Typical gain |
|----------|---------|--------------|
| **Hoist invariants** | Move loop-invariant computation out of Minuit FCN; precompute per-event constants once | 10-30% |
| **Data layout** | SoA arrays, raw pointers instead of virtual calls, bulk memcpy | 15-50% |
| **Bulk/SIMD** | Split loops into homogeneous vectorizable passes (cos, sqrt, etc.) | 30-100% |
| **Algorithmic** | Reduce iteration count (spatial indexing, early termination, better initial guess) | 20-40% |
| **Algebraic** | Replace exp+log with log1p, pow(x,3)→x*x*x, reciprocal precompute | 5-15% |
| **Fast-path** | Inline common-case paths, skip unnecessary computation for ineligible elements | 15-25% |

**Rules**:
- One optimization per commit
- Measure drift and speed after each commit
- If speed regresses ≥5%: rollback
- If drift exceeds tolerance: rollback or apply `[drift-ok]` marker with justification

### 4.5 Unit Test (FCN-level)

Extract the core fit function (the one Minuit calls) as a pure free function. Capture its inputs (calibration data, per-event arrays, trial point) and golden outputs from the reference version as binary fixtures.

**Test**: Load fixtures, call the free function, compare result to golden value. Tolerance: `1e-13` relative (FP noise floor) for bit-exact refactors; physics-based tolerance for algebraic changes.

This catches FP regressions from innocent-looking changes (e.g., `float` vs `double` in an intermediate, Kahan summation reordering).

Reference: `omilrecv2/tests/unit/test_fcn.cc`, `tests/fixtures/v107_rev1/`.

### 4.6 Benchmark

- 100 events, single thread, Release build
- Record: ms/evt, wall-clock total, peak RSS, CPU model
- Append to `benchmarks/speed.csv`
- Compare HEAD vs HEAD~1: ≥5% regression is a warning; `[speed-gate]` in commit message makes it a hard fail

Reference: `omilrecv2/scripts/quick_bench.sh`, `benchmarks/speed.csv`.

### 4.7 Iterate

After each optimization cycle (Steps 3→4→5→6):
- If speed improved and tests pass: commit, continue
- If speed unchanged: document why (e.g., bottleneck shifted), continue
- If speed regressed: rollback, document the failed experiment in `benchmarks/FAILED.md`

Stop when: ≥1.5x speedup achieved and verified.

### 4.8 Acceptance Test

Run N events (≥1000) through both reference and optimized reconstruction. Compare physics resolution metrics:

- Energy resolution
- Vertex/position resolution
- Any other physics-critical output

**Tolerance**: No statistically significant degradation vs reference. Exact metrics TBD based on TAO physics requirements.

Reference: omilrecv2 uses an 8-metric ACU Ge-68 gate comparison (via `/omilrec-acceptance` skill).

## 5. Repository Structure (Planned)

```
TAO-reconstruction-speedup/
├── README.md
├── PRD.md                          # This file
├── CLAUDE.md                       # Claude Code context
├── CMakeLists.txt                  # Build against CVMFS TAOSW
├── TAOREC/                         # Reconstruction package (extracted from TAOSW)
│   ├── CMakeLists.txt
│   └── src/                        # Algorithm source + extracted free functions
├── tests/
│   ├── conftest.py
│   ├── test_consistency.py         # E2E consistency vs reference
│   ├── test_memory.py              # ASan memory safety
│   └── unit/                       # C++ unit tests (FCN-level, no framework)
├── scripts/
│   ├── build.sh
│   ├── benchmark.sh
│   └── generate_reference.sh
├── benchmarks/
│   ├── speed.csv                   # Per-commit speed tracking
│   ├── drift.csv                   # Per-commit FCN drift tracking
│   └── RESULTS.md                  # Human-readable results table
├── reference/
│   └── ref_Nevt.root               # Frozen reference output
├── docs/
│   └── profiling/                  # Profiling reports per iteration
└── fixtures/                       # Binary fixtures for FCN unit tests
```

## 6. TAO-Specific Considerations

### Differences from JUNO OMILREC

| Aspect | JUNO OMILREC | TAO Reconstruction |
|--------|-------------|-------------------|
| Detector | Central detector (~17k large PMTs + dynode/MCP) | TAO detector (SiPM arrays) |
| Software | JUNOSW J26.1.1 | TAOSW T25.7.2 |
| Reconstruction | `tut_rtraw2rec.py` → OMILREC algorithm | `Elec2Rec.py` → TAO reco chain |
| Calibration | nPE maps, time PDFs, charge PDFs from OMILRECCalibSvc | CondDB with GTag, SiPMCalibAlg |
| Output | RecVertex (15 fields) | ESD (to be inspected) |
| Current speed | 1525 ms/evt baseline | Unknown (to be measured) |

### Unknowns (to resolve in Step 1)

- What is the dominant algorithm/phase in TAO reconstruction? (Is there a Minuit-based fitter? A different approach?)
- What are the hot loops? (PMT/SiPM loops? Bin interpolation?)
- What is the current ms/evt?
- What are the key output fields to validate?

## 7. Success Criteria

| Criterion | Threshold |
|-----------|-----------|
| Speedup | ≥1.5x single-thread vs baseline |
| Consistency test | Pass (bit-identical or within physics tolerance) |
| Memory safety | ASan clean |
| FCN drift | ≤1e-13 relative (or documented physics tolerance) |
| Acceptance resolution | No degradation beyond physics tolerance |
| Speed gate | No commit regresses ≥5% without explicit override |

## 8. Timeline (TBD after Step 1 profiling)

Rough estimate based on omilrecv2 experience:
- Step 1 (Test bed): 1-2 sessions
- Step 2 (Refactor): 1-2 sessions
- Step 3 (Profile): 1 session
- Steps 4-7 (Optimize + iterate): 3-6 sessions per 20% speedup
- Step 8 (Acceptance): 1 session

Total: ~8-15 sessions for 1.5x. Heavily depends on how much low-hanging fruit exists.