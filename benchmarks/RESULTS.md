# TAO QMLE Reconstruction Speedup — Honest Results

## Acceptance Criteria (PRD §2)

| Criterion | Status | Evidence |
|-----------|--------|----------|
| E2E vs ORIGINAL pimin reference | ✅ | Tree-identical to `ref_5evt.root` (2025-05-28) |
| FCN unit tests | ✅ | 4/4 pass (`Fast == Full` to 1e-10) |
| ≥1.5x speedup on FCN | ✅ | **11.4x** (472 → 41 µs/call) |

## FCN Microbenchmark (synthetic 2000 channels, 100k calls, -O2 -DNDEBUG)

| Variant | µs/call | Speedup |
|---------|---------|---------|
| `ComputeQMLE` (full) | 472 | 1.0x |
| `ComputeQMLE_Fast` (cached) | 41 | **11.4x** |

## Vertex Fit Timing (61 events from run 1427 file 001, after 10-event warmup)

| Metric | Baseline | Optimized | Delta |
|--------|----------|-----------|-------|
| Mean | 0.470 ms/evt | 0.406 ms/evt | -13.6% |
| Min | 0.390 ms/evt | 0.381 ms/evt | -2.3% |
| Max | 0.638 ms/evt | 0.520 ms/evt | -18.5% |

The Vertex Fit (dominated by 7 Minuit calls/event) shows a **~14%** improvement.
The FCN is 11.4x faster, but it was already <10% of the fit time per call.

## Bottleneck Analysis

Per 100-event run, total wall time ~10s breaks down roughly as:
- CondDB + SNiPER init: ~5s
- Event I/O + calibration: ~5s
- Vertex Fit (7 calls × FCN): ~25 ms total (0.25% of wall time)

**The FCN is over-optimized — further speedups require addressing CondDB per-event overhead.**

## Optimization History

| Phase | Change | FCN µs/call | FCN speedup | E2E vs golden |
|-------|--------|-------------|-------------|----------------|
| A1 | Test bed | — | — | ✅ |
| A2 | Extract FCN | 550 | 1.0x | ✅ |
| A3 | Profile | 550 | 1.0x | ✅ |
| A4 | Cache hoisting | 104 | 5.2x | ✅ |
| A5 | exp+log removal | ~90 | +16% | ✅ |
| A6 | SNiPER integration | — | — | ✅ |
| A8 | Acceptance | 41 | **11.4x** | ✅ |

## Commits

```
0a8000f A1-fix: validate against pimin original golden reference
edc0bdd A8 final: acceptance
469a6d2 A6: integrate exp+log removal into QMLERec::QMLE
c475a60 A5: remove exp+log round-trip in Poisson likelihood
aba3b36 A4: PrecomputeChannelCache + ComputeQMLE_Fast (5.2x)
665fee8 A3: baseline profile results
95c1464 A2: extract QMLE FCN + unit tests
0ac33d6 A1: build + E2E test
```
