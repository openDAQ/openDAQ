set(CMAKE_FOLDER "external_modules")
list(APPEND CMAKE_MESSAGE_CONTEXT external_modules)
set(CURR_MESSAGE_CONTEXT ${CMAKE_MESSAGE_CONTEXT})

include(openDAQModuleFetchUtils)

set(DAQMODULES_LT_STREAMING_GIT_REF "main" CACHE INTERNAL "openDAQ LT streaming modules project git reference")
set(DAQMODULES_OPCUA_GIT_REF "main" CACHE INTERNAL "openDAQ OpcUa modules project git reference")

if(NOT OPENDAQ_CI_RUNNER AND OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    message(DEPRECATION "Building LT streaming modules indirectly as subprojects of openDAQ is deprecated. Add modules project directly into your super-build by fetching https://github.com/openDAQ/LTStreamingModulesModern")
endif()

if(NOT OPENDAQ_CI_RUNNER AND OPENDAQ_ENABLE_OPCUA)
    message(DEPRECATION "Building OpcUa modules indirectly as subprojects of openDAQ is deprecated. Add modules project directly into your super-build by fetching https://github.com/openDAQ/OpcUaModules")
endif()

if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE))
    set(DAQMODULES_LT_STREAMING_ENABLE_TESTS ${OPENDAQ_ENABLE_TESTS} CACHE BOOL "")
    set(DAQMODULES_LT_STREAMING_ENABLE_CLIENT ${DAQMODULES_OPENDAQ_CLIENT_MODULE} CACHE BOOL "")
    set(DAQMODULES_LT_STREAMING_ENABLE_SERVER ${DAQMODULES_OPENDAQ_SERVER_MODULE} CACHE BOOL "")
    if (DAQMODULES_LT_LEGACY_MODULES)
        opendaq_fetch_module(
            NAME                LtStreamingModulesLegacy
            GIT_REPOSITORY      https://github.com/openDAQ/LTStreamingModulesLegacy.git
            GIT_REF             "${DAQMODULES_LT_STREAMING_GIT_REF}"
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
            GIT_REF             "${DAQMODULES_LT_STREAMING_GIT_REF}"
            GIT_SHALLOW         OFF
        )
        FetchContent_GetProperties(
            LtStreamingModulesModern
            SOURCE_DIR LT_MODULES_SOURCE_DIR
            BINARY_DIR LT_MODULES_BINARY_DIR
        )
    endif()
        
	include("${LT_MODULES_SOURCE_DIR}/external/boost/Boost.cmake" OPTIONAL)
	set(LT_MODULES_SOURCE_DIR ${LT_MODULES_SOURCE_DIR} PARENT_SCOPE)
	set(LT_MODULES_BINARY_DIR ${LT_MODULES_BINARY_DIR} PARENT_SCOPE)
endif()

if (OPENDAQ_ENABLE_OPCUA AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE))
    set(DAQMODULES_OPCUA_ENABLE_TESTS ${OPENDAQ_ENABLE_TESTS} CACHE BOOL "")
    set(DAQMODULES_OPCUA_ENABLE_CLIENT ${DAQMODULES_OPENDAQ_CLIENT_MODULE} CACHE BOOL "")
    set(DAQMODULES_OPCUA_ENABLE_SERVER ${DAQMODULES_OPENDAQ_SERVER_MODULE} CACHE BOOL "")
    opendaq_fetch_module(
        NAME                OpcUaModules
        GIT_REPOSITORY      https://github.com/openDAQ/OpcUaModules.git
        GIT_REF             "${DAQMODULES_OPCUA_GIT_REF}"
        GIT_SHALLOW         OFF
    )
    FetchContent_GetProperties(
        OpcUaModules
        SOURCE_DIR OPCUA_MODULES_SOURCE_DIR
        BINARY_DIR OPCUA_MODULES_BINARY_DIR
    )
	
	include("${OPCUA_MODULES_SOURCE_DIR}/external/boost/Boost.cmake" OPTIONAL)
	set(OPCUA_MODULES_SOURCE_DIR ${OPCUA_MODULES_SOURCE_DIR} PARENT_SCOPE)
	set(OPCUA_MODULES_BINARY_DIR ${OPCUA_MODULES_BINARY_DIR} PARENT_SCOPE)
endif()