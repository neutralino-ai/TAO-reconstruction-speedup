cmake_minimum_required(VERSION 3.20)
project(TAOReconstructionSpeedup)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/InstallArea)

find_package(junosw REQUIRED CONFIG)
include(PKG)

add_subdirectory(RecQMLEAlg)

option(BUILD_UNIT_TESTS "Build C++ unit tests (no SNiPER required)" OFF)
if(BUILD_UNIT_TESTS)
    enable_testing()
    add_subdirectory(tests/unit)
endif()

include(JUNOSetupScripts)
include(JUNOInstallOfflineSetup)