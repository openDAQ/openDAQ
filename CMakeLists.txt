cmake_minimum_required(VERSION 3.24)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_FOLDER "${CMAKE_FOLDER}/openDAQ")

set(SDK_NAME openDAQ)
set(SDK_TARGET_NAME opendaq)
set(SDK_TARGET_NAMESPACE daq)
set(SDK_OPTION_PREFIX OPENDAQ)

file(READ "opendaq_version" package_version)
string(STRIP ${package_version} package_version)
set(OPENDAQ_PACKAGE_VERSION "${package_version}")
message(STATUS "openDAQ package version: ${package_version}")

set(CMAKE_MESSAGE_CONTEXT ${SDK_NAME})
set(CMAKE_MESSAGE_CONTEXT_SHOW ON CACHE BOOL "Show CMake message context")

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source build is not supported! Please choose a separate build directory e.g.: /build/x64/msvc")
endif()

project(${SDK_NAME}
    LANGUAGES CXX
    VERSION ${OPENDAQ_PACKAGE_VERSION}
)
add_compile_definitions(OPENDAQ_PACKAGE_VERSION="${OPENDAQ_PACKAGE_VERSION}")

include(CheckCompilerFlag)
include(CheckLinkerFlag)
include(CMakeDependentOption)
include(GNUInstallDirs)

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

if (POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif()

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if  (POLICY CMP0076)
     cmake_policy(SET CMP0076 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake
)

# All install targets from within the project should eventually specify the component name.
# This changes the default name for all the targets without an explicit name ("Unspecified").
# The component "Unspecified" is a bit special and makes it hard to be excluded.
# Ideally we would just avoid installing all external components and we wouldn't need setting this at all.
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "External")

if(APPLE)
    set(LIB_LOAD_ORIGIN @loader_path)
else()
    set(LIB_LOAD_ORIGIN $ORIGIN)
endif()

file(RELATIVE_PATH RPATH_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
)

# allow CMAKE_INSTALL_RPATH to be cleared from the command line
if (NOT DEFINED CMAKE_INSTALL_RPATH)
    set(CMAKE_INSTALL_RPATH ${LIB_LOAD_ORIGIN} ${LIB_LOAD_ORIGIN}/${RPATH_DIR})
endif()

if (NOT MSVC)
    option(OPENDAQ_FORCE_COMPILE_32BIT "Compile 32Bit on non MSVC" OFF)
endif()

if (MSVC)
    option(OPENDAQ_MSVC_SINGLE_PROCESS_BUILD "Do not include /MP compile option." OFF)
endif()

# Feature options
option(OPENDAQ_ENABLE_PARAMETER_VALIDATION "Enable parameter validation in functions" ON)
option(OPENDAQ_ENABLE_APPLICATION "Enable building of ${SDK_NAME} application" OFF)
option(OPENDAQ_ENABLE_WEBSOCKET_STREAMING "Enable ${SDK_NAME} websocket streaming" OFF)
option(OPENDAQ_THREAD_SAFE "Enable thread-safe implementations where available" ON)
option(OPENDAQ_MIMALLOC_SUPPORT "Enable MiMalloc-based packet allocator" OFF)
option(OPENDAQ_ENABLE_NATIVE_STREAMING "Enable ${SDK_NAME} native streaming" OFF)
option(OPENDAQ_ALWAYS_FETCH_DEPENDENCIES "Ignore any installed libraries and always build all dependencies from source" ON)

option(OPENDAQ_ENABLE_OPCUA "Enable OpcUa" OFF)
cmake_dependent_option(OPCUA_ENABLE_ENCRYPTION "Enable OpcUa encryption" OFF "OPENDAQ_ENABLE_OPCUA" OFF)

# Bindings
option(OPENDAQ_GENERATE_DELPHI_BINDINGS "Generate Delphi bindings" OFF)
option(OPENDAQ_GENERATE_PYTHON_BINDINGS "Generate Python bindings" OFF)

