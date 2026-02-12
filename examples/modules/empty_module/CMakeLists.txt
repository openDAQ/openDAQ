set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.25)

# set project variables
set(REPO_NAME EmptyModule)
set(REPO_OPTION_PREFIX EMPTY_MODULE)
set(${REPO_OPTION_PREFIX}_VERSION ${OPENDAQ_PACKAGE_VERSION})

list(APPEND CMAKE_MESSAGE_CONTEXT ${REPO_NAME})

add_subdirectory(cmake)

# 32-bit Linux cross-compilation setup (must be before any project())
if(NOT DEFINED PROJECT_SOURCE_DIR AND OPENDAQ_FORCE_COMPILE_32BIT AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    opendaq_32bit_build_linux_early_setup()
endif()
opendaq_common_early_setup()

project(${REPO_NAME} VERSION ${${REPO_OPTION_PREFIX}_VERSION} LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
    set(${REPO_OPTION_PREFIX}_BUILDING_AS_SUBMODULE OFF CACHE INTERNAL "Building ${REPO_NAME} standalone")
    message(STATUS "Building ${REPO_NAME} version ${OPENDAQ_PACKAGE_VERSION} standalone")
else()
    set(${REPO_OPTION_PREFIX}_BUILDING_AS_SUBMODULE ON CACHE INTERNAL "Building ${REPO_NAME} as submodule")
    message(STATUS "Building ${REPO_NAME} version ${OPENDAQ_PACKAGE_VERSION} as submodule")
endif()

# options
opendaq_setup_common_build_options()
opendaq_setup_project_specific_build_options(${REPO_OPTION_PREFIX})
option(${REPO_OPTION_PREFIX}_ENABLE_EXAMPLE_APP "Enable ${REPO_NAME} example applications" ON)
option(${REPO_OPTION_PREFIX}_ENABLE_TESTS "Enable ${REPO_NAME} testing" ON)
include("${CMAKE_CURRENT_LIST_DIR}/cmake/ModuleOptions.cmake" OPTIONAL)

opendaq_common_compile_targets_settings()
opendaq_setup_compiler_flags(${REPO_OPTION_PREFIX})

add_subdirectory(external)
add_subdirectory(shared)
add_subdirectory(modules)

if (OPENDAQ_ENABLE_TESTS AND ${REPO_OPTION_PREFIX}_BUILDING_AS_SUBMODULE OR ${REPO_OPTION_PREFIX}_ENABLE_TESTS AND NOT ${REPO_OPTION_PREFIX}_BUILDING_AS_SUBMODULE)
    add_subdirectory(tests)
endif()

if (${REPO_OPTION_PREFIX}_ENABLE_EXAMPLE_APP)
    add_subdirectory(examples)
endif()

add_subdirectory(docs)
