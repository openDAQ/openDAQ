##
## General CPack configuration for Blueberry
##

set(CPACK_PACKAGE_NAME ${PROJECT_NAME} CACHE STRING "The package name")
set(CPACK_PACKAGE_VENDOR "openDAQ d.o.o.")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "openDAQ SDK for C++"
    CACHE STRING "Package description for the package metadata"
)
# or set CPACK_PACKAGE_DESCRIPTION_FILE

set(CPACK_PACKAGE_HOMEPAGE_URL "https://opendaq.io/")
set(CPACK_PACKAGE_CONTACT "info@opendaq.io")

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(CPACK_VERBATIM_VARIABLES ON)

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_SOURCE_DIR}/build/_packages")

if (UNIX)
    # https://unix.stackexchange.com/a/11552/254512
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/opendaq")
endif()

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# create one large package containing everything (TODO: Change to package per group when it makes sense)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

set(CPACK_THREADS 0)

##
## Package-specific settings
##

if(APPLE)
    # productbuild = native .pkg only (ZIP/TXZ are used on Linux/Windows CI).
    set(CPACK_GENERATOR productbuild)
    set(CPACK_PRODUCTBUILD_IDENTIFIER "io.opendaq.sdk" CACHE STRING "Bundle id for productbuild (.pkg)")
    # productbuild only accepts .txt, .rtf, .html, .rtfd for license/readme/welcome (not LICENSE or README.md).
    set(_CPACK_APPLE_STAGED "${CMAKE_BINARY_DIR}/CPack_apple_resources")
    file(MAKE_DIRECTORY "${_CPACK_APPLE_STAGED}")
    configure_file("${CMAKE_SOURCE_DIR}/LICENSE" "${_CPACK_APPLE_STAGED}/LICENSE.txt" COPYONLY)
    configure_file("${CMAKE_SOURCE_DIR}/README.md" "${_CPACK_APPLE_STAGED}/README.txt" COPYONLY)
    file(WRITE "${_CPACK_APPLE_STAGED}/WELCOME.txt" "openDAQ SDK\n")
    set(CPACK_RESOURCE_FILE_LICENSE "${_CPACK_APPLE_STAGED}/LICENSE.txt")
    set(CPACK_RESOURCE_FILE_README "${_CPACK_APPLE_STAGED}/README.txt")
    set(CPACK_RESOURCE_FILE_WELCOME "${_CPACK_APPLE_STAGED}/WELCOME.txt")
elseif(WIN32)
    set(CPACK_GENERATOR ZIP TXZ NSIS)
elseif(UNIX)
    set(CPACK_GENERATOR ZIP TXZ DEB)
else()
    set(CPACK_GENERATOR ZIP TXZ)
endif()

## Archive

# A monolithic zip would have been better, but it refuses to skip unwanted components
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

## NSIS

set(CPACK_NSIS_WELCOME_TITLE "Welcome to openDAQ SDK Setup")
set(CPACK_NSIS_BRANDING_TEXT "openDAQ d.o.o.")
set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/media/opendaq.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}\\media\\opendaq.ico")
# Paths must be in Windows format otherwise NSIS produces errors
set(CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_CURRENT_SOURCE_DIR}\\media\\nsis_header.bmp")
set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_CURRENT_SOURCE_DIR}\\media\\nsis_welcome.bmp")
set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

## Debian

set(CPACK_DEB_COMPONENT_INSTALL ON)

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "openDAQ SDK")

