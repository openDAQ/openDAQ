set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.24)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_FOLDER "${CMAKE_FOLDER}/openDAQ")

add_subdirectory(cmake)

set(SDK_NAME "${OPENDAQ_SDK_NAME}")
set(SDK_TARGET_NAME "${OPENDAQ_SDK_TARGET_NAME}")
set(SDK_TARGET_NAMESPACE "${OPENDAQ_SDK_TARGET_NAMESPACE}")

set(SDK_OPTION_PREFIX OPENDAQ)

list(APPEND CMAKE_MESSAGE_CONTEXT ${SDK_NAME})

opendaq_read_file_contents("${CMAKE_CURRENT_LIST_DIR}/opendaq_version" package_version)
opendaq_get_version_major_minor_patch("${package_version}" package_version_major_minor_patch)

set(OPENDAQ_IS_RELEASE_VERSION OFF)
if (package_version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    set(OPENDAQ_IS_RELEASE_VERSION ON)
endif()

# defined as cache variable to be visible by any parent project
set(OPENDAQ_PACKAGE_VERSION "${package_version_major_minor_patch}" CACHE INTERNAL "openDAQ SDK version")

# 32-bit Linux cross-compilation setup (must be before any project())
if (NOT DEFINED PROJECT_NAME AND OPENDAQ_FORCE_COMPILE_32BIT AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    opendaq_32bit_build_linux_early_setup()
endif()
opendaq_common_early_setup()

project(${SDK_NAME}
    LANGUAGES CXX
    VERSION ${OPENDAQ_PACKAGE_VERSION}
)

if (PROJECT_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".CMakePredefinedTargets")
endif()

if (PROJECT_IS_TOP_LEVEL)
    set(${SDK_OPTION_PREFIX}_BUILDING_AS_SUBMODULE OFF CACHE INTERNAL "Building ${SDK_NAME} standalone")
    message(STATUS "Building ${OPENDAQ_SDK_NAME} version ${OPENDAQ_PACKAGE_VERSION} standalone")
else()
    set(${SDK_OPTION_PREFIX}_BUILDING_AS_SUBMODULE ON CACHE INTERNAL "Building ${SDK_NAME} as submodule")
    message(STATUS "Building ${OPENDAQ_SDK_NAME} version ${OPENDAQ_PACKAGE_VERSION} as submodule")
endif()

if (NOT OPENDAQ_CI_RUNNER)
    set(OPENDAQ_CI_RUNNER OFF)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake
)
include(DeprecatedCommands)

get_filename_component(PROJ_DIR ${PROJECT_SOURCE_DIR} REALPATH)

find_package(GitInfo)
if (COMMAND GIT_WC_INFO)
    GIT_WC_INFO("${CMAKE_CURRENT_SOURCE_DIR}" "OPENDAQ")
    if (DEFINED CI_GIT_BRANCH AND NOT ${CI_GIT_BRANCH} STREQUAL "")
        STRING(REPLACE "refs/heads/" "" OPENDAQ_WC_BRANCH_NAME ${CI_GIT_BRANCH})
    endif()

    message(STATUS "Git branch: ${OPENDAQ_WC_BRANCH_NAME} | ${OPENDAQ_WC_REVISION_HASH}")
    if (NOT ${OPENDAQ_WC_BRANCH_NAME} MATCHES "^release\/[0-9]+\.[0-9]+(\.[0-9])?(\.[0-9])?(-[a-z|A-Z|0-9]*)?$")
        set (OPENDAQ_PACKAGE_VERSION_WITH_SHA "${OPENDAQ_PACKAGE_VERSION}_${OPENDAQ_WC_REVISION_HASH}")
    endif()
else()
    set(OPENDAQ_WC_REVISION "NA")
endif()

if (NOT ${OPENDAQ_PACKAGE_VERSION_WITH_SHA} STREQUAL "")
    message(STATUS "openDAQ package version: ${OPENDAQ_PACKAGE_VERSION_WITH_SHA}")
    add_compile_definitions(OPENDAQ_PACKAGE_VERSION="${OPENDAQ_PACKAGE_VERSION_WITH_SHA}")
else()
    message(STATUS "openDAQ package version: ${OPENDAQ_PACKAGE_VERSION}")
    add_compile_definitions(OPENDAQ_PACKAGE_VERSION="${OPENDAQ_PACKAGE_VERSION}")
endif()

opendaq_setup_common_build_options()
opendaq_setup_project_specific_build_options(${SDK_OPTION_PREFIX})

# Feature options
option(OPENDAQ_ENABLE_PARAMETER_VALIDATION "Enable parameter validation in functions" ON)
option(OPENDAQ_ENABLE_WEBSOCKET_STREAMING "Enable ${SDK_NAME} websocket streaming" OFF)
cmake_dependent_option(DAQMODULES_LT_LEGACY_MODULES "Use legacy LT streaming modules" ${OPENDAQ_CI_RUNNER} "OPENDAQ_ENABLE_WEBSOCKET_STREAMING" OFF)
option(OPENDAQ_THREAD_SAFE "Enable thread-safe implementations where available" ON)
option(OPENDAQ_MIMALLOC_SUPPORT "Enable MiMalloc-based packet allocator" OFF)
option(OPENDAQ_ENABLE_NATIVE_STREAMING "Enable ${SDK_NAME} native streaming" OFF)
option(OPENDAQ_ENABLE_ACCESS_CONTROL "Enable object-level access control" ON)

option(OPENDAQ_ENABLE_OPCUA "Enable OpcUa" OFF)

option(OPENDAQ_ENABLE_EXAMPLE_APP "Enable example applications" ON)

# Bindings
option(OPENDAQ_GENERATE_DELPHI_BINDINGS "Generate Delphi bindings" OFF)
option(OPENDAQ_GENERATE_PYTHON_BINDINGS "Generate Python bindings" OFF)
option(OPENDAQ_GENERATE_CSHARP_BINDINGS "Generate CSharp bindings" OFF)
option(OPENDAQ_GENERATE_C_BINDINGS "Generate C bindings" OFF)

# Testing and coverage options
option(OPENDAQ_ENABLE_TESTS "Enable testing" ON)
option(OPENDAQ_ENABLE_TEST_UTILS "Enable testing utils library" ON)
option(OPENDAQ_ENABLE_OPTIONAL_TESTS "Enable optional (debugging) tests" OFF)
option(OPENDAQ_ENABLE_COVERAGE "Enable code coverage in testing" OFF)
option(OPENDAQ_ENABLE_REGRESSION_TESTS "Enable regression testing" OFF)
option(OPENDAQ_ENABLE_UNSTABLE_TEST_LABELS "Enable labeling unstable tests" OFF)

# Additional build options
option(OPENDAQ_ENABLE_ERROR_GUARD "Enable error guard" OFF)

if (NOT OPENDAQ_ENABLE_ERROR_GUARD AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OPENDAQ_ENABLE_ERROR_GUARD ON CACHE BOOL "Enable error guard" FORCE)
endif()

if (OPENDAQ_ENABLE_ERROR_GUARD)
    add_compile_definitions(OPENDAQ_ENABLE_ERROR_GUARD)
endif()

# Dependent options
cmake_dependent_option(OPENDAQ_ENABLE_DELPHI_BINDINGS_TESTS "Enable delphi bindings tests" ON "OPENDAQ_GENERATE_DELPHI_BINDINGS;OPENDAQ_ENABLE_TESTS" OFF)
cmake_dependent_option(OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS "Enable python bindings tests" ON "OPENDAQ_GENERATE_PYTHON_BINDINGS;OPENDAQ_ENABLE_TESTS" OFF)
cmake_dependent_option(OPENDAQ_ENABLE_CSHARP_BINDINGS_TESTS "Enable csharp bindings tests" ON "OPENDAQ_GENERATE_CSHARP_BINDINGS;OPENDAQ_ENABLE_TESTS" OFF)
cmake_dependent_option(OPENDAQ_ENABLE_C_BINDINGS_TESTS "Enable C bindings tests" ON "OPENDAQ_GENERATE_C_BINDINGS;OPENDAQ_ENABLE_TESTS" OFF)
cmake_dependent_option(OPENDAQ_GENERATE_PYTHON_BINDINGS_STUBS "Generate python bindings stubs for auto-completion" OFF "OPENDAQ_GENERATE_PYTHON_BINDINGS" OFF)
cmake_dependent_option(OPENDAQ_SKIP_UNSTABLE_TESTS "Skip tests marked as unstable" ON "OPENDAQ_ENABLE_UNSTABLE_TEST_LABELS" ON)

# Optional Modules
option(DAQMODULES_OPENDAQ_CLIENT_MODULE "Building of ${SDK_NAME} client modules" OFF)
option(DAQMODULES_OPENDAQ_SERVER_MODULE "Building of ${SDK_NAME} server modules" OFF)
option(DAQMODULES_EMPTY_MODULE "Building of demo module" OFF)
option(DAQMODULES_AUDIO_DEVICE_MODULE "Building of audio device module" OFF)
option(DAQMODULES_REF_DEVICE_MODULE "Building of reference device module" OFF)
option(DAQMODULES_SIMULATOR_DEVICE_MODULE "Building of simulator device module" OFF)
option(DAQMODULES_REF_FB_MODULE "Building of reference function block module" OFF)
option(DAQMODULES_BASIC_CSV_RECORDER_MODULE "Building of basic CSV recorder module" OFF)
option(DAQMODULES_PARQUET_RECORDER_MODULE "Building of Parquet recorder module" OFF)
option(DAQMODULES_LICENSING_MODULE "Enable licensing" OFF)

# logging start
include(openDAQLogging)

set(OPENDAQ_LOG_LEVEL_DEBUG "Debug" CACHE STRING "Compile time logging level for debug builds")
set_property(CACHE OPENDAQ_LOG_LEVEL_DEBUG PROPERTY STRINGS ${OPENDAQ_LOG_LEVELS})
opendaq_log_level_str_to_int(${OPENDAQ_LOG_LEVEL_DEBUG} OPENDAQ_LOG_LEVEL_DEBUG_INT)

set(OPENDAQ_LOG_LEVEL_RELEASE "Info" CACHE STRING "Compile time logging level for release builds")
set_property(CACHE OPENDAQ_LOG_LEVEL_RELEASE PROPERTY STRINGS ${OPENDAQ_LOG_LEVELS})
opendaq_log_level_str_to_int(${OPENDAQ_LOG_LEVEL_RELEASE} OPENDAQ_LOG_LEVEL_RELEASE_INT)

add_compile_definitions(OPENDAQ_LOG_LEVEL=$<IF:$<CONFIG:Debug>,${OPENDAQ_LOG_LEVEL_DEBUG_INT},${OPENDAQ_LOG_LEVEL_RELEASE_INT}>)
# logging end

set(OPENDAQ_REPO_PREFIX "https://github.com/openDAQ" CACHE STRING "Set this if using a repository mirror")
message(STATUS "${SDK_NAME} repository prefix: ${OPENDAQ_REPO_PREFIX}")

set(OPENDAQ_SOURCE_DIR ${PROJECT_SOURCE_DIR})

if (OPENDAQ_CI_RUNNER)
    message("Configured as CI runner")
    add_compile_definitions(DS_CI_RUNNER=1)
    set(OPENDAQ_DEBUG_WARNINGS_AS_ERRORS ON)
endif()

if (COMMAND get_mode)
    get_mode(CMAKE_MODE)
    message(STATUS "CMake Mode is ${CMAKE_MODE}")
endif()

if (OPENDAQ_ENABLE_COVERAGE)
    if(NOT OPENDAQ_ENABLE_TESTS)
        message(FATAL_ERROR "Coverage enabled, but testing disabled. Enable testing")
    endif()
    if(CMAKE_COMPILER_IS_GNUCXX OR MSVC)
        message(STATUS "Coverage enabled")
    else()
        message(FATAL_ERROR "Coverage supported only for GCC, G++ and MSVC")
    endif()

    include(CodeCoverage)
endif()

if (OPENDAQ_ENABLE_TESTS)
    if (NOT gtest IN_LIST TESTING_LIBS)
        set(TESTING_LIBS gtest gmock ${TESTING_LIBS})
    endif()
    set(gtest_force_shared_crt On CACHE BOOL "Force shared CRT")

    if (OPENDAQ_ENABLE_DELPHI_BINDINGS_TESTS)
        if (WIN32)
            if ("${DELPHI_BIN_DIR}" STREQUAL "" AND NOT "$ENV{DELPHI_BIN_DIR}" STREQUAL "")
                set(DELPHI_BIN_DIR $ENV{DELPHI_BIN_DIR})
            endif()

            find_program(DELPHI_RSVARS rsvars.bat PATHS ${DELPHI_BIN_DIR})
            if (NOT DELPHI_RSVARS)
                message(FATAL_ERROR "Delphi not found, set DELPHI_BIN_DIR variable. Delphi bindings will not be tested!")
            else()
                message(STATUS "Delphi found: ${DELPHI_RSVARS}")
            endif()
        else()
            message(FATAL_ERROR "Delphi bindings only available in Windows")
        endif()
    endif()
endif()

if (NOT COMMAND set_mode)
    message(STATUS "Including Modern.cmake")
    include(Modern)
else()
    set_mode(MODERN)
endif()

opendaq_common_compile_targets_settings()

if (OPENDAQ_ENABLE_TESTS)
    message(STATUS "Unit tests are ENABLED")
    enable_testing()
else()
    message(STATUS "Unit tests are DISABLED")
endif()

if (CMAKE_COMPILER_IS_GNUCXX)
    if (OPENDAQ_ENABLE_COVERAGE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage -fno-elide-constructors")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    endif()
endif()

if (OPENDAQ_ENABLE_COVERAGE AND MSVC AND NOT CMAKE_COMPILER_IS_CLANGXX)
    # /PROFILE add profiler info to PDB
    # /OPT:NOREF do not omit unreferenced files/functions/classes in the PDB (would hide completely untested code)

    #set(CMAKE_EXE_LINKER_FLAGS_DEBUG    "${CMAKE_EXE_LINKER_FLAGS}    /PROFILE /OPT:NOREF")
    set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS} /PROFILE /OPT:NOREF")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS} /PROFILE /OPT:NOREF")
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS} /PROFILE /OPT:NOREF")
endif()