# Runtime and default 3rd party library linking
option(OPENDAQ_LINK_RUNTIME_STATICALLY "Link the C++ runtime staticaly (embedd it)" OFF)
option(OPENDAQ_LINK_3RD_PARTY_LIBS_STATICALY "Link the 3rd party libraries staticaly (embedd it)" ON)

# Testing and coverage
option(OPENDAQ_ENABLE_TESTS "Enable testing" ON)
option(OPENDAQ_ENABLE_OPTIONAL_TESTS "Enable optional (debugging) tests" OFF)
option(OPENDAQ_ENABLE_COVERAGE "Enable code coverage in testing" OFF)

# Additional build options
option(OPENDAQ_DISABLE_DEBUG_POSTFIX "Disable debug ('-debug') postfix" OFF)
option(OPENDAQ_DEBUG_WARNINGS_AS_ERRORS "Treat debug warnings as errors" OFF)
option(OPENDAQ_RELEASE_WARNINGS_AS_ERRORS "Treat release warnings as errors" ON)
option(OPENDAQ_USE_CCACHE "Use compiler cache driver if available" ON)

# Dependent options
cmake_dependent_option(OPENDAQ_ENABLE_DELPHI_BINDINGS_TESTS "Enable delphi bindings tests" ON "OPENDAQ_GENERATE_DELPHI_BINDINGS;OPENDAQ_ENABLE_TESTS" OFF)
cmake_dependent_option(OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS "Enable python bindings tests" ON "OPENDAQ_GENERATE_PYTHON_BINDINGS;OPENDAQ_ENABLE_TESTS" OFF)

# logging

include(openDAQLogging)

set(OPENDAQ_LOG_LEVEL_DEBUG "Debug" CACHE STRING "Compile time logging level for debug builds")
set_property(CACHE OPENDAQ_LOG_LEVEL_DEBUG PROPERTY STRINGS ${OPENDAQ_LOG_LEVELS})
log_level_str_to_int(${OPENDAQ_LOG_LEVEL_DEBUG} OPENDAQ_LOG_LEVEL_DEBUG_INT)

set(OPENDAQ_LOG_LEVEL_RELEASE "Info" CACHE STRING "Compile time logging level for release builds")
set_property(CACHE OPENDAQ_LOG_LEVEL_RELEASE PROPERTY STRINGS ${OPENDAQ_LOG_LEVELS})
log_level_str_to_int(${OPENDAQ_LOG_LEVEL_RELEASE} OPENDAQ_LOG_LEVEL_RELEASE_INT)

add_compile_definitions(OPENDAQ_LOG_LEVEL=$<IF:$<CONFIG:Debug>,${OPENDAQ_LOG_LEVEL_DEBUG_INT},${OPENDAQ_LOG_LEVEL_RELEASE_INT}>)

set(OPENDAQ_REPO_PREFIX "https://github.com/openDAQ" CACHE STRING "Set this if using a repository mirror")
message(STATUS "${SDK_NAME} repository prefix: ${OPENDAQ_REPO_PREFIX}")

get_filename_component(ROOT_DIR ${CMAKE_SOURCE_DIR} REALPATH)
get_filename_component(PROJ_DIR ${PROJECT_SOURCE_DIR} REALPATH)

if (NOT ${PROJECT_SOURCE_DIR} STREQUAL ${ROOT_DIR})
    set(BUILDING_AS_SUBMODULE ON)
    message(STATUS "Building as submodule")
else()
    message(STATUS "Building standalone")
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".CMakePredefinedTargets")
endif()

include(CMakePrintHelpers)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(OPENDAQ_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(FETCHCONTENT_EXTERNALS_DIR ${ROOT_DIR}/build/__external CACHE PATH "FetchContent folder prefix")

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

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_COMPILER_IS_CLANGXX On CACHE INTERNAL "Compiler is LLVM Clang")
endif ()

