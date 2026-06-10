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
##

opendaq_detect_triplet()

set(_PACKING_VERSION "${OPENDAQ_PACKAGE_VERSION}")
if(package_version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+)")
    string(APPEND _PACKING_VERSION ".${CMAKE_MATCH_1}")
endif()
if(NOT OPENDAQ_IS_RELEASE_VERSION AND OPENDAQ_WC_REVISION_HASH)
    string(SUBSTRING "${OPENDAQ_WC_REVISION_HASH}" 0 7 _PACKING_SHORT_SHA)
    string(APPEND _PACKING_VERSION "-${_PACKING_SHORT_SHA}")
endif()

opendaq_generate_package_name(VERSION "${_PACKING_VERSION}" NAME "opendaq")
set(CPACK_PACKAGE_FILE_NAME "${OPENDAQ_PACKAGE_NAME}")

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