# Sanitizers
if (CMAKE_COMPILER_IS_GNUCXX)
    set(OPENDAQ_SANITIZER "" CACHE STRING "Add sanitizers. Options are: thread, address, ...")
    if (NOT "${OPENDAQ_SANITIZER}" STREQUAL "")
        set(SANITIZE_SWITCH "-fsanitize=${OPENDAQ_SANITIZER}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZE_SWITCH}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZE_SWITCH}")
        #set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} ${SANITIZE_SWITCH}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_SWITCH}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZE_SWITCH}")
    endif()
endif()

if (OPENDAQ_GENERATE_PYTHON_BINDINGS OR OPENDAQ_ENABLE_COVERAGE) #gcc coverage
    set(OPENDAQ_PYTHON_VERSION_INTERNAL 3.8)
    set(OPENDAQ_PYTHON_COMPONENTS Interpreter)
    if(OPENDAQ_GENERATE_PYTHON_BINDINGS)
        list(APPEND OPENDAQ_PYTHON_COMPONENTS Development.Module)
    endif()
    set(OPENDAQ_PYTHON_VERSION "" CACHE STRING "Exact Python version to use for bindings generation")
    if(OPENDAQ_PYTHON_VERSION)
        set(OPENDAQ_PYTHON_VERSION_INTERNAL ${OPENDAQ_PYTHON_VERSION})
        set(OPENDAQ_PYTHON_EXACT EXACT)
        set(OPENDAQ_PYTHON_REQUIRED REQUIRED)
    endif()

    find_package(Python ${OPENDAQ_PYTHON_VERSION_INTERNAL} ${OPENDAQ_PYTHON_EXACT} ${OPENDAQ_PYTHON_REQUIRED} COMPONENTS ${OPENDAQ_PYTHON_COMPONENTS})
    #help pybind11 find the correct python version
    set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})
