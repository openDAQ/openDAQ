set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.25)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set project variables
set(REPO_NAME empty_module)
set(REPO_OPTION_PREFIX EMPTY_MODULE)

message(STATUS "=== Debug: Project detection ===")
message(STATUS "Current PROJECT_NAME           = '${PROJECT_NAME}'")
message(STATUS "Top-level CMAKE_TOP_LEVEL_PROJECT_NAME = '${CMAKE_TOP_LEVEL_PROJECT_NAME}'")

if(PROJECT_NAME STREQUAL CMAKE_TOP_LEVEL_PROJECT_NAME)
    set(IS_TOP_LEVEL_PROJECT TRUE)
    message(STATUS "This is the TOP-LEVEL project")
else()
    set(IS_TOP_LEVEL_PROJECT FALSE)
    message(STATUS "This is a SUBPROJECT")
endif()

message(STATUS "IS_TOP_LEVEL_PROJECT = ${IS_TOP_LEVEL_PROJECT}")
message(STATUS "===============================")


if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(IS_TOP_LEVEL_PROJECT TRUE)
    message(STATUS "This is the TOP-LEVEL project")
else()
    set(IS_TOP_LEVEL_PROJECT FALSE)
    message(STATUS "This is a SUBPROJECT")
endif()

message(STATUS "PROJECT_NAME=${PROJECT_NAME}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")


if (IS_TOP_LEVEL_PROJECT)
    file(READ "repo_version" repo_version)
    string(STRIP ${repo_version} repo_version)
    string(REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" repo_version_major_minor_patch "${repo_version}")
    set(REPO_VERSION "${repo_version_major_minor_patch}")

    # set SDK variables
    set(OPENDAQ_SDK_NAME openDAQ)
    set(OPENDAQ_SDK_TARGET_NAME opendaq)
    set(OPENDAQ_SDK_TARGET_NAMESPACE daq)
else ()
    # set SDK variables
    set(OPENDAQ_SDK_NAME ${SDK_NAME})
    set(OPENDAQ_SDK_TARGET_NAME ${SDK_TARGET_NAME})
    set(OPENDAQ_SDK_TARGET_NAMESPACE ${SDK_TARGET_NAMESPACE})

    set(REPO_VERSION "${OPENDAQ_PACKAGE_VERSION}")
endif()

project(${REPO_NAME}
    LANGUAGES CXX
    VERSION ${REPO_VERSION}
)

#file(READ "opendaq_version" sdk_version)
#string(STRIP ${sdk_version} sdk_version)
#string(REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" sdk_version_major_minor_patch "${sdk_version}")
#set(OPENDAQ_SDK_VERSION "${sdk_version_major_minor_patch}")

list(APPEND CMAKE_MESSAGE_CONTEXT ${REPO_NAME})
set(CMAKE_MESSAGE_CONTEXT_SHOW ON CACHE BOOL "Show CMake message context")

if (IS_TOP_LEVEL_PROJECT)
    list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

    get_filename_component(ROOT_DIR ${CMAKE_SOURCE_DIR} REALPATH)
    set(FETCHCONTENT_EXTERNALS_DIR ${ROOT_DIR}/build/__external CACHE PATH "FetchContent folder prefix")
endif()


#add_definitions(-DFMT_HEADER_ONLY)

# options
#option(${REPO_OPTION_PREFIX}_EXAMPLE_ENABLE_APP "Enable building example application" OFF)
#option(${REPO_OPTION_PREFIX}_ENABLE_TESTS "Enable tests" OFF)

# Runtime and default 3rd party library linking options
#option(${REPO_OPTION_PREFIX}_LINK_RUNTIME_STATICALLY "Link the C++ runtime staticaly (embedd it)" OFF)
#option(${REPO_OPTION_PREFIX}_LINK_3RD_PARTY_LIBS_STATICALY "Link the 3rd party libraries staticaly (embedd it)" ON)

if (IS_TOP_LEVEL_PROJECT)
    include(CommonUtils)
    setup_repo(${REPO_OPTION_PREFIX})
endif()

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)
    message(STATUS "Unit tests are ENABLED")
    enable_testing()
endif()

add_subdirectory(external)

#add_subdirectory(shared)
add_subdirectory(modules)
if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)
#    add_subdirectory(tests)
endif()
if (${REPO_OPTION_PREFIX}_EXAMPLE_ENABLE_APP)
#    add_subdirectory(examples)
endif()
#add_subdirectory(docs)		# FIXME no docs infrastructure to be handled by cmake 
