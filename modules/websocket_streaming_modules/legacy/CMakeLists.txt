set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.25)

FetchContent_GetProperties(
    LtStreamingModulesLegacy
    SOURCE_DIR MODULE_SOURCE_DIR
    BINARY_DIR MODULE_BINARY_DIR
)

add_subdirectory("${MODULE_SOURCE_DIR}" "${MODULE_BINARY_DIR}")

