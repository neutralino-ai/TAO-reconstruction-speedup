import pytest
import os
from pathlib import Path

@pytest.fixture(scope="session")
def project_root():
    return Path(__file__).resolve().parent.parent

@pytest.fixture(scope="session")
def setup_env():
    """Source the JUNO and TAOSW environment."""
    import subprocess
    # Verify environment is set
    result = subprocess.run(
        'bash -c "source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J26.1.0/setup.sh && '
        'source /scratchfs/juno/pimin01/taosw_latest/taosw/setup.sh && echo OK"',
        shell=True, capture_output=True, text=True, executable='/bin/bash'
    )
    if result.returncode != 0 or 'OK' not in result.stdout:
        pytest.skip("JUNO/TAOSW environment not available")
    return True