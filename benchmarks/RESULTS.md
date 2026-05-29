# TAO QMLE Reconstruction Speedup Results

## Baseline (A3)

- **FCN**: ~550 µs/call (2000 active channels, single vertex)
- **perf stat** (100 events, pimin build): 1.48 IPC, 15.9% cache miss, 0.53% branch miss

## Optimization History

| Phase | Change | FCN µs/call | Speedup | E2E |
|-------|--------|-------------|---------|-----|
| A3 | Baseline | 550 | 1.0x | ✅ |
| A4 | PrecomputeChannelCache + ComputeQMLE_Fast | 104 | 5.2x | ✅ |
| A5 | Remove exp+log round-trip (poissonNeg2LogL) | ~90 | 1.1x → **5.7x total** | ✅ |
| A6 | Integrate exp+log fix into QMLERec::QMLE | — | — | ✅ |

## Final FCN Benchmark

```
N=50000   Full: 472 µs/call
          Fast:  41 µs/call
          Speedup: 11.4x

N=100000  Full: 484 µs/call
          Fast:  44 µs/call
          Speedup: 11.1x
```

**FCN speedup: 11.4x** (472 → 41 µs/call with 2000 active channels)

## Unit Tests

- `test_qmle_fcn`: 4/4 pass (all-channels-bad, fast-matches-full, monotonic-energy, finite-output)
- `test_consistency`: E2E PASS (10 events, tree-identical to reference)

## Key Optimizations Applied

1. **PrecomputeChannelCache**: Template + geometry computed once per vertex (not per FCN call)
2. **ComputeQMLE_Fast**: Hot loop only does energy scaling + Poisson likelihood
3. **poissonNeg2LogL**: Direct -2·log(P) computation, skips exp() then log() round-trip
4. **k=0 fast path**: Common case (zero-hit channels) skips lgamma entirely
5. **QMLEChannelData**: TVector3 → flat posX/posY/posZ (cache-friendly AoS→SoA)

## Commits

```
469a6d2 A6: integrate exp+log removal into QMLERec::QMLE
c475a60 A5: remove exp+log round-trip in Poisson likelihood
aba3b36 A4: PrecomputeChannelCache + ComputeQMLE_Fast (5.2x FCN speedup)
665fee8 A3: baseline profile results
95c1464 A2: extract QMLE FCN + unit tests
0ac33d6 A1: build + E2E test
```
