set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.25)

set(REPO_NAME RefFbModule)
set(REPO_OPTION_PREFIX DAQMODULES_REF_FB_MODULE)

list(APPEND CMAKE_MESSAGE_CONTEXT ${REPO_NAME})
add_subdirectory(cmake)

if (NOT DEFINED ${REPO_OPTION_PREFIX}_VERSION)
    if (NOT DEFINED PROJECT_NAME AND DEFINED OPENDAQ_PACKAGE_VERSION)
        set(${REPO_OPTION_PREFIX}_VERSION ${OPENDAQ_PACKAGE_VERSION})
    else()
        message(FATAL_ERROR "Building ${REPO_NAME} standalone without version specified; tip: use '-D${REPO_OPTION_PREFIX}_VERSION=' to set the version")
    endif()
endif()

# 32-bit Linux cross-compilation setup (must be before any project())
if(NOT DEFINED PROJECT_NAME AND OPENDAQ_FORCE_COMPILE_32BIT AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    opendaq_32bit_build_linux_early_setup()
endif()
opendaq_common_early_setup()

project(${REPO_NAME} VERSION ${${REPO_OPTION_PREFIX}_VERSION} LANGUAGES CXX)

if (PROJECT_IS_TOP_LEVEL)
    message(STATUS "Building ${REPO_NAME} version ${${REPO_OPTION_PREFIX}_VERSION} standalone")
else()
    message(STATUS "Building ${REPO_NAME} version ${${REPO_OPTION_PREFIX}_VERSION} as submodule")
endif()

# options
opendaq_setup_common_build_options()
opendaq_setup_project_specific_build_options(${REPO_OPTION_PREFIX})
option(${REPO_OPTION_PREFIX}_ENABLE_EXAMPLE_APP "Enable ${REPO_NAME} example applications" ${PROJECT_IS_TOP_LEVEL})
option(${REPO_OPTION_PREFIX}_ENABLE_TESTS "Enable ${REPO_NAME} testing" ${PROJECT_IS_TOP_LEVEL})
option(${REPO_OPTION_PREFIX}_ENABLE_RENDERER "Enable renderer function block" ON)

opendaq_common_compile_targets_settings()

if (${REPO_OPTION_PREFIX}_ENABLE_RENDERER AND APPLE)
    enable_language(OBJC OBJCXX)
endif()

opendaq_setup_compiler_flags(${REPO_OPTION_PREFIX})

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)
    message(STATUS "Unit tests in ${REPO_NAME} are ENABLED")
    enable_testing()
else()
    message(STATUS "Unit tests in ${REPO_NAME} are DISABLED")
endif()

if (NOT TARGET "${OPENDAQ_SDK_TARGET_NAMESPACE}::${OPENDAQ_SDK_TARGET_NAME}")
    if (PROJECT_IS_TOP_LEVEL)
        find_package(${OPENDAQ_SDK_NAME} GLOBAL)
    endif()
    if (NOT ${OPENDAQ_SDK_NAME}_FOUND)
        if (NOT DEFINED OPENDAQ_REF)
            message(FATAL_ERROR "Building ${REPO_NAME} standalone without ${OPENDAQ_SDK_NAME} project git reference specified; tip: use '-DOPENDAQ_REF=' to set it")
        endif()

        include(FetchContent)
        set(OPENDAQ_ENABLE_TESTS OFF CACHE BOOL "")
        FetchContent_Declare(
            ${OPENDAQ_SDK_NAME}
            GIT_REPOSITORY https://github.com/openDAQ/openDAQ.git
            GIT_TAG ${OPENDAQ_REF}
            GIT_PROGRESS   ON
            SOURCE_DIR ${FETCHCONTENT_EXTERNALS_DIR}/src/${OPENDAQ_SDK_NAME}
        )
        FetchContent_MakeAvailable(${OPENDAQ_SDK_NAME})
    else()
        message(STATUS "Found installed ${OPENDAQ_SDK_NAME} version: ${${OPENDAQ_SDK_NAME}_VERSION}")
    endif()
endif()

add_subdirectory(external)
add_subdirectory(shared)
add_subdirectory(modules)
