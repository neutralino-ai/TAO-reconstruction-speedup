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
set(TAOSW_INSTALL "/scratchfs/juno/pimin01/taosw_latest/taosw/InstallArea")
if(EXISTS ${TAOSW_INSTALL})
    target_include_directories(RecQMLEAlg PRIVATE ${TAOSW_INSTALL}/include)
    target_link_directories(RecQMLEAlg BEFORE PRIVATE ${TAOSW_INSTALL}/lib64)
endif()

# --- QMLE FCN as a standalone static library (no SNiPER dependency) ---
add_library(QMLEFCN STATIC
    src/QMLEFCN.cc
    src/ChargeTemplate.cc
)
set_target_properties(QMLEFCN PROPERTIES POSITION_INDEPENDENT_CODE ON)
find_package(ROOT REQUIRED COMPONENTS Core Hist Tree Physics RIO)
target_include_directories(QMLEFCN PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TAOSW_INSTALL}/include
    ${ROOT_INCLUDE_DIRS}
)
target_link_libraries(QMLEFCN PUBLIC
    ROOT::Core ROOT::Hist ROOT::Tree ROOT::Physics ROOT::RIO
)
install(TARGETS QMLEFCN ARCHIVE DESTINATION lib64)
target_link_libraries(RecQMLEAlg QMLEFCN)

# --- Unit test for the QMLE FCN ---
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(test_qmle_fcn tests/test_qmle_fcn.cc)
    target_include_directories(test_qmle_fcn PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_directories(test_qmle_fcn PRIVATE ${TAOSW_INSTALL}/lib64)
    target_link_libraries(test_qmle_fcn QMLEFCN ROOT::Core ROOT::Hist ROOT::Tree ROOT::Physics ROOT::RIO)
    add_test(NAME qmle_fcn COMMAND test_qmle_fcn)
    set_tests_properties(qmle_fcn PROPERTIES
        ENVIRONMENT "RECQMLEALGROOT=${CMAKE_CURRENT_SOURCE_DIR}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()

# Install Python module that shadows CVMFS RecQMLEAlg
install(DIRECTORY python/ DESTINATION python)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
    target_compile_options(RecQMLEAlg PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(RecQMLEAlg PRIVATE -fsanitize=address)
endif()