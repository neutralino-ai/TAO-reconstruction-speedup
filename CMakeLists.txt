cmake_minimum_required(VERSION 3.20)
project(TAOReconstructionSpeedup)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/InstallArea)

find_package(junosw REQUIRED CONFIG)
include(PKG)

add_subdirectory(RecQMLEAlg)


include(JUNOSetupScripts)
include(JUNOInstallOfflineSetup)