endif()

find_package(Filesystem REQUIRED COMPONENTS Final Experimental)

if (OPENDAQ_GENERATE_DELPHI_BINDINGS OR OPENDAQ_GENERATE_PYTHON_BINDINGS OR OPENDAQ_GENERATE_CSHARP_BINDINGS OR OPENDAQ_GENERATE_C_BINDINGS)
    set(OPENDAQ_GENERATE_BINDINGS On)
endif()

if(OPENDAQ_ENABLE_DELPHI_BINDINGS_TESTS OR OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS OR OPENDAQ_ENABLE_CSHARP_BINDINGS_TESTS)
    set(TEST_BINDINGS On)
endif()

set(OPENDAQ_GENERATE_BINDINGS_LANG "")
if(OPENDAQ_GENERATE_DELPHI_BINDINGS)
    message(STATUS "Generating Delphi bindings")
    set(OPENDAQ_GENERATE_BINDINGS_LANG ${OPENDAQ_GENERATE_BINDINGS_LANG} Delphi)
endif()

if(OPENDAQ_GENERATE_PYTHON_BINDINGS)
    message(STATUS "Generating Python bindings")
    set(OPENDAQ_GENERATE_BINDINGS_LANG ${OPENDAQ_GENERATE_BINDINGS_LANG} Python)