if (UNIX AND (CMAKE_COMPILER_IS_CLANGXX OR CMAKE_COMPILER_IS_GNUXX))
    # hide all symbols expect those specifically exported with PUBLIC_EXPORT macro
    if (APPLE)
      # not possible on Mac, should be done per library
    else()
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,ALL")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
endif()

if (MSVC AND NOT (${CMAKE_GENERATOR} MATCHES "Visual Studio"))
    find_package(WindowsSDK)

    if (WINDOWSSDK_FOUND)
        get_windowssdk_include_dirs(${WINDOWSSDK_LATEST_DIR} WIN_SDK_INCLUDE)
        get_windowssdk_library_dirs(${WINDOWSSDK_LATEST_DIR} WIN_SDK_LIBRARIES)

        include_directories(${WIN_SDK_INCLUDE})
        link_directories(${WIN_SDK_LIBRARIES})
    else()
        message(WARNING "Windows SDK not found. Some projects might not build or link correctly.")
    endif()
endif()

if (NOT BUILDING_AS_SUBMODULE)
    find_package(GitInfo)
    if (COMMAND GIT_WC_INFO)
       GIT_WC_INFO("${CMAKE_CURRENT_SOURCE_DIR}" "OPENDAQ")
       message(STATUS "Git branch: ${OPENDAQ_WC_BRANCH_NAME} | ${OPENDAQ_WC_REVISION_HASH}")
    else()
        set(OPENDAQ_WC_REVISION "NA")
    endif()
endif()

