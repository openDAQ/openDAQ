set(BASE_NAME signal)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Connection connection.h)
rtgen(SRC_Dimension dimension.h)
rtgen(SRC_DimensionBuilder dimension_builder.h)
rtgen(SRC_EventPacket event_packet.h)
rtgen(SRC_Packet packet.h)
rtgen(SRC_PacketDestructCallback packet_destruct_callback.h)
rtgen(SRC_Range range.h)
rtgen(SRC_DataDescriptor data_descriptor.h)
rtgen(SRC_DataDescriptorBuilder data_descriptor_builder.h)
rtgen(SRC_DataPacket data_packet.h)
rtgen(SRC_ReusableDataPacket reusable_data_packet.h)
rtgen(SRC_Signal signal.h)
rtgen(SRC_SignalEvents signal_events.h)
rtgen(SRC_SignalPrivate signal_private.h)
rtgen(SRC_SignalConfig signal_config.h)
rtgen(SRC_InputPort input_port.h)
rtgen(SRC_InputPortConfig input_port_config.h)
rtgen(SRC_InputPortPrivate input_port_private.h)
rtgen(SRC_Scaling scaling.h)
rtgen(SRC_ScalingBuilder scaling_builder.h)
rtgen(SRC_DataRule data_rule.h)
rtgen(SRC_DataRuleBuilder data_rule_builder.h)
rtgen(SRC_DimensionRule dimension_rule.h)
rtgen(SRC_DimensionRuleBuilder dimension_rule_builder.h)
rtgen(SRC_RulePrivate rule_private.h)
rtgen(SRC_InputPortNotifications input_port_notifications.h)
rtgen(SRC_Deleter deleter.h)
rtgen(SRC_Allocator allocator.h)

source_group("signal" FILES ${SDK_HEADERS_DIR}/signal.h
                            ${SDK_HEADERS_DIR}/signal_impl.h
                            ${SDK_HEADERS_DIR}/signal_container_impl.h
                            ${SDK_HEADERS_DIR}/signal_factory.h
                            ${SDK_HEADERS_DIR}/signal_events.h
                            ${SDK_HEADERS_DIR}/signal_private.h
                            ${SDK_HEADERS_DIR}/signal_config.h
                            signal_impl.cpp
)

source_group("data_descriptor" FILES ${SDK_HEADERS_DIR}/data_descriptor.h
                                     ${SDK_HEADERS_DIR}/data_descriptor_impl.h
                                     ${SDK_HEADERS_DIR}/data_descriptor_builder.h
                                     ${SDK_HEADERS_DIR}/data_descriptor_builder_impl.h
                                     ${SDK_HEADERS_DIR}/data_descriptor_factory.h
                                     data_descriptor_impl.cpp
                                     data_descriptor_builder_impl.cpp
)

source_group("dimension" FILES ${SDK_HEADERS_DIR}/dimension.h
                               ${SDK_HEADERS_DIR}/dimension_builder.h
                               ${SDK_HEADERS_DIR}/dimension_impl.h
                               ${SDK_HEADERS_DIR}/dimension_builder_impl.h
                               ${SDK_HEADERS_DIR}/dimension_factory.h
                               dimension_impl.cpp
                               dimension_builder_impl.cpp
)

source_group("range" FILES ${SDK_HEADERS_DIR}/range.h
                           ${SDK_HEADERS_DIR}/range_impl.h
                           ${SDK_HEADERS_DIR}/range_factory.h
                           ${SDK_HEADERS_DIR}/range_type.h
                           range_impl.cpp
)

source_group("connection" FILES ${SDK_HEADERS_DIR}/connection.h
                                ${SDK_HEADERS_DIR}/connection_impl.h
                                ${SDK_HEADERS_DIR}/connection_factory.h
                                connection_impl.cpp
)

