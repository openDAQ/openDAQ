set(BASE_NAME logger)

option(OPENDAQ_USE_SYNCHRONOUS_LOGGER "Output log messages immediately (blocks until finished)" OFF)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_Logger logger.h)
    rtgen(SRC_LoggerComponent logger_component.h)
    rtgen(SRC_LoggerSink logger_sink.h)
    rtgen(SRC_LoggerSinkBasePrivate logger_sink_base_private.h)
    rtgen(SRC_LoggerSinkLastMessagePrivate logger_sink_last_message_private.h)
    rtgen(SRC_LoggerThreadPool logger_thread_pool.h)
    
    set(SRC_PublicHeaders_Component_Generated
        ${SRC_Logger_PublicHeaders}
        ${SRC_LoggerComponent_PublicHeaders}
        ${SRC_LoggerSink_PublicHeaders}
        ${SRC_LoggerSinkLastMessagePrivate_PublicHeaders}
        ${SRC_LoggerThreadPool_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_Logger_PrivateHeaders}
        ${SRC_LoggerComponent_PrivateHeaders}
        ${SRC_LoggerSink_PrivateHeaders}
        ${SRC_LoggerSinkBasePrivate_PrivateHeaders}
        ${SRC_LoggerSinkLastMessagePrivate_PrivateHeaders}
        ${SRC_LoggerThreadPool_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_Logger_Cpp}
        ${SRC_LoggerComponent_Cpp}
        ${SRC_LoggerSink_Cpp}
        ${SRC_LoggerThreadPool_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "logger/include/opendaq")
    set(SDK_SRC_DIR "logger/src")
    
    source_group("logger//logger" FILES 
        ${SDK_HEADERS_DIR}/logger.h
        ${SDK_HEADERS_DIR}/log.h
        ${SDK_HEADERS_DIR}/log_level.h
        ${SDK_HEADERS_DIR}/logger_factory.h
        ${SDK_HEADERS_DIR}/logger_impl.h
        ${SDK_HEADERS_DIR}/source_location.h
        ${SDK_HEADERS_DIR}/custom_log.h
        ${SDK_SRC_DIR}/logger_impl.cpp
        ${SDK_SRC_DIR}/log.cpp
    )
    
    source_group("logger//component" FILES 
        ${SDK_HEADERS_DIR}/logger_component.h
        ${SDK_HEADERS_DIR}/logger_component_factory.h
        ${SDK_HEADERS_DIR}/logger_component_impl.h
        ${SDK_SRC_DIR}/logger_component_impl.cpp
    )
    
    source_group("logger//sink" FILES 
        ${SDK_HEADERS_DIR}/logger_sink.h
        ${SDK_HEADERS_DIR}/logger_sink_factory.h
        ${SDK_HEADERS_DIR}/logger_sink_impl.h
        ${SDK_HEADERS_DIR}/logger_sink_base_private.h
        ${SDK_HEADERS_DIR}/logger_sink_last_message_private.h
        ${SDK_HEADERS_DIR}/logger_sink_last_message_impl.h
        ${SDK_SRC_DIR}/logger_sink_impl.cpp
    )
    
    source_group("logger//threadpool" FILES 
        ${SDK_HEADERS_DIR}/logger_thread_pool.h
        ${SDK_HEADERS_DIR}/logger_thread_pool_factory.h
        ${SDK_HEADERS_DIR}/logger_thread_pool_private.h
        ${SDK_HEADERS_DIR}/logger_thread_pool_impl.h
        ${SDK_SRC_DIR}/logger_thread_pool_impl.cpp
    )
endfunction()

set(SRC_PublicHeaders_Component 
    log.h
    log_level.h
    logger_errors.h
    logger_factory.h
    logger_component_factory.h
    logger_sink_factory.h
    logger_thread_pool_factory.h
    source_location.h
    custom_log.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component
    logger_impl.h
    logger_component_impl.h
    logger_sink_impl.h
    logger_sink_base_private.h
    logger_sink_last_message_private.h
    logger_sink_last_message_impl.h
    logger_thread_pool_private.h
    logger_thread_pool_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    log.cpp
    logger_impl.cpp
    logger_component_impl.cpp
    logger_sink_impl.cpp
    logger_thread_pool_impl.cpp
    PARENT_SCOPE
)

set(SRC_PrivateLinkLibraries_Component
    $<BUILD_INTERFACE:spdlog>
    PARENT_SCOPE
)

if (OPENDAQ_USE_SYNCHRONOUS_LOGGER)
    set(SRC_PublicCompileDefinitions_Component
        OPENDAQ_LOGGER_SYNC
        PARENT_SCOPE
    )
endif()