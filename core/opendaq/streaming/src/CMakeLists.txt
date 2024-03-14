set(BASE_NAME streaming)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Streaming streaming.h)
rtgen(SRC_MirroredSignal mirrored_signal_config.h)
rtgen(SRC_SubscriptionEventArgs subscription_event_args.h)

source_group("mirrored_signal" FILES ${SDK_HEADERS_DIR}/mirrored_signal_config.h
                                     ${SDK_HEADERS_DIR}/mirrored_signal_impl.h
                                     ${SDK_HEADERS_DIR}/mirrored_signal_private.h
)

source_group("streaming" FILES ${SDK_HEADERS_DIR}/streaming.h
                               ${SDK_HEADERS_DIR}/streaming_impl.h
                               ${SDK_HEADERS_DIR}/streaming_private.h
)

source_group("subscription_event_args" FILES ${SDK_HEADERS_DIR}/subscription_event_args.h
                                             ${SDK_HEADERS_DIR}/subscription_event_args_factory.h
                                             ${SDK_HEADERS_DIR}/subscription_event_args_impl.h
                                             subscription_event_args_impl.cpp
)

set(SRC_Cpp subscription_event_args_impl.cpp
)

set(SRC_PublicHeaders streaming_impl.h
                      mirrored_signal_impl.h
                      streaming_private.h
                      mirrored_signal_private.h
                      subscription_event_args_factory.h
)

set(SRC_PrivateHeaders subscription_event_args_impl.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Streaming_Cpp}
                    ${SRC_MirroredSignal_Cpp}
                    ${SRC_SubscriptionEventArgs_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Streaming_PublicHeaders}
                              ${SRC_MirroredSignal_PublicHeaders}
                              ${SRC_SubscriptionEventArgs_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_Streaming_PrivateHeaders}
                               ${SRC_MirroredSignal_PrivateHeaders}
                               ${SRC_SubscriptionEventArgs_PrivateHeaders}
)

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${ConfigHeaderSource}
    streaming.natvis
)

opendaq_target_link_libraries(${BASE_NAME}
    PUBLIC
        daq::coreobjects
        daq::logger
        daq::utility
        daq::signal
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>

        $<TARGET_PROPERTY:daq::signal,INTERFACE_INCLUDE_DIRECTORIES>
)
