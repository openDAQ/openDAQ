set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.24)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_FOLDER "${CMAKE_FOLDER}/openDAQ")

set(SDK_NAME openDAQ)
set(SDK_TARGET_NAME opendaq)
set(SDK_TARGET_NAMESPACE daq)
set(SDK_OPTION_PREFIX OPENDAQ)

add_subdirectory(cmake)
setup_repo_version("OPENDAQ_PACKAGE" ${SDK_NAME} "opendaq_version")

project(${SDK_NAME}
    LANGUAGES CXX
    VERSION ${OPENDAQ_PACKAGE_VERSION}
)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake
)

setup_build_mode(${SDK_OPTION_PREFIX} ${SDK_NAME})
setup_repo(${SDK_OPTION_PREFIX})

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

if (NOT MSVC)
    option(OPENDAQ_FORCE_COMPILE_32BIT "Compile 32Bit on non MSVC" OFF)
endif()

if (MSVC)
    option(OPENDAQ_MSVC_SINGLE_PROCESS_BUILD "Do not include /MP compile option." OFF)
endif()

# Feature options
option(OPENDAQ_ENABLE_PARAMETER_VALIDATION "Enable parameter validation in functions" ON)
option(OPENDAQ_ENABLE_WEBSOCKET_STREAMING "Enable ${SDK_NAME} websocket streaming" OFF)
cmake_dependent_option(DAQMODULES_LT_LEGACY_MODULES "Use legacy LT streaming modules" OFF "OPENDAQ_ENABLE_WEBSOCKET_STREAMING" OFF)
option(OPENDAQ_THREAD_SAFE "Enable thread-safe implementations where available" ON)
option(OPENDAQ_MIMALLOC_SUPPORT "Enable MiMalloc-based packet allocator" OFF)
option(OPENDAQ_ENABLE_NATIVE_STREAMING "Enable ${SDK_NAME} native streaming" OFF)
option(OPENDAQ_ENABLE_ACCESS_CONTROL "Enable object-level access control" ON)

option(OPENDAQ_ENABLE_OPCUA "Enable OpcUa" OFF)
cmake_dependent_option(OPCUA_ENABLE_ENCRYPTION "Enable OpcUa encryption" OFF "OPENDAQ_ENABLE_OPCUA" OFF) # TODO move to module

# Bindings
option(OPENDAQ_GENERATE_DELPHI_BINDINGS "Generate Delphi bindings" OFF)
option(OPENDAQ_GENERATE_PYTHON_BINDINGS "Generate Python bindings" OFF)
option(OPENDAQ_GENERATE_CSHARP_BINDINGS "Generate CSharp bindings" OFF)
option(OPENDAQ_GENERATE_C_BINDINGS "Generate C bindings" OFF)

# Runtime and default 3rd party library linking options
option(OPENDAQ_LINK_RUNTIME_STATICALLY "Link the C++ runtime staticaly (embedd it)" OFF)
option(OPENDAQ_LINK_3RD_PARTY_LIBS_STATICALY "Link the 3rd party libraries staticaly (embedd it)" ON)

# Testing and coverage options
option(OPENDAQ_ENABLE_TEST_UTILS "Enable testing utils library" ON)
option(OPENDAQ_ENABLE_OPTIONAL_TESTS "Enable optional (debugging) tests" OFF)
option(OPENDAQ_ENABLE_COVERAGE "Enable code coverage in testing" OFF)
option(OPENDAQ_ENABLE_REGRESSION_TESTS "Enable regression testing" OFF)
option(OPENDAQ_ENABLE_UNSTABLE_TEST_LABELS "Enable labeling unstable tests" OFF)

# Additional build options
option(OPENDAQ_USE_CCACHE "Use compiler cache driver if available" ON)
option(OPENDAQ_ENABLE_ERROR_GUARD "Enable error guard" OFF)

if(NOT OPENDAQ_ENABLE_ERROR_GUARD AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OPENDAQ_ENABLE_ERROR_GUARD ON CACHE BOOL "Enable error guard" FORCE)
endif()

if(OPENDAQ_ENABLE_ERROR_GUARD)
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
log_level_str_to_int(${OPENDAQ_LOG_LEVEL_DEBUG} OPENDAQ_LOG_LEVEL_DEBUG_INT)

