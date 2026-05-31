#!/usr/bin/env python3
"""
E2E Consistency — validates against ORIGINAL pimin golden reference.

GOLDEN: reference/ref_5evt.root (pimin's script, 2025-05-28, NEVER regenerate)
"""
import os
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

from loguru import logger

PROJ = Path(__file__).resolve().parent.parent
GOLDEN = str(PROJ / "reference/ref_5evt.root")
ESD_DIR = "/eos/juno/tao-reprod/TaoPP26A/mix_stream/00001000/00001400_CalibData/1427"
ESD_PATTERN = ".001_T25"


def sh(cmd, timeout=180):
    script = (
        "source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J26.1.0/setup.sh 2>/dev/null\n"
        "source /scratchfs/juno/pimin01/taosw_latest/taosw/setup.sh 2>/dev/null\n" + cmd
    )
    return subprocess.run(["bash", "-c", script], capture_output=True, text=True, timeout=timeout)


def eos_ls(directory):
    result = sh(f"eos ls {shlex.quote(directory)}")
    return [line.strip() for line in result.stdout.split("\n") if line.strip() and "Setup" not in line]


def scan_tree(rootfile):
    macro = f'{{TFile *f=TFile::Open("{rootfile}");TTree *t=(TTree*)f->Get("Event/Rec/QMLE/CdVertexRecEvt");if(!t){{cerr<<"ERR"<<endl;return;}}t->Scan("*","","colsize=20");}}'
    macro_path = f"/tmp/ts_{os.getpid()}.C"
    with open(macro_path, "w") as handle:
        handle.write(macro)
    result = sh(f"root -l -b -q {macro_path} 2>&1")
    os.unlink(macro_path)
    lines = [line for line in result.stdout.splitlines() if line.startswith("*")]
    return "\n".join(lines)


def main():
    assert os.path.exists(GOLDEN), f"GOLDEN missing: {GOLDEN}"
    golden_txt = scan_tree(GOLDEN)
    assert golden_txt.strip(), "Cannot read golden"

    files = eos_ls(ESD_DIR)
    esd = [filename for filename in files if ESD_PATTERN in filename]
    assert esd, f"No ESD at {ESD_DIR}"
    tmpdir = tempfile.mkdtemp(prefix="tc_")

    result = sh(f"eos cp {ESD_DIR}/{esd[0]} {tmpdir}/")
    inp = f"{tmpdir}/{esd[0]}"
    out = f"{tmpdir}/output.root"
    if result.returncode != 0:
        logger.error("failed to copy ESD input: {}", result.stderr[-500:])
        return 1

    # Minimal overlay: only our libRecQMLEAlg.so
    overlay = f"{tmpdir}/overlay"
    os.makedirs(f"{overlay}/lib64", exist_ok=True)
    our_lib = f"{PROJ}/InstallArea/lib64/libRecQMLEAlg.so"
    assert os.path.exists(our_lib), f"Build not found: {our_lib}"
    subprocess.run(["cp", our_lib, f"{overlay}/lib64/"], check=True)

    cmd = (
        f"export CMAKE_PREFIX_PATH={overlay}:$CMAKE_PREFIX_PATH && "
        f"export LD_LIBRARY_PATH={overlay}/lib64:$LD_LIBRARY_PATH && "
        f"export RECQMLEALGROOT={PROJ}/RecQMLEAlg && "
        f"python {PROJ}/RecQMLEAlg/share/run.py "
        "--evtmax 5 --use_true_vertex false "
        f"--input {shlex.quote(inp)} --output {shlex.quote(out)} "
        "--charge_template_file charge_template --loglevel WARN"
    )
    result = sh(cmd, timeout=300)
    assert result.returncode == 0 and os.path.exists(out), f"Reco failed:\n{result.stderr[-500:]}"

    test_txt = scan_tree(out)
    assert test_txt.strip(), "Cannot read test output"

    if golden_txt == test_txt:
        logger.info("PASS: tree-identical to pimin golden reference")
        return 0

    logger.error("FAIL: differs from golden")
    logger.error("Golden lines: {}, Test lines: {}", golden_txt.count(chr(10)), test_txt.count(chr(10)))
    golden_lines = golden_txt.splitlines()
    test_lines = test_txt.splitlines()
    for index, (golden_line, test_line) in enumerate(zip(golden_lines, test_lines)):
        if golden_line != test_line:
            logger.error("L{}: G={}\n       T={}", index, golden_line[:100], test_line[:100])
            break
    return 1


if __name__ == "__main__":
    sys.exit(main())
