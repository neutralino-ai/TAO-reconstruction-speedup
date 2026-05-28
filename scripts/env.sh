#!/bin/bash
# Setup wrapper: sources JUNO + TAOSW + local overlay for SNiPER-based reco.
# Usage: bash scripts/env.sh <command>
set -e
PROJ="$(cd "$(dirname "$0")/.." && pwd)"
export RECQMLEALGROOT="$PROJ/RecQMLEAlg"
source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J26.1.0/setup.sh 2>/dev/null
source /scratchfs/juno/pimin01/taosw_latest/taosw/setup.sh 2>/dev/null
# NOTE: Do NOT source our InstallArea/setup.sh here — that causes
# "duplicated class name" errors in SNiPER because CVMFS junosw already
# provides CondDBSvc, BufferMemMgr etc.
# Our libRecQMLEAlg is loaded via the algorithm python that finds it
# through the `share/run.py` path and CVMFS overlay.
exec "$@"