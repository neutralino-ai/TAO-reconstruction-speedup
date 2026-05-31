# TAO Reconstruction Speedup

Optimize TAO QMLE reconstruction algorithm for speed while preserving agreement with the pimin baseline.

**Current release**: `v0.1.0` — Step 1 test bed complete.

## Build

```bash
cd /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J26.1.0
source setup.sh
cd /path/to/TAO-reconstruction-speedup
source /scratchfs/juno/pimin01/taosw_latest/taosw/setup.sh
bash scripts/build.sh Release
```

Build outputs:

```text
InstallArea/lib64/libRecQMLEAlg.so
InstallArea/lib64/libQMLEFCN.a
build/bin/test_qmle_fcn.exe
```

## Test

```bash
# FCN fixture golden test
export RECQMLEALGROOT=$PWD/RecQMLEAlg
./build/bin/test_qmle_fcn.exe

# E2E consistency test: 5 events vs pimin golden reference
python tests/test_consistency.py
```

References:

```text
FCN fixture: fixtures/fcn_evt0.txt
FCN golden:  22551.821885859372
E2E golden:  reference/ref_5evt.root
```

## Benchmark / Profile

```bash
bash scripts/benchmark.sh [N_TOTAL=110]
bash scripts/profile.sh [N_FCN_CALLS=1000]
```

## Project Structure

```text
├── RecQMLEAlg/        # SNiPER algorithm and standalone QMLE FCN library
├── tests/             # Python E2E consistency test
├── fixtures/          # FCN golden fixture
├── scripts/           # Build, benchmark, profile, fixture capture helpers
├── benchmarks/        # speed.csv, RESULTS.md
├── reference/         # ignored locally; contains ref_5evt.root golden reference
└── docs/              # PRD
```
