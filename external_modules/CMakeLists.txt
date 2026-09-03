set(CMAKE_FOLDER "external_modules")
list(APPEND CMAKE_MESSAGE_CONTEXT external_modules)
set(CURR_MESSAGE_CONTEXT ${CMAKE_MESSAGE_CONTEXT})


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
        opendaq_get_custom_fetch_content_params(LtStreamingModulesLegacy FC_PARAMS)
        FetchContent_Declare(
            LtStreamingModulesLegacy
            GIT_REPOSITORY https://github.com/openDAQ/LTStreamingModulesLegacy.git
            GIT_TAG        v4.0.0
            GIT_PROGRESS   ON
            GIT_SHALLOW    OFF
            SOURCE_SUBDIR  "INVALID"
            GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
            ${FC_PARAMS}
        )

        FetchContent_MakeAvailable(LtStreamingModulesLegacy)
        
        include("${ltstreamingmoduleslegacy_SOURCE_DIR}/external/boost/Boost.cmake" OPTIONAL)
        set(LT_MODULES_SOURCE_DIR ${ltstreamingmoduleslegacy_SOURCE_DIR} PARENT_SCOPE)
        set(LT_MODULES_BINARY_DIR ${ltstreamingmoduleslegacy_BINARY_DIR} PARENT_SCOPE)
		
    else()
        set(DAQMODULES_LT_STREAMING_ENABLE_TLS ${OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS} CACHE BOOL "" FORCE)

        opendaq_get_custom_fetch_content_params(LtStreamingModulesModern FC_PARAMS)
        
        FetchContent_Declare(
            LtStreamingModulesModern
            GIT_REPOSITORY https://github.com/openDAQ/LTStreamingModulesModern.git
            GIT_TAG        3b54c783d493f9759425d1dbe310080532cad9e1
            GIT_PROGRESS   ON
            GIT_SHALLOW    OFF
            SOURCE_SUBDIR  "INVALID"
            GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
            ${FC_PARAMS}
        )

        FetchContent_MakeAvailable(LtStreamingModulesModern)
        
        include("${ltstreamingmodulesmodern_SOURCE_DIR}/external/boost/Boost.cmake" OPTIONAL)
        set(LT_MODULES_SOURCE_DIR ${ltstreamingmodulesmodern_SOURCE_DIR} PARENT_SCOPE)
        set(LT_MODULES_BINARY_DIR ${ltstreamingmodulesmodern_BINARY_DIR} PARENT_SCOPE)
    endif()
        
endif()

if (OPENDAQ_ENABLE_OPCUA AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE))
    set(DAQMODULES_OPCUA_ENABLE_TESTS ${OPENDAQ_ENABLE_TESTS} CACHE BOOL "")
    set(DAQMODULES_OPCUA_ENABLE_CLIENT ${DAQMODULES_OPENDAQ_CLIENT_MODULE} CACHE BOOL "")
    set(DAQMODULES_OPCUA_ENABLE_SERVER ${DAQMODULES_OPENDAQ_SERVER_MODULE} CACHE BOOL "")

    opendaq_get_custom_fetch_content_params(OpcUaModules FC_PARAMS)
        
    FetchContent_Declare(
        OpcUaModules
        GIT_REPOSITORY https://github.com/openDAQ/OpcUaModules.git
        GIT_TAG        18c22dc97d774f404fac4e0bf87d9a035c8bb4e1
        GIT_PROGRESS   ON
        GIT_SHALLOW    OFF
        SOURCE_SUBDIR  "INVALID"
        GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
        ${FC_PARAMS}
    )
    
    FetchContent_MakeAvailable(OpcUaModules)
    
    include("${opcuamodules_SOURCE_DIR}/external/boost/Boost.cmake" OPTIONAL)
    set(OPCUA_MODULES_SOURCE_DIR ${opcuamodules_SOURCE_DIR} PARENT_SCOPE)
    set(OPCUA_MODULES_BINARY_DIR ${opcuamodules_BINARY_DIR} PARENT_SCOPE)
endif()