string(TIMESTAMP CONFIGURE_DATE)
string(TIMESTAMP CURRENT_YEAR "%Y")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (UNIX AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm.*$")  #e.g. armv7l
    set(BUILD_ARM On CACHE INTERNAL "Build for ARM architecture")
endif()

if (UNIX AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^aarch.*$")  #e.g. aarch64
    set(BUILD_ARM On CACHE INTERNAL "Build for ARM architecture")
endif()

include(DaqUtils)

opendaq_is_64bit_build(BUILD_64Bit)
set(BUILD_64Bit ${BUILD_64Bit} CACHE INTERNAL "Build in 64-bit mode")

if(BUILD_64Bit OR BUILD_ARM)
    set(OPENDAQ_FPIC On CACHE INTERNAL "Set Position independent code flag")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    set(OPENDAQ_FPIC Off CACHE INTERNAL "Set Position independent code flag")
endif()

if (NOT BUILDING_AS_SUBMODULE)
    message(STATUS "Platform: ${CMAKE_SYSTEM_PROCESSOR} | ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
    message(STATUS "Generator: ${CMAKE_GENERATOR} | ${CMAKE_GENERATOR_PLATFORM}")
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "Setting -fPIC to ${OPENDAQ_FPIC}")
endif()

if(NOT CMAKE_DEBUG_POSTFIX AND NOT OPENDAQ_DISABLE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX -debug)
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
    enable_testing()

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

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    add_compile_definitions(NOMINMAX
                            _WIN32_WINNT=0x0601 # Windows 7 Compat
    )

    add_compile_definitions(UNICODE _UNICODE)
endif()

if (NOT COMMAND set_mode)
    message(STATUS "Including Modern.cmake")
    include(Modern)
else()
    set_mode(MODERN)
endif()

# module suffix
set(OPENDAQ_MODULE_SUFFIX ".module${CMAKE_SHARED_LIBRARY_SUFFIX}")
add_compile_definitions(OPENDAQ_MODULE_SUFFIX="${OPENDAQ_MODULE_SUFFIX}")

if ((CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX) AND NOT MSVC)
    option(OPENDAQ_FORCE_LLD_LINKER "Force the use of the fast LLVM LLD linker" OFF)

    if (NOT WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    endif()

    if(OPENDAQ_FORCE_COMPILE_32BIT)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        set(BUILD_64Bit OFF)
    endif()

    # The flag -fuse-ld=lld is needed on MinGW where the default linker is hardly usable.
    # It is not available on macOS (it is ignored though, unless you pass -Werror).
    # It might be default on Linux distros, but it might not be always available.
    # The following code just says "use LLD whenever it seems feasible".
    check_linker_flag(CXX "-fuse-ld=lld" COMPILER_SUPPORTS_LLD_LINKER)
    if(COMPILER_SUPPORTS_LLD_LINKER)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
        message(STATUS "LLD Linker enabled")
    endif()
endif()

if (CMAKE_COMPILER_IS_GNUCXX)
    set(GCC_W_NO_EXTRA "-Wno-comment -Wno-unused-parameter -Wno-missing-field-initializers")
    set(GCC_W_NO_WALL "-Wno-unknown-pragmas -Wno-parentheses -Wno-misleading-indentation -Wno-unused-variable -Wno-switch -Wno-maybe-uninitialized -Wno-psabi")
    set(GCC_CHARSET_FLAGS "-fexec-charset=UTF-8 -finput-charset=UTF-8")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra ${GCC_W_NO_EXTRA} ${GCC_W_NO_WALL} ${GCC_W_NO_PEDANTIC} -Werror=return-type")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra ${GCC_W_NO_EXTRA} ${GCC_W_NO_WALL} ${GCC_W_NO_PEDANTIC} -Werror=return-type ${GCC_CHARSET_FLAGS}")

    # Treat warnings as errors if OPENDAQ_<DEBUG|RELEASE>_WARNINGS_AS_ERRORS is ON
    add_compile_options($<$<OR:$<AND:$<CONFIG:Debug>,$<BOOL:${OPENDAQ_DEBUG_WARNINGS_AS_ERRORS}>>,$<AND:$<NOT:$<CONFIG:Debug>>,$<BOOL:${OPENDAQ_RELEASE_WARNINGS_AS_ERRORS}>>>:-Werror>)

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -ggdb")
    if (MINGW)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-all-symbols")
    endif()

    if (APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error")
    else()
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
    endif()

    if (OPENDAQ_ENABLE_COVERAGE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage -fno-elide-constructors")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    endif()

    if (WIN32 AND NOT BUILD_64Bit)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse -msse2")
    endif()

    if (NOT (WIN32 OR APPLE))
        if (NOT DEFINED HAVE_SPLIT_DWARF)
            check_compiler_flag(CXX "-gsplit-dwarf" HAVE_SPLIT_DWARF)
        endif()

        if (HAVE_SPLIT_DWARF)
            # Only add for debug builds, but could also expand the
            # generator expression to add for RelWithDebInfo too
            add_compile_options("$<$<CONFIG:Debug>:-gsplit-dwarf>")
        endif()
    endif()

    if (OPENDAQ_FORCE_LLD_LINKER)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=lld")
    endif()
endif()

if(CMAKE_COMPILER_IS_CLANGXX)
    # MinGW Clang does not support TLS so try emulating it
    if (MINGW)
        add_compile_options(-femulated-tls)
    endif()
endif()

if (OPENDAQ_FORCE_LLD_LINKER)
    message(STATUS "Forcing the use of LLVM LLD linker. Make sure it is installed and available.")
endif()

if (MSVC)
    # As above CMAKE_CXX_STANDARD but for VS
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/std:c++17>)

    foreach (flag IN ITEMS
        # Set source and execution character sets to UTF-8
        # https://learn.microsoft.com/en-us/cpp/build/reference/utf-8-set-source-and-executable-character-sets-to-utf-8
        /utf-8
        # Display level 1, level 2, and level 3 warnings, and all level 4 (informational) warnings that aren't off by default.
        # https://learn.microsoft.com/en-us/cpp/build/reference/compiler-option-warning-level
        /W4
        # data member 'member1' will be initialized after data member 'member2'
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/c5038
        /w15038
        # Supress warnings
        # https://learn.microsoft.com/en-us/cpp/build/reference/compiler-option-warning-level
        #
        # 'class1' : inherits 'class2::member' via dominance
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4250
        /wd4250
        # Your code uses a function, class member, variable, or typedef that's marked deprecated.
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996
        /wd4996
        # declaration of 'identifier' hides class member
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4458
        /wd4458
        # nonstandard extension used : nameless struct/union
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4201
        /wd4201
        # unreachable code
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4702
        /wd4702
        # declaration of 'identifier' hides global declaration
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4459
        /wd4459
        # 'function' : unreferenced local function has been removed
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4505
        /wd4505
        # conditional expression is constant
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
        /wd4127
        # assignment within conditional expression
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4706
        /wd4706
        # loss of data / precision, unsigned <--> signed
        #
        # 'argument' : conversion from 'type1' to 'type2', possible loss of data
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4244
        /wd4244
        # 'var' : conversion from 'size_t' to 'type', possible loss of data
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4267
        /wd4267
    )
        add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:${flag}>)
    endforeach()

    if (NOT OPENDAQ_MSVC_SINGLE_PROCESS_BUILD)
        # Build with multiple processes
        # https://learn.microsoft.com/en-us/cpp/build/reference/mp-build-with-multiple-processes
        add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/MP>)
    endif()

    # Treat warnings as errors if OPENDAQ_<DEBUG|RELEASE>_WARNINGS_AS_ERRORS is ON
    add_compile_options($<$<OR:$<AND:$<CONFIG:Debug>,$<BOOL:${OPENDAQ_DEBUG_WARNINGS_AS_ERRORS}>>,$<AND:$<NOT:$<CONFIG:Debug>>,$<BOOL:${OPENDAQ_RELEASE_WARNINGS_AS_ERRORS}>>>:/WX>)

    if (MSVC_VERSION GREATER_EQUAL 1910)
        # /Zc:__cplusplus forces MSVC to use the correct value of __cplusplus macro (otherwise always C++98)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            # /Zf (Faster PDB generation) is not supported by ClangCL
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zf")
        endif()

        # Produce diagnostic messages with exact location
        add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/diagnostics:caret>)
    endif()

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221")

    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO    "${CMAKE_EXE_LINKER_FLAGS}    /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_MODULE_LINKER_FLAGS} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_STATIC_LINKER_FLAGS} /DEBUG /OPT:REF /OPT:ICF")

    if(OPENDAQ_ENABLE_COVERAGE)
        # /PROFILE add profiler info to PDB
        # /OPT:NOREF do not omit unreferenced files/functions/classes in the PDB (would hide completely untested code)

        #set(CMAKE_EXE_LINKER_FLAGS_DEBUG    "${CMAKE_EXE_LINKER_FLAGS}    /PROFILE /OPT:NOREF")
        set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS} /PROFILE /OPT:NOREF")
        set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS} /PROFILE /OPT:NOREF")
        set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS} /PROFILE /OPT:NOREF")
    endif()
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
    set(PYTHON_VERSION 3.6)
    set(COMPONENTS Interpreter)
    if(OPENDAQ_GENERATE_PYTHON_BINDINGS)
        list(APPEND COMPONENTS Development.Module)
    endif()
    set(OPENDAQ_PYTHON_VERSION "" CACHE STRING "Exact Python version to use for bindings generation")
    if(OPENDAQ_PYTHON_VERSION)
        set(PYTHON_VERSION ${OPENDAQ_PYTHON_VERSION})
        set(PYTHON_EXACT EXACT)
        set(PYTHON_REQUIRED REQUIRED)
    endif()

    find_package(Python ${PYTHON_VERSION} ${PYTHON_EXACT} ${PYTHON_REQUIRED} COMPONENTS ${COMPONENTS})