source_group("packet" FILES ${SDK_HEADERS_DIR}/packet.h
                            ${SDK_HEADERS_DIR}/packet_impl.h
                            ${SDK_HEADERS_DIR}/packet_factory.h
                            ${SDK_HEADERS_DIR}/data_packet.h
                            ${SDK_HEADERS_DIR}/reusable_data_packet.h
                            ${SDK_HEADERS_DIR}/generic_data_packet_impl.h
                            ${SDK_HEADERS_DIR}/data_packet_impl.h
                            ${SDK_HEADERS_DIR}/event_packet.h
                            ${SDK_HEADERS_DIR}/event_packet_impl.h
                            ${SDK_HEADERS_DIR}/event_packet_ids.h
                            ${SDK_HEADERS_DIR}/event_packet_params.h
                            ${SDK_HEADERS_DIR}/deleter.h
                            ${SDK_HEADERS_DIR}/deleter_impl.h
                            ${SDK_HEADERS_DIR}/deleter_factory.h
                            ${SDK_HEADERS_DIR}/binary_data_packet.h
                            ${SDK_HEADERS_DIR}/binary_data_packet_impl.h
                            ${SDK_HEADERS_DIR}/binary_data_packet_factory.h
                            ${SDK_HEADERS_DIR}/packet_destruct_callback.h
                            ${SDK_HEADERS_DIR}/packet_destruct_callback_factory.h
                            ${SDK_HEADERS_DIR}/packet_destruct_callback_impl.h
                            data_packet_impl.cpp
                            generic_data_packet_impl.cpp
                            event_packet_impl.cpp
                            binary_data_packet_impl.cpp
)

source_group("input_port" FILES ${SDK_HEADERS_DIR}/input_port.h
                                ${SDK_HEADERS_DIR}/input_port_impl.h
                                ${SDK_HEADERS_DIR}/input_port_factory.h
                                ${SDK_HEADERS_DIR}/input_port_notifications.h
                                ${SDK_HEADERS_DIR}/input_port_private.h
                                ${SDK_HEADERS_DIR}/input_port_config.h
                                ${SDK_HEADERS_DIR}/input_port_config_ptr.custom.h
                                input_port_impl.cpp
)

source_group("dimension_rule" FILES ${SDK_HEADERS_DIR}/dimension_rule.h
                                    ${SDK_HEADERS_DIR}/dimension_rule_builder.h
                                    ${SDK_HEADERS_DIR}/dimension_rule_impl.h
                                    ${SDK_HEADERS_DIR}/dimension_rule_builder_impl.h
                                    ${SDK_HEADERS_DIR}/dimension_rule_factory.h
                                    dimension_rule_impl.cpp
                                    dimension_rule_builder_impl.cpp
)

source_group("data_rule" FILES ${SDK_HEADERS_DIR}/data_rule.h
                               ${SDK_HEADERS_DIR}/data_rule_builder.h
                               ${SDK_HEADERS_DIR}/data_rule_impl.h
                               ${SDK_HEADERS_DIR}/data_rule_builder_impl.h
                               ${SDK_HEADERS_DIR}/data_rule_factory.h
                               ${SDK_HEADERS_DIR}/rule_private.h
                               ${SDK_HEADERS_DIR}/data_rule_calc.h
                               ${SDK_HEADERS_DIR}/data_rule_calc_private.h
                               data_rule_impl.cpp
                               data_rule_builder_impl.cpp
)

source_group("scaling" FILES ${SDK_HEADERS_DIR}/scaling.h
                             ${SDK_HEADERS_DIR}/scaling_builder.h
                             ${SDK_HEADERS_DIR}/scaling_impl.h
                             ${SDK_HEADERS_DIR}/scaling_builder_impl.h
                             ${SDK_HEADERS_DIR}/scaling_factory.h
                             ${SDK_HEADERS_DIR}/scaling_calc.h
                             ${SDK_HEADERS_DIR}/scaling_calc_private.h
                             scaling_impl.cpp
                             scaling_builder_impl.cpp
)

set(SRC_GROUP_Allocator FILES ${SDK_HEADERS_DIR}/malloc_allocator_factory.h
                              ${SDK_HEADERS_DIR}/malloc_allocator_impl.h
                              ${SDK_HEADERS_DIR}/external_allocator_factory.h
                              ${SDK_HEADERS_DIR}/external_allocator_impl.h
                              malloc_allocator_impl.cpp
                              external_allocator_impl.cpp
)

