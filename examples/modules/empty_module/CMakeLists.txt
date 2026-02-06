set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.25)

# set project variables
set(REPO_NAME EmptyModule)
set(REPO_OPTION_PREFIX EMPTY_MODULE)
set(${REPO_OPTION_PREFIX}_VERSION ${OPENDAQ_PACKAGE_VERSION})

project(${REPO_NAME} VERSION ${${REPO_OPTION_PREFIX}_VERSION} LANGUAGES CXX)

add_subdirectory(cmake)
opendaq_module_common_setup(${REPO_OPTION_PREFIX} ${REPO_NAME})

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
