#!/bin/bash
# Build TAO Reconstruction Speedup project
#
# Prerequisites:
#   source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J26.1.0/setup.sh
#   source /scratchfs/juno/pimin01/taosw_latest/taosw/setup.sh
#
# Usage:
#   bash scripts/build.sh [Debug|Release|RelWithDebInfo]
#   bash scripts/build.sh Debug -DENABLE_ASAN=ON

set -euo pipefail

BUILD_TYPE="${1:-Release}"
shift || true

PROJ="$(cd "$(dirname "$0")/.." && pwd)"

if [ -z "${JUNOTOP:-}" ]; then
    echo "ERROR: JUNOTOP not set. Source CVMFS setup.sh first."
    exit 1
fi

echo "Building TAO-reconstruction-speedup (${BUILD_TYPE})..."

cmake -S "${PROJ}" -B "${PROJ}/build" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    "$@"

cmake --build "${PROJ}/build" --target install --parallel

echo ""
echo "Build complete. Source overlay before running:"
echo "  source ${PROJ}/InstallArea/setup.sh"