endif()

if (OPENDAQ_GENERATE_CSHARP_BINDINGS)
    message(STATUS "Generating C# bindings")
    set(OPENDAQ_GENERATE_BINDINGS_LANG ${OPENDAQ_GENERATE_BINDINGS_LANG} CSharp)
endif()

set(RTGEN_NAMESPACE ${SDK_TARGET_NAMESPACE})
include(RTGen)

if (NOT WIN32)
    set(MONO_C mono)
endif()

set(RTGEN "${CMAKE_CURRENT_SOURCE_DIR}/shared/tools/RTGen/bin/rtgen.exe" CACHE INTERNAL "RTGEN")

execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/.editorconfig ${CMAKE_BINARY_DIR})
execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format ${CMAKE_BINARY_DIR})

if (OPENDAQ_ENABLE_OPTIONAL_TESTS)
    set(OPENDAQ_ENABLE_OPTIONAL_TESTS ON CACHE BOOL "Enable optional (debugging) tests")

    message(STATUS "Optional tests enabled")
    add_compile_definitions(OPENDAQ_ENABLE_OPTIONAL_TESTS)
    set(OPENDAQ_TEST_COMPILE_DEFINES "OPENDAQ_ENABLE_OPTIONAL_TESTS=1")
endif()

if (OPENDAQ_ENABLE_UNSTABLE_TEST_LABELS)
    message(STATUS "Labeling unstable tests enabled")
    add_compile_definitions(OPENDAQ_ENABLE_UNSTABLE_TEST_LABELS)
