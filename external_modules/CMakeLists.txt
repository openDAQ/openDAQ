set(CMAKE_FOLDER "external_modules")
list(APPEND CMAKE_MESSAGE_CONTEXT external_modules)
set(CURR_MESSAGE_CONTEXT ${CMAKE_MESSAGE_CONTEXT})

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
        FetchContent_Declare(
            LtStreamingModulesLegacy
            GIT_REPOSITORY https://github.com/openDAQ/LTStreamingModulesLegacy.git
            GIT_TAG        ${DAQMODULES_LT_STREAMING_GIT_REF}
            GIT_PROGRESS   ON
            GIT_SHALLOW    OFF
            SOURCE_SUBDIR  "INVALID"
            GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
        )
        
        FetchContent_MakeAvailable(LtStreamingModulesLegacy)
        message(WARNING LtStreamingModulesLegacy_SOURCE_DIR)
		
        include("${LtStreamingModulesLegacy_SOURCE_DIR}/external/boost/Boost.cmake" OPTIONAL)
        set(LT_MODULES_SOURCE_DIR ${LtStreamingModulesLegacy_SOURCE_DIR} PARENT_SCOPE)
        set(LT_MODULES_BINARY_DIR ${LtStreamingModulesLegacy_BINARY_DIR} PARENT_SCOPE)
    else()
        FetchContent_Declare(
            LtStreamingModulesModern
            GIT_REPOSITORY https://github.com/openDAQ/LTStreamingModulesModern.git
            GIT_TAG        ${DAQMODULES_LT_STREAMING_GIT_REF}
            GIT_PROGRESS   ON
            GIT_SHALLOW    OFF
            SOURCE_SUBDIR  "INVALID"
            GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
        )
        
        FetchContent_MakeAvailable(LtStreamingModulesModern)
        
        include("${LtStreamingModulesModern_SOURCE_DIR}/external/boost/Boost.cmake" OPTIONAL)
        set(LT_MODULES_SOURCE_DIR ${LtStreamingModulesModern_SOURCE_DIR} PARENT_SCOPE)
        set(LT_MODULES_BINARY_DIR ${LtStreamingModulesModern_BINARY_DIR} PARENT_SCOPE)
    endif()
        
endif()

if (OPENDAQ_ENABLE_OPCUA AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE))
    set(DAQMODULES_OPCUA_ENABLE_TESTS ${OPENDAQ_ENABLE_TESTS} CACHE BOOL "")
    set(DAQMODULES_OPCUA_ENABLE_CLIENT ${DAQMODULES_OPENDAQ_CLIENT_MODULE} CACHE BOOL "")
    set(DAQMODULES_OPCUA_ENABLE_SERVER ${DAQMODULES_OPENDAQ_SERVER_MODULE} CACHE BOOL "")

    FetchContent_Declare(
        OpcUaModules
        GIT_REPOSITORY https://github.com/openDAQ/OpcUaModules.git
        GIT_TAG        ${DAQMODULES_OPCUA_GIT_REF}
        GIT_PROGRESS   ON
        GIT_SHALLOW    OFF
        SOURCE_SUBDIR  "INVALID"
        GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
    )
    
    FetchContent_MakeAvailable(OpcUaModules)
    
    include("${OpcUaModules_SOURCE_DIR}/external/boost/Boost.cmake" OPTIONAL)
    set(OPCUA_MODULES_SOURCE_DIR ${OpcUaModules_SOURCE_DIR} PARENT_SCOPE)
    set(OPCUA_MODULES_BINARY_DIR ${OpcUaModules_BINARY_DIR} PARENT_SCOPE)
endif()