set(BASE_NAME streaming)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_Streaming streaming.h)
    rtgen(SRC_StreamingType streaming_type.h)
    rtgen(SRC_MirroredSignal mirrored_signal_config.h)
    rtgen(SRC_SubscriptionEventArgs subscription_event_args.h)
    rtgen(SRC_MirroredSignalPrivate mirrored_signal_private.h)
    rtgen(SRC_MirroredDevice mirrored_device.h)
    rtgen(SRC_MirroredDeviceConfig mirrored_device_config.h)
    
    set(SRC_PublicHeaders_Component_Generated
        ${SRC_Streaming_PublicHeaders}
        ${SRC_MirroredSignal_PublicHeaders}
        ${SRC_SubscriptionEventArgs_PublicHeaders}
        ${SRC_MirroredSignalPrivate_PublicHeaders}
        ${SRC_MirroredDevice_PublicHeaders}
        ${SRC_MirroredDeviceConfig_PublicHeaders}
        ${SRC_StreamingType_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_Streaming_PrivateHeaders}
        ${SRC_MirroredSignal_PrivateHeaders}
        ${SRC_SubscriptionEventArgs_PrivateHeaders}
        ${SRC_MirroredSignalPrivate_PrivateHeaders}
        ${SRC_MirroredDevice_PrivateHeaders}
        ${SRC_MirroredDeviceConfig_PrivateHeaders}
        ${SRC_StreamingType_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_Streaming_Cpp}
        ${SRC_MirroredSignal_Cpp}
        ${SRC_SubscriptionEventArgs_Cpp}
        ${SRC_MirroredDevice_Cpp}
        ${SRC_MirroredDeviceConfig_Cpp}
        ${SRC_StreamingType_Cpp}
        PARENT_SCOPE
    )
endfunction()


function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "streaming/include/opendaq")
    set(SDK_SRC_DIR "streaming/src")
    
    source_group("streaming//mirrored_signal" FILES 
        ${SDK_HEADERS_DIR}/mirrored_signal_config.h
        ${SDK_HEADERS_DIR}/mirrored_signal_impl.h
        ${SDK_HEADERS_DIR}/mirrored_signal_private.h
    )
    
    source_group("streaming//streaming" FILES 
        ${SDK_HEADERS_DIR}/streaming.h
        ${SDK_HEADERS_DIR}/streaming_impl.h
        ${SDK_HEADERS_DIR}/streaming_private.h
    )
    
    source_group("streaming//subscription_event_args" FILES 
        ${SDK_HEADERS_DIR}/subscription_event_args.h
        ${SDK_HEADERS_DIR}/subscription_event_args_factory.h
        ${SDK_HEADERS_DIR}/subscription_event_args_impl.h
        ${SDK_SRC_DIR}/subscription_event_args_impl.cpp
    )
    
    source_group("streaming//mirrored_device" FILES 
        ${SDK_HEADERS_DIR}/mirrored_device.h
        ${SDK_HEADERS_DIR}/mirrored_device_config.h
        ${SDK_HEADERS_DIR}/mirrored_device_impl.h
    )
    
    source_group("streaming" FILES 
        ${SDK_SRC_DIR}/streaming.natvis
    )
    
    source_group("streaming_type" FILES 
        ${SDK_HEADERS_DIR}/streaming_type.h
        ${SDK_HEADERS_DIR}/streaming_type_factory.h
        ${SDK_HEADERS_DIR}/streaming_type_impl.h
        streaming_type_impl.cpp
    )
endfunction()

set(SRC_PublicHeaders_Component 
    streaming_impl.h
    mirrored_signal_impl.h
    streaming_private.h
    mirrored_signal_private.h
    subscription_event_args_factory.h
    mirrored_device.h
    mirrored_device_config.h
    mirrored_device_impl.h
    streaming_type.h
    streaming_type_factory.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component 
    subscription_event_args_impl.h
    streaming_type_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    subscription_event_args_impl.cpp
    streaming_type_impl.cpp
    streaming.natvis
    PARENT_SCOPE
)