# Currently we bundle xxHash together in the one-large-package with all the dependencies bundled-in
# TODO: Properly define when we split the packages again
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxxhash-dev (>= 0.8.1)")

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
    set (CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
endif()

##
## Package filename customization
##
## Format: opendaq-<version>[-<sha>]-<arch>-<os>-<compiler>-<compiler-version>-<build-type>
## Field ordering follows GNU triplet convention (<arch>-<os>-…), values follow
## Conan settings vocabulary (arch, os, compiler). SHA is appended to the
## version for non-release builds to disambiguate dev/rc commits.
##

# OS name (Conan settings.os, lowercase)
string(TOLOWER "${CMAKE_SYSTEM_NAME}" _PACKING_OS_NAME)
if(_PACKING_OS_NAME STREQUAL "darwin")
    set(_PACKING_OS_NAME "macos")
endif()

# Architecture (Conan settings.arch)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
        set(_PACKING_ARCH_NAME "armv8")
    else()
        set(_PACKING_ARCH_NAME "x86_64")
    endif()
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM")
        set(_PACKING_ARCH_NAME "armv7")
    else()
        set(_PACKING_ARCH_NAME "x86")
    endif()
endif()

# Compiler (Conan settings.compiler) and major version
if(MSVC)
    set(_PACKING_COMPILER_ID "msvc")
    set(_PACKING_COMPILER_VER "${MSVC_TOOLSET_VERSION}")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(_PACKING_COMPILER_ID "gcc")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(_PACKING_COMPILER_ID "apple-clang")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(_PACKING_COMPILER_ID "clang")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel" OR CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
    set(_PACKING_COMPILER_ID "intel-cc")
else()
    string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" _PACKING_COMPILER_ID)
endif()
if(NOT MSVC)
    string(REGEX REPLACE "^([0-9]+).*" "\\1" _PACKING_COMPILER_VER "${CMAKE_CXX_COMPILER_VERSION}")
endif()

# Build type (lowercase). Empty on multi-config generators when CMAKE_BUILD_TYPE
# is not set at configure time; fall back to "release".
if(CMAKE_BUILD_TYPE)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" _PACKING_BUILD_TYPE)
else()
    set(_PACKING_BUILD_TYPE "release")
endif()

# Version comes from opendaq_version as-is (keeps dev/rc suffix). SHA is
# appended for non-release builds to disambiguate commits of the same version.
set(_PACKING_VERSION "${package_version}")
if(NOT OPENDAQ_IS_RELEASE_VERSION AND OPENDAQ_WC_REVISION_HASH)
    string(SUBSTRING "${OPENDAQ_WC_REVISION_HASH}" 0 7 _PACKING_SHORT_SHA)
    string(APPEND _PACKING_VERSION "-${_PACKING_SHORT_SHA}")
endif()

set(CPACK_PACKAGE_FILE_NAME
    "opendaq-${_PACKING_VERSION}-${_PACKING_ARCH_NAME}-${_PACKING_OS_NAME}-${_PACKING_COMPILER_ID}-${_PACKING_COMPILER_VER}-${_PACKING_BUILD_TYPE}"
)

##
## Finally ...
##

include(CPack)


## Install Groups

cpack_add_component_group(libopendaq
    DISPLAY_NAME
        "openDAQ"
    DESCRIPTION
        "openDAQ libraries"
    #EXPANDED
)

cpack_add_component_group(libopendaq-dev
    DISPLAY_NAME
        "openDAQ SDK headers"
    DESCRIPTION
        "openDAQ SDK headers for C++"
    #EXPANDED
)

cpack_add_component_group(libopendaq-external
    DISPLAY_NAME
        "External dependencies"
    DESCRIPTION
        "Temporarily required for a functional installation"
    #EXPANDED
)

## Install Components

cpack_add_component(openDAQ_Development
    HIDDEN
    GROUP
        libopendaq-dev
)
    
set(REQUIRED_COMPONENTS
    coretypes
    coreobjects
    corecontainers
    opendaq
)

set(OPTIONAL_COMPONENTS
    copendaq
    ref_device_module
    ref_fb_module
    audio_device_module
    opcua_server_module
    ws_stream_srv_module
    opcua_client_module
    ws_stream_cl_module
    native_stream_srv_module
    native_stream_cl_module
    new_ws_stream_srv_module
    basic_csv_recorder_module
)

foreach(COMPONENT IN LISTS REQUIRED_COMPONENTS)
    cpack_add_component(openDAQ_${COMPONENT}_Runtime
        REQUIRED
        DISPLAY_NAME
            ${COMPONENT}
        GROUP
            libopendaq
    )
    cpack_add_component(openDAQ_${COMPONENT}_Development
		REQUIRED
        DISPLAY_NAME
            ${COMPONENT}
        GROUP
            libopendaq-dev
        DEPENDS
            openDAQ_${COMPONENT}_Runtime
    )
endforeach()

foreach(COMPONENT IN LISTS OPTIONAL_COMPONENTS)
    cpack_add_component(openDAQ_${COMPONENT}_Runtime
        DISPLAY_NAME
            ${COMPONENT}
        GROUP
            libopendaq
    )
    cpack_add_component(openDAQ_${COMPONENT}_Development
        DISPLAY_NAME
            ${COMPONENT}
        GROUP
            libopendaq-dev
        DEPENDS
            openDAQ_${COMPONENT}_Runtime
    )
endforeach()

# temporary only
cpack_add_component(External
    REQUIRED
    DISPLAY_NAME
        External
    GROUP
        libopendaq-external
)

# SFML seems to be using the 'devel' component
cpack_add_component(devel
    DISPLAY_NAME
        SFML
    GROUP
        libopendaq-external
)

# We could add names and descriptions for all the components above
# set(CPACK_COMPONENT_openDAQ_coretypes_Runtime_DISPLAY_NAME "openDAQ core types")
