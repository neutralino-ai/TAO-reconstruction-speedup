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
| **Step 1**: Test bed | E2E + FCN fixtures + unit tests | Bit-identical reproduction vs golden reference |
| **Step 2**: Profile | perf/flamegraph, phase timing, roofline | Clear bottleneck ranking |
| **Step 3**: Optimize | Targeted optimizations, one commit at a time | Each commit passes tests; speed improves |
| **Step 4**: Benchmark | 100-event wall-clock timing | Speed gate: ≥5% regression → reject |
| **Step 5**: Iterate | Go back to Step 2 until goal met | Progressive speedup |
| **Step 6**: Acceptance | Resolution comparison vs reference | No degradation beyond tolerance |

## 4. Step 1: Test Bed (DETAILED)

### 4.1a E2E Test: ESD → Reco Vertex

**What**: Run pimin's unmodified reconstruction on an ESD file, collect output.
- **Input**: ESD file (calibrated hit t/q, SiPM channel data)
- **Output**: Reco vertex (x, y, z, energy) — stored as golden reference `reference/ref_Nevt.root`
- **Test**: Run reconstruction again, compare tree data to golden. Must be **tree-identical**.

**Status**: ✅ DONE
- Golden: `reference/ref_5evt.root` (pimin's original, 2025-05-28)
- Test: `tests/test_consistency.py` — minimal overlay, tree-identical
- ESD source: run 1427 file 001 from EOS

### 4.1b FCN Fixture: Calibrated Hits → FCN Value

**What**: Extract the FCN as a pure free function, then capture its inputs and output
from the unmodified (pimin) code for a specific event, and save as a binary fixture.

**Input to FCN**:
- `fChannelHit[8048]` — calibrated hit charge per channel
- `bad_channel_list[8048]` — which channels to skip
- `rPDE_list[8048]` — relative PDE per channel
- `dcr_list[8048]` — dark count rate per channel
- `channel_vec[8048]` — SiPM position vectors
- `charge_template_ge68` — Ge68 charge template (TH3F)
- Fit parameters at initial guess: (Evis, vr, vtheta, vphi)
- Calibration constants: GeEvis, PDE, ESF, saturation

**Output from FCN**: `-2 ln L` value (single double)

**How to capture**: Instrument pimin's `QMLERec::QMLE()` in the unmodified code to
dump all inputs and the return value for event 0 (first event after warmup).

**Status**: ❌ NOT DONE
- `ComputeQMLE` already extracted (A2) but no fixture captured from pimin baseline
- Need: pimin's code → dump fixture → our code → compare value (≤1e-13 drift)
- Fixture file: `fixtures/fcn_evt0.bin`

### 4.1c Verify Both Tests Pass

**What**: After each change, run:
1. `python tests/test_consistency.py` — E2E: our reco output == golden reference
2. `build/bin/test_qmle_fcn` — FCN: our FCN(fixture) == golden value

Both must pass before any optimization commit.

**Status**: ⚠️ PARTIAL
- E2E test: ✅ working (tree-identical to pimin golden)
- FCN fixture test: ❌ not built yet

## 5. Step 2: Profile (DETAILED)

### 5.1 Phase Timing

Instrument `QMLERec::execute()` with per-phase stopwatches:
- Load/calibration phase
- Charge center calculation
- Vertex minimization (already timed as "Vertex Fit Time")
- Output/EDM write

### 5.2 perf Flamegraph

```
perf record -g -- python RecQMLEAlg/share/run.py --evtmax 100 ...
perf script | FlameGraph/stackcollapse-perf.pl | FlameGraph/flamegraph.pl > flame.svg
```

### 5.3 Hotspot Ranking

Top 10 functions by self-time from perf report.

**Status**: ❌ NOT DONE (partial perf stat only)

## 6. Step 3: Optimize (DETAILED)

See original PRD §4.4. Key rules:
- One optimization per commit
- Measure FCN drift and full-reco speed after each
- Rollback if speed regresses ≥5% or drift exceeds tolerance

## 7. Step 4: Benchmark

- 100 events minimum (after warmup)
- Single thread, Release build
- Record ms/evt from SNiPER profiling or per-event stopwatches
- Append to `benchmarks/speed.csv`

**Status**: ❌ NOT DONE

## 8. Step 5-6: Iterate + Acceptance

See original PRD §§4.7-4.8.

---

## Current Status

| Step | Status | Gap |
|------|--------|-----|
| 1a: E2E test | ✅ | Golden ref_5evt.root, test_consistency.py passes |
| 1b: FCN fixture | ❌ | Need to capture fixture from pimin baseline |
| 1c: Both tests pass | ⚠️ | Only E2E works |
| 2: Profile | ❌ | No flamegraph, no phase breakdown |
| 3: Optimize | ⚠️ | Optimizations exist but not measured properly |
| 4: Benchmark | ❌ | Never done 100-event benchmark |
| 5-6: Iterate+Accept | ❌ | Not started |