set(SRC_Cpp connection_impl.cpp
            dimension_impl.cpp
            dimension_builder_impl.cpp
            range_impl.cpp
            signal_impl.cpp
            data_packet_impl.cpp
            generic_data_packet_impl.cpp
            event_packet_impl.cpp
            dimension_rule_impl.cpp
            dimension_rule_builder_impl.cpp
            data_rule_impl.cpp
            data_rule_builder_impl.cpp
            scaling_impl.cpp
            scaling_builder_impl.cpp
            input_port_impl.cpp
            binary_data_packet_impl.cpp
            data_descriptor_impl.cpp
            data_descriptor_builder_impl.cpp
            malloc_allocator_impl.cpp
            external_allocator_impl.cpp
)

set(SRC_PublicHeaders
    connection_factory.h
    data_rule_factory.h
    dimension_factory.h
    dimension_rule_factory.h
    data_descriptor_factory.h
    input_port_factory.h
    packet_factory.h
    range_factory.h
    scaling_factory.h
    signal_errors.h
    signal_exceptions.h
    signal_factory.h
    range_type.h
    sample_type.h
    sample_type_traits.h
    signal_utils.h
    event_packet_ids.h
    signal_container_impl.h
    input_port_config_ptr.custom.h
    binary_data_packet.h
    binary_data_packet_factory.h
    deleter.h
    deleter_impl.h
    deleter_factory.h
    allocator.h
    malloc_allocator_factory.h
    external_allocator_factory.h
    event_packet_params.h
    packet_destruct_callback_impl.h
    packet_destruct_callback_factory.h
    signal_impl.h
)

set(SRC_PrivateHeaders connection_impl.h
                       dimension_impl.h
                       dimension_builder_impl.h
                       range_impl.h
                       data_descriptor_impl.h
                       data_descriptor_builder_impl.h
                       input_port_impl.h
                       packet_impl.h
                       generic_data_packet_impl.h
                       data_packet_impl.h
                       event_packet_impl.h
                       scaling_impl.h
                       scaling_builder_impl.h
                       data_rule_impl.h
                       data_rule_builder_impl.h
                       dimension_rule_impl.h
                       dimension_rule_builder_impl.h
                       data_rule_calc.h
                       scaling_calc.h
                       binary_data_packet_impl.h
                       malloc_allocator_impl.h
                       data_rule_calc_private.h
                       scaling_calc_private.h
                       external_allocator_impl.h
)

set(SRC_ExtraPublicLibraries)

if (OPENDAQ_MIMALLOC_SUPPORT)
    list(APPEND SRC_ExtraPublicLibraries
        PRIVATE
            $<BUILD_INTERFACE:mimalloc-static>
    )
    list(APPEND SRC_Cpp mimalloc_allocator_impl.cpp)
    list(APPEND SRC_PublicHeaders mimalloc_allocator_factory.h)
    list(APPEND SRC_PrivateHeaders mimalloc_allocator_impl.h)
    list(APPEND SRC_GROUP_Allocator ${SDK_HEADERS_DIR}/mimalloc_allocator_factory.h
                                    ${SDK_HEADERS_DIR}/mimalloc_allocator_impl.h
                                    mimalloc_allocator_impl.cpp
    )
endif()