set(OPENDAQ_LOG_LEVEL_RELEASE "Info" CACHE STRING "Compile time logging level for release builds")
set_property(CACHE OPENDAQ_LOG_LEVEL_RELEASE PROPERTY STRINGS ${OPENDAQ_LOG_LEVELS})
log_level_str_to_int(${OPENDAQ_LOG_LEVEL_RELEASE} OPENDAQ_LOG_LEVEL_RELEASE_INT)

add_compile_definitions(OPENDAQ_LOG_LEVEL=$<IF:$<CONFIG:Debug>,${OPENDAQ_LOG_LEVEL_DEBUG_INT},${OPENDAQ_LOG_LEVEL_RELEASE_INT}>)
# logging end

set(OPENDAQ_REPO_PREFIX "https://github.com/openDAQ" CACHE STRING "Set this if using a repository mirror")
message(STATUS "${SDK_NAME} repository prefix: ${OPENDAQ_REPO_PREFIX}")

set(OPENDAQ_SOURCE_DIR ${PROJECT_SOURCE_DIR})

if (NOT OPENDAQ_CI_RUNNER)
    set(OPENDAQ_CI_RUNNER OFF)
endif()

if (OPENDAQ_CI_RUNNER)
    message("Configured as CI runner")
    add_compile_definitions(DS_CI_RUNNER=1)
    set(OPENDAQ_DEBUG_WARNINGS_AS_ERRORS ON)
endif()

if (COMMAND get_mode)
    get_mode(CMAKE_MODE)
    message(STATUS "CMake Mode is ${CMAKE_MODE}")
endif()

include(DaqInternal)

opendaq_is_64bit_build(BUILD_64Bit)
set(BUILD_64Bit ${BUILD_64Bit} CACHE INTERNAL "Build in 64-bit mode")

if(BUILD_64Bit OR BUILD_ARM)
    set(OPENDAQ_FPIC On CACHE INTERNAL "Set Position independent code flag")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    set(OPENDAQ_FPIC Off CACHE INTERNAL "Set Position independent code flag")
endif()

if (NOT OPENDAQ_BUILDING_AS_SUBMODULE)
    message(STATUS "Setting -fPIC to ${OPENDAQ_FPIC}")
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

# module suffix
if (NOT DEFINED OPENDAQ_MODULE_SUFFIX)
    set(OPENDAQ_MODULE_SUFFIX ".module${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

if (NOT DEFINED CACHE{OPENDAQ_MODULE_SUFFIX})
    set(OPENDAQ_MODULE_SUFFIX ${OPENDAQ_MODULE_SUFFIX} CACHE INTERNAL "Default DLL/SO extension")
endif()

message(STATUS "Default Module extension: ${OPENDAQ_MODULE_SUFFIX}")
add_compile_definitions(OPENDAQ_MODULE_SUFFIX="${OPENDAQ_MODULE_SUFFIX}")

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

if(OPENDAQ_GENERATE_PYTHON_BINDINGS OR OPENDAQ_ENABLE_COVERAGE) #gcc coverage
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

if(OPENDAQ_GENERATE_DELPHI_BINDINGS OR OPENDAQ_GENERATE_PYTHON_BINDINGS OR OPENDAQ_GENERATE_CSHARP_BINDINGS OR OPENDAQ_GENERATE_C_BINDINGS)
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

if(OPENDAQ_GENERATE_CSHARP_BINDINGS)
    message(STATUS "Generating C# bindings")
    set(OPENDAQ_GENERATE_BINDINGS_LANG ${OPENDAQ_GENERATE_BINDINGS_LANG} CSharp)
endif()

if(OPENDAQ_LINK_RUNTIME_STATICALLY)
    opendaq_set_runtime(STATIC)
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

# libraries needed before the core is read
add_subdirectory(external)

add_subdirectory(core)
add_subdirectory(modules)
add_subdirectory(shared)
add_subdirectory(examples)
add_subdirectory(bindings)
add_subdirectory(docs)
add_subdirectory(simulator)

if (NOT OPENDAQ_BUILDING_AS_SUBMODULE)
    # fetch boost at latest point when list of required boost libs and headers is completed
    opendaq_setup_boost()
endif()

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
else ()
    message(ERROR "missing mdns dependency")
endif()

set(OPENDAQ_CONFIGURED_ONCE TRUE CACHE INTERNAL "A flag showing that CMake has configured at least once.")

include(cmake/Packing.cmake)