endif()

if (MSVC)
    set(FLEX_FLAGS "--wincompat")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Filesystem REQUIRED COMPONENTS Final Experimental)

if(OPENDAQ_GENERATE_DELPHI_BINDINGS OR OPENDAQ_GENERATE_PYTHON_BINDINGS)
    set(OPENDAQ_GENERATE_BINDINGS On)
endif()

if(OPENDAQ_ENABLE_DELPHI_BINDINGS_TESTS OR OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS)
    set(TEST_BINDINGS On)
endif()

set(OPENDAQ_GENERATE_BINDINGS_LANG "")
if(OPENDAQ_GENERATE_DELPHI_BINDINGS)
    set(OPENDAQ_GENERATE_BINDINGS_LANG ${OPENDAQ_GENERATE_BINDINGS_LANG} Delphi)
endif()

if(OPENDAQ_GENERATE_PYTHON_BINDINGS)
    set(OPENDAQ_GENERATE_BINDINGS_LANG ${OPENDAQ_GENERATE_BINDINGS_LANG} Python)
endif()

if(OPENDAQ_LINK_RUNTIME_STATICALLY)
    opendaq_set_runtime(STATIC)
endif()

add_compile_definitions(BOOST_ALL_NO_LIB=1)

set(RTGEN_NAMESPACE ${SDK_TARGET_NAMESPACE})
include(RTGen)

