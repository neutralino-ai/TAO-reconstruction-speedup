# TAO QMLE Reconstruction Speedup — Results

## Current Release

`v0.1.0` completes PRD Step 1: the QMLE test bed.

Commit:

```text
9f01784 Complete Step 1 QMLE test bed
```

## Step 1 Acceptance Evidence

| Criterion | Status | Evidence |
|-----------|--------|----------|
| Build `libRecQMLEAlg.so` | ✅ | `InstallArea/lib64/libRecQMLEAlg.so` |
| Build `libQMLEFCN.a` | ✅ | `InstallArea/lib64/libQMLEFCN.a` |
| Extract free FCN | ✅ | `ComputeQMLE(const QMLEInput&, Evis, vr, vtheta, vphi)` |
| E2E consistency | ✅ | `python tests/test_consistency.py` passes tree-identical comparison |
| FCN fixture test | ✅ | `./build/bin/test_qmle_fcn.exe` passes with zero drift |

## FCN Fixture Test

Input:

```text
fixtures/fcn_evt0.txt
RecQMLEAlg/input/Ge68_charge_template.root:h3_interpolation
```

Golden output:

```text
FCN = 22551.821885859372
```

Observed validation output:

```text
fixture FCN:  22551.821885859372
computed FCN: 22551.821885859372
abs diff:     0
rel diff:     0
```

Tolerance:

```text
relative drift <= 1e-13
```

## E2E Consistency Test

Input:

```text
/eos/juno/tao-reprod/TaoPP26A/mix_stream/00001000/00001400_CalibData/1427
pattern: .001_T25
evtmax: 5
```

Golden reference:

```text
reference/ref_5evt.root
```

Comparison target:

```text
Event/Rec/QMLE/CdVertexRecEvt
TTree::Scan("*") output must be identical
```

Observed validation output:

```text
PASS: tree-identical to pimin golden reference
```

## Notes

Older benchmark numbers in previous commits are superseded by this Step 1 test-bed release. Future optimization work should update this file with fresh, gated measurements after both tests pass.
