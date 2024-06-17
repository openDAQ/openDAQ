set(BASE_NAME streaming)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Streaming streaming.h)
rtgen(SRC_StreamingType streaming_type.h)
rtgen(SRC_MirroredSignal mirrored_signal_config.h)
rtgen(SRC_SubscriptionEventArgs subscription_event_args.h)
rtgen(SRC_MirroredSignalPrivate mirrored_signal_private.h)
rtgen(SRC_MirroredDevice mirrored_device.h)
rtgen(SRC_MirroredDeviceConfig mirrored_device_config.h)

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

source_group("mirrored_device" FILES ${SDK_HEADERS_DIR}/mirrored_device.h
                                     ${SDK_HEADERS_DIR}/mirrored_device_config.h
                                     ${SDK_HEADERS_DIR}/mirrored_device_impl.h
)

source_group("streaming_type" FILES ${SDK_HEADERS_DIR}/streaming_type.h
                                    ${SDK_HEADERS_DIR}/streaming_type_factory.h
                                    ${SDK_HEADERS_DIR}/streaming_type_impl.h
                                    streaming_type_impl.cpp
)

set(SRC_Cpp subscription_event_args_impl.cpp
            streaming_type_impl.cpp
)

set(SRC_PublicHeaders streaming_impl.h
                      mirrored_signal_impl.h
                      streaming_private.h
                      mirrored_signal_private.h
                      subscription_event_args_factory.h
                      mirrored_device.h
                      mirrored_device_config.h
                      mirrored_device_impl.h
                      streaming_type.h
                      streaming_type_factory.h
)

set(SRC_PrivateHeaders subscription_event_args_impl.h
                       streaming_type_impl.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Streaming_Cpp}
                    ${SRC_MirroredSignal_Cpp}
                    ${SRC_SubscriptionEventArgs_Cpp}
                    ${SRC_MirroredDevice_Cpp}
                    ${SRC_MirroredDeviceConfig_Cpp}
                    ${SRC_StreamingType_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Streaming_PublicHeaders}
                              ${SRC_MirroredSignal_PublicHeaders}
                              ${SRC_SubscriptionEventArgs_PublicHeaders}
                              ${SRC_MirroredSignalPrivate_PublicHeaders}
                              ${SRC_MirroredDevice_PublicHeaders}
                              ${SRC_MirroredDeviceConfig_PublicHeaders}
                              ${SRC_StreamingType_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_Streaming_PrivateHeaders}
                               ${SRC_MirroredSignal_PrivateHeaders}
                               ${SRC_SubscriptionEventArgs_PrivateHeaders}
                               ${SRC_MirroredSignalPrivate_PrivateHeaders}
                               ${SRC_MirroredDevice_PrivateHeaders}
                               ${SRC_MirroredDeviceConfig_PrivateHeaders}
                               ${SRC_StreamingType_PrivateHeaders}
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
        daq::device
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>

        $<TARGET_PROPERTY:daq::signal,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:daq::device,INTERFACE_INCLUDE_DIRECTORIES>
)