endif()

if (OPENDAQ_SKIP_UNSTABLE_TESTS)
    message(STATUS "Skipping unstable tests enabled")
    add_compile_definitions(OPENDAQ_SKIP_UNSTABLE_TESTS)
endif()

if (OPENDAQ_ENABLE_PARAMETER_VALIDATION)
    message(STATUS "Parameter validation enabled")
else()
    message(STATUS "Parameter validation disabled")
endif()

if (OPENDAQ_THREAD_SAFE)
    message(STATUS "Thread-safe implementations enabled")
else()
    message(STATUS "Thread-safe implementations disabled")
endif()

if (OPENDAQ_MIMALLOC_SUPPORT)
    message(STATUS "MiMalloc allocator enabled")
    add_compile_definitions(OPENDAQ_MIMALLOC_SUPPORT)
else()
    message(STATUS "MiMalloc allocator disabled")
endif()

if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    message(STATUS "Websocket streaming enabled")
    add_compile_definitions(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
else()
    message(STATUS "Websocket streaming disabled")
endif()

if (OPENDAQ_ENABLE_NATIVE_STREAMING)
    message(STATUS "Native streaming enabled")
    add_compile_definitions(OPENDAQ_ENABLE_NATIVE_STREAMING)
else()
    message(STATUS "Native streaming disabled")
endif()

# FIXME remove the following messages
message(STATUS "")
message(STATUS "========== 32/64-bit / toolchain configuration ==========")

message(STATUS "CMAKE_HOST_SYSTEM_NAME     = '${CMAKE_HOST_SYSTEM_NAME}'")
message(STATUS "CMAKE_SYSTEM_NAME          = '${CMAKE_SYSTEM_NAME}'")
message(STATUS "CMAKE_SYSTEM_PROCESSOR     = '${CMAKE_SYSTEM_PROCESSOR}'")

message(STATUS "CMAKE_C_FLAGS_INIT         = '${CMAKE_C_FLAGS_INIT}'")
message(STATUS "CMAKE_CXX_FLAGS_INIT       = '${CMAKE_CXX_FLAGS_INIT}'")
message(STATUS "CMAKE_ASM_FLAGS_INIT       = '${CMAKE_ASM_FLAGS_INIT}'")

message(STATUS "CMAKE_C_FLAGS              = '${CMAKE_C_FLAGS}'")
message(STATUS "CMAKE_CXX_FLAGS            = '${CMAKE_CXX_FLAGS}'")
message(STATUS "CMAKE_ASM_FLAGS            = '${CMAKE_ASM_FLAGS}'")

message(STATUS "CMAKE_LIBRARY_PATH         = '${CMAKE_LIBRARY_PATH}'")
message(STATUS "CMAKE_SIZEOF_VOID_P        = '${CMAKE_SIZEOF_VOID_P}'")

message(STATUS "======================================================")
message(STATUS "")

opendaq_setup_compiler_flags(${SDK_OPTION_PREFIX})

# libraries needed before the core is read
include(openDAQModuleFetchUtils)
if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE))
    set(DAQMODULES_LT_ENABLE_TESTS ${OPENDAQ_ENABLE_TESTS} CACHE BOOL "")
    set(DAQMODULES_LT_ENABLE_CLIENT ${DAQMODULES_OPENDAQ_CLIENT_MODULE} CACHE BOOL "")
    set(DAQMODULES_LT_ENABLE_SERVER ${DAQMODULES_OPENDAQ_SERVER_MODULE} CACHE BOOL "")
    if (DAQMODULES_LT_LEGACY_MODULES)
        opendaq_fetch_module(
            NAME                LtStreamingModulesLegacy
            GIT_REPOSITORY      https://github.com/openDAQ/LTStreamingModulesLegacy.git
            GIT_REF             other/complete-missing-init-files
            GIT_SHALLOW         OFF
        )
        FetchContent_GetProperties(
            LtStreamingModulesLegacy
            SOURCE_DIR LT_MODULES_SOURCE_DIR
            BINARY_DIR LT_MODULES_BINARY_DIR
        )
    else()
        opendaq_fetch_module(
            NAME                LtStreamingModulesModern
            GIT_REPOSITORY      https://github.com/openDAQ/LTStreamingModulesModern.git
            GIT_REF             other/complete-missing-init-files
            GIT_SHALLOW         OFF
        )
        FetchContent_GetProperties(
            LtStreamingModulesModern
            SOURCE_DIR LT_MODULES_SOURCE_DIR
            BINARY_DIR LT_MODULES_BINARY_DIR
        )
    endif()