set(CMAKE_INSTALL_DEBUG_LIBRARIES ON)
include(InstallRequiredSystemLibraries)

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

if (OPENDAQ_ENABLE_PARAMETER_VALIDATION)
    message(STATUS "Parameter validation enabled")
    add_compile_definitions(OPENDAQ_ENABLE_PARAMETER_VALIDATION)
else()
    message(STATUS "Parameter validation disabled")
endif()

if (OPENDAQ_THREAD_SAFE)
    message(STATUS "Thread-safe implementations enabled")
    add_compile_definitions(OPENDAQ_THREAD_SAFE)
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

use_compiler_cache()

if (WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
endif()

# libraries needed before the core is read
add_subdirectory(external)

if(CMAKE_COMPILER_IS_CLANGXX)
    set(CLANG_FLAGS "\
        -Wno-unused-variable \
        -Wno-missing-braces \
        -Wno-unused-function \
        -Wno-logical-op-parentheses \
        -Wno-unused-variable \
        -Werror=return-type \
        -Wno-deprecated-declarations \
        -Wno-undef \
        -Wno-incompatible-pointer-types \
        -Wno-unused-parameter \
        -Wno-misleading-indentation \
        -Wno-missing-field-initializers"
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_FLAGS}")

    # Treat warnings as errors if OPENDAQ_<DEBUG|RELEASE>_WARNINGS_AS_ERRORS is ON
    add_compile_options($<$<OR:$<AND:$<CONFIG:Debug>,$<BOOL:${OPENDAQ_DEBUG_WARNINGS_AS_ERRORS}>>,$<AND:$<NOT:$<CONFIG:Debug>>,$<BOOL:${OPENDAQ_RELEASE_WARNINGS_AS_ERRORS}>>>:-Werror>)
endif()

add_subdirectory(core)
add_subdirectory(bindings)
add_subdirectory(modules)
add_subdirectory(shared)
add_subdirectory(app)
add_subdirectory(examples)
add_subdirectory(docs)
add_subdirectory(simulator)

if (NOT BUILDING_AS_SUBMODULE)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".CMakePredefinedTargets")
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

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SDK_NAME}Config.cmake
        DESTINATION ${INSTALLED_CMAKE_CONFIG_DIR}/${SDK_TARGET_NAME}
        COMPONENT ${SDK_NAME}_Development
)

install(FILES cmake/${SDK_NAME}Utils.cmake
        DESTINATION share/cmake
        COMPONENT ${SDK_NAME}_Development
)

install(EXPORT ${SDK_NAME}
        NAMESPACE ${SDK_TARGET_NAMESPACE}::
        FILE ${SDK_NAME}.cmake
        DESTINATION ${INSTALLED_CMAKE_CONFIG_DIR}/${SDK_TARGET_NAME}
        COMPONENT ${SDK_NAME}_Development
)

set(OPENDAQ_CONFIGURED_ONCE TRUE CACHE INTERNAL "A flag showing that CMake has configured at least once.")

include(cmake/Packing.cmake)
