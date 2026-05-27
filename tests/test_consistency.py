#!/usr/bin/env python3
"""
E2E Consistency Test for TAO QMLE Reconstruction.

Usage:
    python tests/test_consistency.py [--evtmax 10] [--generate-only]
"""

import os, sys, subprocess, tempfile, hashlib, argparse, shlex
from pathlib import Path

PROJ = Path(__file__).resolve().parent.parent
ENV_SH = str(PROJ / 'scripts/env.sh')

def sh(cmd: str, timeout: int = 300) -> subprocess.CompletedProcess:
    return subprocess.run(
        [ENV_SH, 'bash', '-c', cmd],
        capture_output=True, text=True, timeout=timeout
    )

def eos_list(dirpath: str) -> list:
    r = sh(f'eos ls {shlex.quote(dirpath)}')
    lines = r.stdout.strip().split('\n')
    # Filter setup banner
    lines = [l for l in lines if l.strip() and 'Setup Official' not in l]
    return lines

def eos_cp(src: str, dstdir: str):
    r = sh(f'eos cp {shlex.quote(src)} {shlex.quote(dstdir)}/')
    if r.returncode != 0:
        raise RuntimeError(f"EOS cp failed: {r.stderr}")

def get_input_file():
    input_dir = "/eos/juno/tao-reprod/TaoPP26A/mix_stream/00001000/00001400_CalibData/1427"
    files = eos_list(input_dir)
    esd = [f for f in files if '.001_T25' in f]
    if not esd:
        raise RuntimeError(f"No input file at {input_dir}")
    tmpdir = tempfile.mkdtemp(prefix='tao_test_')
    print(f"Fetching {esd[0]} from EOS...")
    eos_cp(f'{input_dir}/{esd[0]}', tmpdir)
    local = f'{tmpdir}/{esd[0]}'
    assert os.path.exists(local)
    return local

def run_reco(evtmax: int, output: str, input_file: str) -> bool:
    cmd = (
        f'python {PROJ}/RecQMLEAlg/share/run.py '
        f'--evtmax {evtmax} '
        f'--use_true_vertex false '
        f'--input {shlex.quote(input_file)} '
        f'--output {shlex.quote(output)} '
        f'--charge_template_file charge_template '
        f'--loglevel WARN'
    )
    r = sh(cmd)
    ok = r.returncode == 0 and os.path.exists(output)
    if not ok:
        print(f"STDERR tail: {r.stderr[-500:]}")
    return ok

def md5(path: str) -> str:
    with open(path, 'rb') as f:
        return hashlib.md5(f.read()).hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--evtmax', type=int, default=10)
    parser.add_argument('--reference', type=str, default=None)
    parser.add_argument('--generate-only', action='store_true')
    args = parser.parse_args()

    os.makedirs(PROJ / 'reference', exist_ok=True)
    os.makedirs(PROJ / 'TEMP', exist_ok=True)

    ref_path = args.reference or str(PROJ / f'reference/ref_{args.evtmax}evt.root')
    input_file = get_input_file()

    if os.path.exists(ref_path) and not args.generate_only:
        print(f"Using existing reference: {ref_path}")
    else:
        print(f"Generating reference ({args.evtmax} events)...")
        ok = run_reco(args.evtmax, ref_path, input_file)
        assert ok, "Reference generation failed"
        print(f"OK: {ref_path} ({os.path.getsize(ref_path)} bytes)")

    if args.generate_only:
        print("Done (generate-only).")
        return 0

    test_out = str(PROJ / 'TEMP/test_output.root')
    print(f"Running test reco ({args.evtmax} events)...")
    ok = run_reco(args.evtmax, test_out, input_file)
    assert ok, "Test reco failed"

    rmd5, tmd5 = md5(ref_path), md5(test_out)
    print(f"MD5: ref={rmd5[:16]}...  test={tmd5[:16]}...")
    if rmd5 == tmd5:
        print("PASS: bit-identical (MD5 match)")
        return 0

    print("MD5 mismatch (expected due to timestamps), comparing tree data...")
    # Write and run ROOT macro for tree Scan
    macro = """
{TFile *f=TFile::Open("%s");TTree *t=(TTree*)f->Get("Event/Rec/QMLE/CdVertexRecEvt");t->Scan("*","","colsize=20");}
"""
    mp_r = f'/tmp/tao_ref_{os.getpid()}.C'
    mp_t = f'/tmp/tao_test_{os.getpid()}.C'
    with open(mp_r,'w') as f: f.write(macro % ref_path)
    with open(mp_t,'w') as f: f.write(macro % test_out)
    ref_txt = sh(f'root -l -b -q {mp_r} 2>&1').stdout
    test_txt = sh(f'root -l -b -q {mp_t} 2>&1').stdout
    os.unlink(mp_r); os.unlink(mp_t)

    # Filter ROOT banner lines
    ref_txt = '\n'.join(l for l in ref_txt.splitlines() if not l.startswith('Processing') and 'Setup' not in l)
    test_txt = '\n'.join(l for l in test_txt.splitlines() if not l.startswith('Processing') and 'Setup' not in l)

    if ref_txt == test_txt:
        print("PASS: tree data identical")
        return 0
    else:
        print("FAIL: tree data differs")
        with open(PROJ/'TEMP/ref_scan.txt','w') as f: f.write(ref_txt)
        with open(PROJ/'TEMP/test_scan.txt','w') as f: f.write(test_txt)
        return 1

if __name__ == '__main__':
    sys.exit(main())