endif()

if (OPENDAQ_ENABLE_OPCUA AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE))
    set(DAQMODULES_OPCUA_ENABLE_TESTS ${OPENDAQ_ENABLE_TESTS} CACHE BOOL "")
    set(DAQMODULES_OPCUA_ENABLE_CLIENT ${DAQMODULES_OPENDAQ_CLIENT_MODULE} CACHE BOOL "")
    set(DAQMODULES_OPCUA_ENABLE_SERVER ${DAQMODULES_OPENDAQ_SERVER_MODULE} CACHE BOOL "")
    opendaq_fetch_module(
        NAME                OpcUaModules
        GIT_REPOSITORY      https://github.com/openDAQ/OpcUaModules.git
        GIT_REF             other/complete-missing-files
        GIT_SHALLOW         OFF
    )
    FetchContent_GetProperties(
        OpcUaModules
        SOURCE_DIR OPCUA_MODULES_SOURCE_DIR
        BINARY_DIR OPCUA_MODULES_BINARY_DIR
    )
endif()

set(OPENDAQ_MODULE_EVAL_EXPRS
    "DAQMODULES_EMPTY_MODULE"
    "DAQMODULES_REF_FB_MODULE"
    "OPENDAQ_ENABLE_WEBSOCKET_STREAMING AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE)"
    "OPENDAQ_ENABLE_OPCUA AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE)"
)

set(OPENDAQ_MODULE_DIRS
    "${PROJ_DIR}/examples/modules/empty_module"
    "${PROJ_DIR}/examples/modules/ref_fb_module"
    "${LT_MODULES_SOURCE_DIR}"
    "${OPCUA_MODULES_SOURCE_DIR}"
)