source_group("allocator" FILES ${SRC_GROUP_Allocator})

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Connection_Cpp}
                    ${SRC_Dimension_Cpp}
                    ${SRC_DimensionBuilder_Cpp}
                    ${SRC_EventPacket_Cpp}
                    ${SRC_Packet_Cpp}
                    ${SRC_Range_Cpp}
                    ${SRC_DataDescriptor_Cpp}
                    ${SRC_DataDescriptorBuilder_Cpp}
                    ${SRC_DataPacket_Cpp}
                    ${SRC_ReusableDataPacket_Cpp}
                    ${SRC_Signal_Cpp}
                    ${SRC_SignalEvents_Cpp}
                    ${SRC_SignalConfig_Cpp}
                    ${SRC_InputPort_Cpp}
                    ${SRC_DimensionRule_Cpp}
                    ${SRC_DimensionRuleBuilder_Cpp}
                    ${SRC_DataRule_Cpp}
                    ${SRC_DataRuleBuilder_Cpp}
                    ${SRC_Scaling_Cpp}
                    ${SRC_ScalingBuilder_Cpp}
                    ${SRC_InputPortNotifications_Cpp}
                    ${SRC_Allocator_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Connection_PublicHeaders}
                              ${SRC_Dimension_PublicHeaders}
                              ${SRC_DimensionBuilder_PublicHeaders}
                              ${SRC_EventPacket_PublicHeaders}
                              ${SRC_Packet_PublicHeaders}
                              ${SRC_PacketDestructCallback_PublicHeaders}
                              ${SRC_Range_PublicHeaders}
                              ${SRC_DataDescriptor_PublicHeaders}
                              ${SRC_DataDescriptorBuilder_PublicHeaders}
                              ${SRC_DataPacket_PublicHeaders}
                              ${SRC_ReusableDataPacket_PublicHeaders}
                              ${SRC_Signal_PublicHeaders}
                              ${SRC_SignalConfig_PublicHeaders}
                              ${SRC_InputPort_PublicHeaders}
                              ${SRC_InputPortConfig_PublicHeaders}
                              ${SRC_DimensionRule_PublicHeaders}
                              ${SRC_DimensionRuleBuilder_PublicHeaders}
                              ${SRC_DataRule_PublicHeaders}
                              ${SRC_DataRuleBuilder_PublicHeaders}
                              ${SRC_Scaling_PublicHeaders}
                              ${SRC_ScalingBuilder_PublicHeaders}
                              ${SRC_SignalEvents_PublicHeaders}
                              ${SRC_InputPortNotifications_PublicHeaders}
                              ${SRC_Deleter_PublicHeaders}
                              ${SRC_Allocator_PublicHeaders}
                              ${SRC_InputPortPrivate_PublicHeaders}
                              ${SRC_SignalPrivate_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_Connection_PrivateHeaders}
                               ${SRC_Dimension_PrivateHeaders}
                               ${SRC_DimensionBuilder_PrivateHeaders}
                               ${SRC_EventPacket_PrivateHeaders}
                               ${SRC_Packet_PrivateHeaders}
                               ${SRC_PacketDestructCallback_PrivateHeaders}
                               ${SRC_Range_PrivateHeaders}
                               ${SRC_DataDescriptor_PrivateHeaders}
                               ${SRC_DataDescriptorBuilder_PrivateHeaders}
                               ${SRC_ReusableDataPacket_PrivateHeaders}
                               ${SRC_Signal_PrivateHeaders}
                               ${SRC_SignalEvents_PrivateHeaders}
                               ${SRC_SignalPrivate_PrivateHeaders}
                               ${SRC_SignalConfig_PrivateHeaders}
                               ${SRC_InputPort_PrivateHeaders}
                               ${SRC_InputPortConfig_PrivateHeaders}
                               ${SRC_InputPortPrivate_PrivateHeaders}
                               ${SRC_DimensionRule_PrivateHeaders}
                               ${SRC_DimensionRuleBuilder_PrivateHeaders}
                               ${SRC_DataRule_PrivateHeaders}
                               ${SRC_DataRuleBuilder_PrivateHeaders}
                               ${SRC_Scaling_PrivateHeaders}
                               ${SRC_ScalingBuilder_PrivateHeaders}
                               ${SRC_RulePrivate_PublicHeaders}
                               ${SRC_RulePrivate_PrivateHeaders}
                               ${SRC_InputPortNotifications_PrivateHeaders}
                               ${SRC_Deleter_PrivateHeaders}
                               ${SRC_Allocator_PrivateHeaders}
)

if (WIN32)
    set(BCRYPT_LIB bcrypt.dll)
endif()

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${BuilderHeaderSource}
    signal.natvis
)

opendaq_target_link_libraries(${BASE_NAME}
    PRIVATE tsl::ordered_map
            ${BCRYPT_LIB}
            daq::logger
    PUBLIC daq::coreobjects
           daq::scheduler
           daq::utility
           daq::context
           daq::component
    ${SRC_ExtraPublicLibraries}
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>

        $<TARGET_PROPERTY:daq::module_manager,INTERFACE_INCLUDE_DIRECTORIES>
)
