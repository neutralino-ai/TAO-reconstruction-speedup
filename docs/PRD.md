# TAO Reconstruction Speedup — PRD

## 1. Context
TAO reconstruction via TAOSW. Reference: omilrecv2 (JUNO OMILREC, 1525→189 ms/evt, 8.1x).

## 2. Goal
≥1.5x single-thread speedup, no resolution degradation.

## 3. Phase Overview

| Step | What | Deliverable |
|------|------|-------------|
| **1**: Test bed | Build + extract FCN + 2 golden tests | E2E test + FCN fixture test both pass |
| **2**: Profile | perf/flamegraph/phase-timing | Hotspot ranking |
| **3**: Optimize | One commit per opt, measure speed+drift | Speed improves, both tests still pass |
| **4**: Benchmark | 100-event timing, speed.csv | ms/evt per commit |
| **5**: Iterate | 2→3→4 loop | ≥1.5x reached |
| **6**: Acceptance | Resolution vs pimin baseline | No degradation |

---

## 4. Step 1: Test Bed

```
1.1 Build         → libRecQMLEAlg.so + libQMLEFCN.a
1.2 Refactor      → extract QMLERec::QMLE() as free function ComputeQMLE()
1.3 E2E test      → golden reference: ESD → reco vertex (x,y,z,E)
1.4 FCN fixture   → golden fixture:  calibrated hits → FCN value (-2lnL)
1.5 Gate          → both 1.3 and 1.4 must pass before any optimization
```

1.2 (Refactor) must complete before 1.4 (FCN fixture), because the fixture test calls the FCN as a standalone free function.

### 1.1 Build
Build `libRecQMLEAlg.so` and `libQMLEFCN.a` against CVMFS J26.1.0 + pimin's TAOSW overlay.
**Status**: ✅

### 1.2 Refactor — Extract FCN as Free Function
Extract `QMLERec::QMLE()` → `ComputeQMLE(input, Evis, vr, vtheta, vphi)`.
**Input**: `QMLEInput { fChannelHit[8048], channels[8048], charge_template_ge68, GeEvis, PDE, ESF, saturation }`
**Output**: `double` (-2 ln L). No SNiPER dependency.
**Status**: ✅

### 1.3 E2E Test: ESD → Reco Vertex
**Input**: ESD file. **Output**: reco vertex (x,y,z,E) per event.
**Golden**: `reference/ref_5evt.root` (pimin's original, 2025-05-28, immutable).
**Test**: `tests/test_consistency.py` — TTree::Scan comparison, must be tree-identical.
**Status**: ✅

### 1.4 FCN Fixture: Calibrated Hits → FCN Value
**Input**: QMLEInput + fit parameters (Evis, vr, vtheta, vphi) at initial guess.
**Output**: double (-2 ln L).
**Golden**: `fixtures/fcn_evt0.txt` — captured from pimin's **unmodified** code.
**Test**: `build/bin/test_qmle_fcn` — load fixture, call ComputeQMLE(), compare to golden. Tolerance ≤1e-13.
**Status**: ❌

### 1.5 Gate: Both Tests Must Pass
Before any optimization: `test_consistency.py` PASS + `test_qmle_fcn` PASS.
**Status**: ❌ (blocked by 1.4)

---

## 5. Step 2: Profile

### 5.1 Phase Timing
Add `TStopwatch` to `QMLERec::execute()` for: load/calibration, charge center, minimizer, output.

### 5.2 Flamegraph
```
perf record -g -- python RecQMLEAlg/share/run.py --evtmax 100 ...
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

### 5.3 Hotspot Ranking
Top 10 functions by self-time from `perf report`.

**Status**: ❌ (partial `perf stat` only in A3)

---

## 6. Step 3: Optimize

Rules:
- One optimization per commit
- Run 1.5 gate after each commit
- Speed regression ≥5% → rollback; FCN drift >1e-13 → rollback

Categories: hoist invariants, data layout (SoA), algebraic (exp/log removal, fast math), SIMD, algorithmic.

**Status**: ⚠️ Optimizations written (A4-A6) but not gated by 1.5.

---

## 7. Step 4: Benchmark

- 100 events minimum (10 warmup), single thread, Release build
- Measure: Vertex Fit Time from per-event `TStopwatch`
- Append to `benchmarks/speed.csv`: commit, ms/evt, date
- Speed gate: CI fails if ≥5% regression vs main

**Status**: ❌

---

## 8. Step 5: Iterate

Loop Step 2 → 3 → 4 until ≥1.5x speedup reached.

**Status**: ❌

---

## 9. Step 6: Acceptance

- Run ≥1000 events through both pimin baseline and optimized code
- Compare energy resolution, vertex resolution
- No statistically significant degradation

**Status**: ❌

---

## Current Status

| Sub-step | Status |
|----------|--------|
| 1.1 Build | ✅ |
| 1.2 Refactor | ✅ |
| 1.3 E2E test | ✅ |
| 1.4 FCN fixture | ❌ |
| 1.5 Gate | ❌ |
| 2 Profile | ❌ |
| 3 Optimize | ⚠️ |
| 4 Benchmark | ❌ |
| 5 Iterate | ❌ |
| 6 Acceptance | ❌ |