foreach(OPENDAQ_MODULE_EVAL_EXPR OPENDAQ_MODULE_DIR IN ZIP_LISTS OPENDAQ_MODULE_EVAL_EXPRS OPENDAQ_MODULE_DIRS)
    if (${OPENDAQ_MODULE_EVAL_EXPR})
        if (NOT IS_DIRECTORY "${OPENDAQ_MODULE_DIR}")
            message(FATAL_ERROR "Specified module directory ${OPENDAQ_MODULE_DIR} does not exist")
        endif()
        include("${OPENDAQ_MODULE_DIR}/external/boost/Boost.cmake" OPTIONAL)
    endif()
endforeach()
include("${PROJ_DIR}/external/boost/Boost.cmake")
opendaq_complete_boost_dependency()

add_subdirectory(external)
# external dependencies required for core are completed at this point

add_subdirectory(core)
add_subdirectory(modules)
if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE))
    add_subdirectory("${LT_MODULES_SOURCE_DIR}" "${LT_MODULES_BINARY_DIR}")
endif()
if (OPENDAQ_ENABLE_OPCUA AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE))
    add_subdirectory("${OPCUA_MODULES_SOURCE_DIR}" "${OPCUA_MODULES_BINARY_DIR}")
endif()
add_subdirectory(shared)
add_subdirectory(examples)
add_subdirectory(bindings)
add_subdirectory(docs)
add_subdirectory(simulator)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()

if (NOT WIN32)
    set(INSTALLED_MODULES_DIR ${CMAKE_INSTALL_LIBDIR})
else()
    set(INSTALLED_MODULES_DIR ${CMAKE_INSTALL_BINDIR})
endif()

set(INSTALLED_CMAKE_CONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/cmake)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/${SDK_NAME}Config.cmake.in ${SDK_NAME}Config.cmake
    INSTALL_DESTINATION ${INSTALLED_CMAKE_CONFIG_DIR}/${SDK_TARGET_NAME}
    PATH_VARS
        CMAKE_INSTALL_DATAROOTDIR
        INSTALLED_MODULES_DIR
        INSTALLED_CMAKE_CONFIG_DIR
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(${SDK_NAME}ConfigVersion.cmake
  VERSION ${OPENDAQ_PACKAGE_VERSION}
  COMPATIBILITY ExactVersion )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SDK_NAME}Config.cmake
        DESTINATION ${INSTALLED_CMAKE_CONFIG_DIR}/${SDK_TARGET_NAME}
        COMPONENT ${SDK_NAME}_Development
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SDK_NAME}ConfigVersion.cmake
        DESTINATION ${INSTALLED_CMAKE_CONFIG_DIR}/${SDK_TARGET_NAME}
        COMPONENT ${SDK_NAME}_Development
)

install(FILES cmake/DeprecatedCommands.cmake
        DESTINATION share/cmake
        COMPONENT ${SDK_NAME}_Development
)
FetchContent_GetProperties(
    opendaq-cmake-utils
    SOURCE_DIR OPENDAQ_CMAKE_UTILS_SOURCE_DIR
)
install(FILES ${OPENDAQ_CMAKE_UTILS_SOURCE_DIR}/cmake/openDAQBaseUtils.cmake
        DESTINATION share/cmake
        COMPONENT ${SDK_NAME}_Development
)
install(FILES ${OPENDAQ_CMAKE_UTILS_SOURCE_DIR}/cmake/version/version.h.in ${OPENDAQ_CMAKE_UTILS_SOURCE_DIR}/cmake/version/version.rc.in
        DESTINATION share/cmake/version
        COMPONENT ${SDK_NAME}_Development
)

install(EXPORT ${SDK_NAME}
        NAMESPACE ${SDK_TARGET_NAMESPACE}::
        FILE ${SDK_NAME}.cmake
        DESTINATION ${INSTALLED_CMAKE_CONFIG_DIR}/${SDK_TARGET_NAME}
        COMPONENT ${SDK_NAME}_Development
)

if (DEFINED mdns_SOURCE_DIR)
    message(STATUS "mdns dependency found - mdns.h will be installed from: ${mdns_SOURCE_DIR}")
    install(
        FILES "${mdns_SOURCE_DIR}/mdns.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
else()
    message(ERROR "missing mdns dependency")
endif()

set(OPENDAQ_CONFIGURED_ONCE TRUE CACHE INTERNAL "A flag showing that CMake has configured at least once.")

include(cmake/Packing.cmake)
