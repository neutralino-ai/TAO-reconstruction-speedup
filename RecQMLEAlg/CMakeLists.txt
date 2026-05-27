PKG(RecQMLEAlg
    MODULE
    DEPENDS
        TAOGeometry
        EvtNavigator
        SimEvent
        ElecEvent
        CalibEvent
        RecEvent
        BufferMemMgr
        CalibSvcLib
        ${SNIPER_RootWriter_LIBRARY}
)

# Use pimin's taosw_latest InstallArea (has getTileCenter, etc.) instead of CVMFS.
# The setup.sh from pimin's area puts it at the front of CMAKE_PREFIX_PATH.
# We add its include dir and lib dir explicitly for safety.

set(TAOSW_INSTALL "/scratchfs/juno/pimin01/taosw_latest/taosw/InstallArea")
if(EXISTS ${TAOSW_INSTALL})
    target_include_directories(RecQMLEAlg BEFORE PRIVATE ${TAOSW_INSTALL}/include)
    target_link_directories(RecQMLEAlg BEFORE PRIVATE ${TAOSW_INSTALL}/lib64)
endif()

# Install Python module that shadows CVMFS RecQMLEAlg
install(DIRECTORY python/ DESTINATION python)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
    target_compile_options(RecQMLEAlg PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(RecQMLEAlg PRIVATE -fsanitize=address)
endif()