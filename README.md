# TAO Reconstruction Speedup

Optimize TAO QMLE reconstruction algorithm for speed.

**Reference project**: [omilrecv2](https://code.ihep.ac.cn/neutrino-physics/dr-sai-juno/omilrecv2) — JUNO OMILREC was optimized from 1525 → 189 ms/evt (8.1x).

## Build

```bash
source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J26.1.0/setup.sh
source /scratchfs/juno/pimin01/taosw_latest/taosw/setup.sh
bash scripts/build.sh
source InstallArea/setup.sh
```

## Test

```bash
# E2E consistency test (10 events vs reference)
python tests/test_consistency.py --evtmax 10

# Generate/update reference
python tests/test_consistency.py --evtmax 10 --generate-only
```

## Benchmark

```bash
bash scripts/benchmark.sh [N=100]
```

## Project Structure

```
├── RecQMLEAlg/        # SNiPER algorithm (copy of pimin's RecQMLEAlg)
├── tests/             # Python tests
├── scripts/           # Build, benchmark, env
├── benchmarks/        # speed.csv, RESULTS.md
├── reference/         # ref_10evt.root
└── docs/              # PRD, plans
```