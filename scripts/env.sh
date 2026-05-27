#!/bin/bash
# Setup wrapper: sources JUNO + TAOSW + local overlay, then runs the given command.
# Usage: bash scripts/env.sh <command>
set -e
source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J26.1.0/setup.sh 2>/dev/null
source /scratchfs/juno/pimin01/taosw_latest/taosw/setup.sh 2>/dev/null
PROJ="$(cd "$(dirname "$0")/.." && pwd)"
source "$PROJ/InstallArea/setup.sh" 2>/dev/null
exec "$@"