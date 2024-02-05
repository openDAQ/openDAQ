set(BASE_NAME logger)

option(OPENDAQ_USE_SYNCHRONOUS_LOGGER "Output log messages immediately (blocks until finished)" OFF)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Logger logger.h)
rtgen(SRC_LoggerComponent logger_component.h)
rtgen(SRC_LoggerSink logger_sink.h)
rtgen(SRC_LoggerSinkBasePrivate logger_sink_base_private.h)
rtgen(SRC_LoggerSinkLastMessagePrivate logger_sink_last_message_private.h)
rtgen(SRC_LoggerThreadPool logger_thread_pool.h)

source_group("logger" FILES ${SDK_HEADERS_DIR}/logger.h
                            ${SDK_HEADERS_DIR}/logger_factory.h
                            ${SDK_HEADERS_DIR}/logger_impl.h
                            logger_impl.cpp
                            log.cpp
)

source_group("component" FILES ${SDK_HEADERS_DIR}/logger_component.h
                            ${SDK_HEADERS_DIR}/logger_component_factory.h
                            ${SDK_HEADERS_DIR}/logger_component_impl.h
                            logger_component_impl.cpp
)

source_group("sink" FILES ${SDK_HEADERS_DIR}/logger_sink.h
                            ${SDK_HEADERS_DIR}/logger_sink_factory.h
                            ${SDK_HEADERS_DIR}/logger_sink_impl.h
                            ${SDK_HEADERS_DIR}/logger_sink_base_private.h
                            ${SDK_HEADERS_DIR}/logger_sink_last_message_private.h
                            ${SDK_HEADERS_DIR}/logger_sink_last_message_impl.h
                            logger_sink_impl.cpp
)

source_group("threadpool" FILES ${SDK_HEADERS_DIR}/logger_thread_pool.h
                            ${SDK_HEADERS_DIR}/logger_thread_pool_factory.h
                            ${SDK_HEADERS_DIR}/logger_thread_pool_private.h
                            ${SDK_HEADERS_DIR}/logger_thread_pool_impl.h
                            logger_thread_pool_impl.cpp
)

set(SRC_Cpp log.cpp
            logger_impl.cpp
            logger_component_impl.cpp
            logger_sink_impl.cpp
            logger_thread_pool_impl.cpp
)

set(SRC_PublicHeaders log.h
                      log_level.h
                      logger_factory.h
                      logger_component_factory.h
                      logger_sink_factory.h
                      logger_thread_pool_factory.h
                      source_location.h
                      custom_log.h
)

set(SRC_PrivateHeaders logger_impl.h
                       logger_component_impl.h
                       logger_sink_impl.h
                       logger_sink_base_private.h
                       logger_sink_last_message_private.h
                       logger_sink_last_message_impl.h
                       logger_thread_pool_private.h
                       logger_thread_pool_impl.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Logger_Cpp}
                    ${SRC_LoggerComponent_Cpp}
                    ${SRC_LoggerSink_Cpp}
                    ${SRC_LoggerThreadPool_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Logger_PublicHeaders}
                              ${SRC_LoggerComponent_PublicHeaders}
                              ${SRC_LoggerSink_PublicHeaders}
                              ${SRC_LoggerSinkBasePrivate_PublicHeaders}
                              ${SRC_LoggerSinkLastMessagePrivate_PublicHeaders}
                              ${SRC_LoggerThreadPool_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_Logger_PrivateHeaders}
                               ${SRC_LoggerComponent_PrivateHeaders}
                               ${SRC_LoggerSink_PrivateHeaders}
                               ${SRC_LoggerSinkBasePrivate_PrivateHeaders}
                               ${SRC_LoggerSinkLastMessagePrivate_PrivateHeaders}
                               ${SRC_LoggerThreadPool_PrivateHeaders}
)

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${ConfigHeaderSource}
)

opendaq_target_link_libraries(${BASE_NAME}
    PUBLIC
        daq::coretypes
    PRIVATE
        $<BUILD_INTERFACE:spdlog::spdlog>
)

if (OPENDAQ_USE_SYNCHRONOUS_LOGGER)
    opendaq_target_compile_definitions(${BASE_NAME} PUBLIC OPENDAQ_LOGGER_SYNC)
endif()

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>
)
