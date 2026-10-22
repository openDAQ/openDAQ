cmake_minimum_required(VERSION 3.20)
project(credential_providers VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_executable(credential_providers credential_providers.cpp)
target_link_libraries(credential_providers PRIVATE daq::opendaq)

add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

set(CREDENTIAL_DEMO_KEYS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/credential_demo_module/keys")
cmake_path(NORMAL_PATH CREDENTIAL_DEMO_KEYS_DIR)
add_compile_definitions(CREDENTIAL_DEMO_KEYS_DIR="${CREDENTIAL_DEMO_KEYS_DIR}")
