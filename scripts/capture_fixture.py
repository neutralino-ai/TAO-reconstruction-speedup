#!/usr/bin/env python3
"""Capture a valid QMLE FCN fixture from the instrumented RecQMLEAlg build."""

import os
import subprocess
from pathlib import Path

from loguru import logger

PROJ = Path(__file__).resolve().parent.parent
FIXTURE = PROJ / "fixtures" / "fcn_evt0.txt"
ESD_DIR = "/eos/juno/tao-reprod/TaoPP26A/mix_stream/00001000/00001400_CalibData/1427"
ESD_PATTERN = ".001_T25"


script = f'''
set -eo pipefail
unset VIRTUAL_ENV PYTHONHOME PYTHONPATH
source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J26.1.0/setup.sh 2>/dev/null
source /scratchfs/juno/pimin01/taosw_latest/taosw/setup.sh 2>/dev/null
export RECQMLEALGROOT={PROJ}/RecQMLEAlg
export QMLE_CAPTURE_FCN_FIXTURE={FIXTURE}

TMPDIR=$(mktemp -d /tmp/tao_fcn_fixture_XXXXXX)
cleanup() {{ rm -rf "$TMPDIR"; }}
trap cleanup EXIT

INPUT=$(ls /tmp/tao_test_*/RUN.1427*.rec.root 2>/dev/null | head -1 || true)
if [ -z "$INPUT" ]; then
    FILE=$(eos ls {ESD_DIR} | grep '{ESD_PATTERN}' | head -1)
    eos cp {ESD_DIR}/$FILE "$TMPDIR/"
    INPUT="$TMPDIR/$FILE"
fi

OVERLAY="$TMPDIR/overlay"
mkdir -p "$OVERLAY/lib64"
cp {PROJ}/InstallArea/lib64/libRecQMLEAlg.so "$OVERLAY/lib64/"

CMAKE_PREFIX_PATH="$OVERLAY:$CMAKE_PREFIX_PATH" LD_LIBRARY_PATH="$OVERLAY/lib64:$LD_LIBRARY_PATH" python {PROJ}/RecQMLEAlg/share/run.py \
    --evtmax 5 --use_true_vertex false \
    --input "$INPUT" --output "$TMPDIR/fx_cap.root" \
    --charge_template_file charge_template --loglevel WARN
'''

result = subprocess.run(["bash", "-c", script], capture_output=True, text=True, timeout=300)
if result.returncode != 0:
    logger.error("capture command failed with code {}", result.returncode)
    logger.error("stderr tail:\n{}", "\n".join(result.stderr.splitlines()[-20:]))
    logger.error("stdout tail:\n{}", "\n".join(result.stdout.splitlines()[-20:]))
    raise SystemExit(result.returncode)

if not FIXTURE.exists():
    logger.error("fixture was not created: {}", FIXTURE)
    logger.error("stdout tail:\n{}", "\n".join(result.stdout.splitlines()[-20:]))
    raise SystemExit(1)

lines = FIXTURE.read_text().splitlines()
logger.info("captured {} lines to {}", len(lines), FIXTURE)
for line in lines[:8]:
    logger.info("head: {}", line)
for line in lines[-5:]:
    logger.info("tail: {}", line)

fcn_lines = [line for line in lines if line.startswith("FCN=")]
if not fcn_lines:
    logger.error("fixture missing FCN line")
    raise SystemExit(1)
if fcn_lines[-1] == "FCN=0":
    logger.error("fixture has FCN=0; refusing invalid fixture")
    raise SystemExit(1)

logger.info("fixture capture OK: {}", fcn_lines[-1])
