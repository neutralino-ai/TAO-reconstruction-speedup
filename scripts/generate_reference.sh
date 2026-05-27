#!/bin/bash
# Generate reference output: run N events with the current build,
# save to reference/ref_Nevt.root
#
# Usage:
#   bash scripts/generate_reference.sh [N=10] [run_id=1427] [file_id=001]

set -euo pipefail

PROJ="$(cd "$(dirname "$0")/.." && pwd)"
TMPDIR="/tmp/tao_reco_ref_$$"
mkdir -p "$TMPDIR"

if [ -z "${JUNOTOP:-}" ]; then
    echo "ERROR: JUNOTOP not set. Source CVMFS + TAOSW setup.sh first."
    exit 1
fi

N_EVENTS="${1:-10}"
RUN_ID="${2:-1427}"
FILE_ID="${3:-001}"

# Source our overlay
source "${PROJ}/InstallArea/setup.sh"

INPUT_DIR="/eos/juno/tao-reprod/TaoPP26A/mix_stream/00001000/00001400_CalibData/${RUN_ID}"
ESD_GLOB="RUN.${RUN_ID}.*.${FILE_ID}_T25.7.1_T25.7.1.rec.root"

echo "Copying input file from EOS..."
INPUT_FILE=$(eos ls "${INPUT_DIR}" 2>/dev/null | grep "RUN.${RUN_ID}.*${FILE_ID}" | head -1)
if [ -z "$INPUT_FILE" ]; then
    echo "ERROR: No input file found at ${INPUT_DIR}/ matching ${FILE_ID}"
    rm -rf "$TMPDIR"
    exit 1
fi

eos cp "${INPUT_DIR}/${INPUT_FILE}" "${TMPDIR}/"
LOCAL_INPUT="${TMPDIR}/${INPUT_FILE}"

echo "Input: ${LOCAL_INPUT}"
echo "Running ${N_EVENTS} events..."

mkdir -p "${PROJ}/reference"

python "${PROJ}/RecQMLEAlg/share/run.py" \
    --evtmax "${N_EVENTS}" \
    --use_true_vertex false \
    --input "${LOCAL_INPUT}" \
    --output "${PROJ}/reference/ref_${N_EVENTS}evt.root" \
    --charge_template_file charge_template \
    --loglevel WARN

echo "Reference saved to reference/ref_${N_EVENTS}evt.root"
rm -rf "$TMPDIR"