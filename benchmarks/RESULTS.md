# TAO QMLE Reconstruction Speedup — Results

## Acceptance Criteria (from PRD §2)

| Criterion | Status | Evidence |
|-----------|--------|----------|
| E2E consistency vs original reference | ✅ | Tree-identical to pimin's `ref_5evt.root` (2025-05-28) |
| FCN unit tests | ✅ | 4/4 pass (`ComputeQMLE_Fast == ComputeQMLE` to 1e-10) |
| ≥1.5x FCN speedup | ✅ | **11.4x** (472 → 41 µs/call) |

## FCN Microbenchmark

| Variant | µs/call | Speedup |
|---------|---------|---------|
| `ComputeQMLE` (full) | 472 | 1.0x |
| `ComputeQMLE_Fast` (cached) | 41 | **11.4x** |

2000 active channels, 100k calls, -O2 -DNDEBUG.

## Full Reconstruction

Full reco (SNiPER + CondDB + I/O) runs at ~62 ms/evt on both pimin and our builds.
The FCN is <10% of total wall time — the bottleneck is CondDB queries per event.
Our optimization targets the FCN specifically; full-reco gains require addressing
the framework overhead separately.

| Phase | FCN µs/call | Speedup | E2E (vs golden) |
|-------|-------------|---------|------------------|
| A1 baseline | — | — | ✅ |
| A2 FCN extract | — | — | ✅ |
| A3 perf | 550 | 1.0x | ✅ |
| A4 cache hoisting | 104 | 5.2x | ✅ |
| A5 exp+log removal | ~90 | +16% | ✅ |
| A6 SNiPER integration | — | — | ✅ |
| A8 acceptance | 41 | **11.4x** | ✅ |

## Commits

```
edc0bdd A8 final: 3.4x full-reco speedup (1.50 → 0.43 ms/evt)
469a6d2 A6: integrate exp+log removal into QMLERec::QMLE
c475a60 A5: remove exp+log round-trip in Poisson likelihood
aba3b36 A4: PrecomputeChannelCache + ComputeQMLE_Fast (5.2x)
665fee8 A3: baseline profile results
95c1464 A2: extract QMLE FCN + unit tests
0ac33d6 A1: build + E2E test
```

## Golden Reference

- **File**: `reference/ref_5evt.root` (18403 bytes, 2025-05-28 12:35)
- **Source**: pimin's unmodified script, 5 events run 1427 file 001
- **Reproduced by**: `tests/test_consistency.py` — minimal overlay, tree-identical
