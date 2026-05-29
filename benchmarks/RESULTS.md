# TAO QMLE Reconstruction Speedup Results

## Full Reconstruction Timing (AlmaLinux, Intel Xeon)

| Build | ms/evt (500 evt) | Speedup |
|-------|------------------|---------|
| Pimin baseline | 1.50 | 1.0x |
| Our optimized | 0.43 | **3.4x** |

Measured with `LD_PRELOAD` override — both builds use same SNiPER/CondDB stack, only `libRecQMLEAlg.so` differs.

## FCN Microbenchmark

| Variant | µs/call | Speedup |
|---------|---------|---------|
| Original `ComputeQMLE` | 472 | 1.0x |
| `ComputeQMLE_Fast` (cached) | 41 | **11.4x** |

2000 active channels, single vertex, 100k calls.

## perf stat (baseline, 100 events)

| Metric | Value |
|--------|-------|
| IPC | 1.48 |
| Cache misses | 15.9% |
| Branch misses | 0.53% |
| Task clock | 9.31 s |

## Optimization History

| Phase | Change | FCN µs/call | Speedup | E2E |
|-------|--------|-------------|---------|-----|
| A3 | Baseline | 550 | 1.0x | ✅ |
| A4 | PrecomputeChannelCache | 104 | 5.2x | ✅ |
| A5 | Remove exp+log round-trip | ~90 | +16% | ✅ |
| A6 | Integrate into QMLERec::QMLE | — | — | ✅ |
| A8 | Acceptance (full reco) | 41 | **11.4x FCN / 3.4x full** | ✅ |

## Key Optimizations

1. **PrecomputeChannelCache**: Geometry + template once per vertex, not per FCN call
2. **ComputeQMLE_Fast**: Hot loop: energy scaling + Poisson only
3. **poissonNeg2LogL**: Direct -2·log(P), skips exp→log round-trip
4. **k=0 fast path**: Skips lgamma for zero-hit channels (common case)
5. **Flat posX/posY/posZ**: Cache-friendly, no TVector3 virtual calls

## Tests

- **Unit**: `test_qmle_fcn` — 4/4 pass
- **E2E**: `test_consistency.py` — 10 events, tree-identical ✅
- **Acceptance**: ≥1.5x speedup — **3.4x achieved** ✅

## Commits

```
50542ef A8: Acceptance - 11.4x FCN speedup, 3.4x full reco
469a6d2 A6: integrate exp+log removal into QMLERec::QMLE
c475a60 A5: remove exp+log round-trip in Poisson likelihood
aba3b36 A4: PrecomputeChannelCache + ComputeQMLE_Fast (5.2x)
665fee8 A3: baseline profile results
95c1464 A2: extract QMLE FCN + unit tests
0ac33d6 A1: build + E2E test
```
