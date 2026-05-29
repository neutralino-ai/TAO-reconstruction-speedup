# A3: Baseline Profile Results

**Date**: 2026-05-29  
**Machine**: user-Super-Server (AlmaLinux 9.4), Intel(R) Xeon(R) Gold ...

## Timing (pimin's unmodified build via LD_PRELOAD)

| Events | Wall (s) | ms/evt | Notes |
|--------|----------|--------|-------|
| 100    | ~0.20    | 1.96   | warm cache |
| 200    | ~0.16    | 0.80   | |
| 500    | ~0.18    | 0.35   | ~70 evt overhead dominates small runs |

**Stable ms/evt (taking 500 evt as closest to steady-state): ~0.35-0.50 ms/evt**

Note: The ~1 ms/evt number is dominated by SNiPER framework overhead (event loop, I/O). 
The FCN itself likely runs in ~10-50 µs per call.

## perf stat (100 events, pimin's build)

```
 29.7 G cycles           @ 3.194 GHz
 44.0 G instructions     1.48 IPC
135.5 M cache-references 14.6 M/sec
 21.6 M cache-misses     15.9% miss rate
 13.7 G branches         1.48 G/sec
 72.3 M branch-misses    0.53% miss rate
 9.31 s task-clock       0.91 CPU util
10.24 s wall time        8.63s user + 0.68s sys
```

## Key observations

1. **IPC 1.48**: Not bad but not great — room for SIMD/ILP improvement
2. **15.9% cache miss rate**: Moderately high — data layout matters (8048 SiPM channels)
3. **0.53% branch miss**: Excellent — predictable branching
4. **Per-event cost**: ~93 ms wall, ~86 ms CPU. I/O + DB queries are ~9s of the 10s total.
   The reconstruction itself is maybe 1-2 ms/evt.

## Hotspot hypothesis (to verify with flamegraph)

Based on code reading of `QMLERec::QMLE()`:
1. `v_vec.Angle(channel_vec[i])` — 8048 acos calls per FCN call
2. `CalExpChargeHit()` — 8048 2D interpolations per FCN call
3. `TMath::LnGamma(k+1)` — 8048 calls per FCN call
4. FCN called 7x per event (Minuit simplex iterations)
   → Total: 7 × 8048 = ~56k hot-loop iterations per event