set(LIB_NAME copendaq)
set_cmake_folder_context(TARGET_FOLDER_NAME copendaq)

set(SRC_CoreTypes_Headers
    include/copendaq.h
    include/ccommon.h

    include/ccoretypes/factories.h

    include/ccoretypes/base_object.h
    include/ccoretypes/binarydata.h
    include/ccoretypes/boolean.h
    include/ccoretypes/cloneable.h
    include/ccoretypes/comparable.h
    include/ccoretypes/complex_number.h
    include/ccoretypes/convertible.h
    include/ccoretypes/coretype.h
    # include/ccoretypes/dict_element_type.h # IntfID output paramater handling issue
    include/ccoretypes/dictobject.h
    include/ccoretypes/enumeration.h
    include/ccoretypes/enumeration_type.h
    include/ccoretypes/event.h
    include/ccoretypes/event_args.h
    include/ccoretypes/event_handler.h
    include/ccoretypes/float.h
    include/ccoretypes/freezable.h
    include/ccoretypes/function.h
    include/ccoretypes/integer.h
    include/ccoretypes/iterable.h
    include/ccoretypes/iterator.h
    # include/ccoretypes/list_element_type.h # IntfID output paramater handling issue
    include/ccoretypes/listobject.h
    include/ccoretypes/number.h
    include/ccoretypes/procedure.h
    include/ccoretypes/ratio.h
    include/ccoretypes/serializable.h
    include/ccoretypes/serialized_list.h
    include/ccoretypes/serialized_object.h
    include/ccoretypes/serializer.h
    include/ccoretypes/simple_type.h
    include/ccoretypes/stringobject.h
    include/ccoretypes/struct_builder.h
    include/ccoretypes/struct.h
    include/ccoretypes/struct_type.h
    include/ccoretypes/type.h
    # include/ccoretypes/type_manager_private.h # not implemented in daq
    include/ccoretypes/type_manager.h
    include/ccoretypes/updatable.h
    include/ccoretypes/version_info.h
)

set(SRC_Private_Headers
    include/copendaq_private.h
    include/private/utils.h
    include/private/c_event_handler_impl.h
)

set(SRC_CoreTypes_Cpp
    src/ccoretypes/factories.cpp

    src/ccoretypes/base_object.cpp
    src/ccoretypes/binarydata.cpp
    src/ccoretypes/boolean.cpp
    src/ccoretypes/cloneable.cpp
    src/ccoretypes/comparable.cpp
    src/ccoretypes/complex_number.cpp
    src/ccoretypes/convertible.cpp
    src/ccoretypes/coretype.cpp
    # src/ccoretypes/dict_element_type.cpp
    src/ccoretypes/dictobject.cpp
    src/ccoretypes/enumeration.cpp
    src/ccoretypes/enumeration_type.cpp
    src/ccoretypes/event.cpp
    src/ccoretypes/event_args.cpp
    src/ccoretypes/event_handler.cpp
    src/ccoretypes/float.cpp
    src/ccoretypes/freezable.cpp
    src/ccoretypes/function.cpp
    src/ccoretypes/integer.cpp
    src/ccoretypes/iterable.cpp
    src/ccoretypes/iterator.cpp
    # src/ccoretypes/list_element_type.cpp
    src/ccoretypes/listobject.cpp
    src/ccoretypes/number.cpp
    src/ccoretypes/procedure.cpp
    src/ccoretypes/ratio.cpp
    src/ccoretypes/serializable.cpp
    src/ccoretypes/serialized_list.cpp
    src/ccoretypes/serialized_object.cpp
    src/ccoretypes/serializer.cpp
    src/ccoretypes/simple_type.cpp
    src/ccoretypes/stringobject.cpp
    src/ccoretypes/struct_builder.cpp
    src/ccoretypes/struct.cpp
    src/ccoretypes/struct_type.cpp
    src/ccoretypes/type.cpp
    # src/ccoretypes/type_manager_private.cpp
    src/ccoretypes/type_manager.cpp
    src/ccoretypes/updatable.cpp
    src/ccoretypes/version_info.cpp
)

set(SRC_Private_Cpp
    src/private/utils.cpp
)

set(SRC_CoreObjects_Headers
    include/ccoreobjects/common.h

    include/ccoreobjects/argument_info.h
    include/ccoreobjects/authentication_provider.h
    include/ccoreobjects/callable_info.h
    include/ccoreobjects/coercer.h
    include/ccoreobjects/core_event_args.h
    include/ccoreobjects/end_update_event_args.h
    include/ccoreobjects/eval_value.h
    include/ccoreobjects/ownable.h
    include/ccoreobjects/permission_manager.h
    include/ccoreobjects/permission_mask_builder.h
    include/ccoreobjects/permissions.h
    include/ccoreobjects/permissions_builder.h
    include/ccoreobjects/property.h
    include/ccoreobjects/property_builder.h
    include/ccoreobjects/property_object.h
    include/ccoreobjects/property_object_class.h
    include/ccoreobjects/property_object_class_builder.h
    include/ccoreobjects/property_object_protected.h
    include/ccoreobjects/property_value_event_args.h
    include/ccoreobjects/unit.h
    include/ccoreobjects/unit_builder.h
    include/ccoreobjects/user.h
    include/ccoreobjects/validator.h
)

set(SRC_CoreObjects_Cpp
    src/ccoreobjects/argument_info.cpp
    src/ccoreobjects/authentication_provider.cpp
    src/ccoreobjects/callable_info.cpp
    src/ccoreobjects/coercer.cpp
    src/ccoreobjects/core_event_args.cpp
    src/ccoreobjects/end_update_event_args.cpp
    src/ccoreobjects/eval_value.cpp
    src/ccoreobjects/ownable.cpp
    src/ccoreobjects/permission_manager.cpp
    src/ccoreobjects/permission_mask_builder.cpp
    src/ccoreobjects/permissions.cpp
    src/ccoreobjects/permissions_builder.cpp
    src/ccoreobjects/property.cpp
    src/ccoreobjects/property_builder.cpp
    src/ccoreobjects/property_object.cpp
    src/ccoreobjects/property_object_class.cpp
    src/ccoreobjects/property_object_class_builder.cpp
    src/ccoreobjects/property_object_protected.cpp
    src/ccoreobjects/property_value_event_args.cpp
    src/ccoreobjects/unit.cpp
    src/ccoreobjects/unit_builder.cpp
    src/ccoreobjects/user.cpp
    src/ccoreobjects/validator.cpp
)

set(SRC_OpenDAQ_Headers
    include/copendaq/component/common.h
    include/copendaq/component/component.h
    # include/copendaq/component/component_deserialize_context.h # not implemented in daq
    # include/copendaq/component/component_holder.h
    include/copendaq/component/component_private.h
    include/copendaq/component/component_status_container.h
    include/copendaq/component/component_status_container_private.h
    include/copendaq/component/component_type.h
    include/copendaq/component/component_type_builder.h
    # include/copendaq/component/component_type_private.h
    # include/copendaq/component/component_update_context.h
    # include/copendaq/component/deserialize_component.h
    include/copendaq/component/folder.h
    include/copendaq/component/folder_config.h
    # include/copendaq/component/recursive_search.h
    include/copendaq/component/removable.h
    include/copendaq/component/search_filter.h
    include/copendaq/component/tags.h
    include/copendaq/component/tags_private.h
    include/copendaq/component/update_parameters.h

    include/copendaq/context/common.h
    include/copendaq/context/context.h

    include/copendaq/device/address_info.h
    include/copendaq/device/address_info_builder.h
    # include/copendaq/device/address_info_private.h
    include/copendaq/device/common.h
    include/copendaq/device/connection_status_container_private.h
    include/copendaq/device/device.h
    include/copendaq/device/device_domain.h
    include/copendaq/device/device_info.h
    include/copendaq/device/device_info_config.h
    # include/copendaq/device/device_info_internal.h
    include/copendaq/device/device_network_config.h
    include/copendaq/device/device_private.h
    include/copendaq/device/device_type.h
    include/copendaq/device/io_folder_config.h
    include/copendaq/device/log_file_info.h
    include/copendaq/device/log_file_info_builder.h
    include/copendaq/device/network_interface.h
    include/copendaq/device/server_capability.h
    include/copendaq/device/server_capability_config.h
    # include/copendaq/device/user_lock.h

    include/copendaq/functionblock/channel.h
    include/copendaq/functionblock/common.h
    include/copendaq/functionblock/function_block.h
    include/copendaq/functionblock/function_block_type.h
    include/copendaq/functionblock/function_block_wrapper.h

    include/copendaq/logger/common.h
    include/copendaq/logger/logger.h
    include/copendaq/logger/logger_component.h
    include/copendaq/logger/logger_sink.h
    # include/copendaq/logger/logger_sink_last_message_private.h

    include/copendaq/modulemanager/common.h
    # include/copendaq/modulemanager/context_internal.h
    include/copendaq/modulemanager/discovery_server.h
    include/copendaq/modulemanager/module.h
    include/copendaq/modulemanager/module_info.h
    include/copendaq/modulemanager/module_manager.h
    # include/copendaq/modulemanager/module_manager_utils.h

    include/copendaq/opendaq/common.h
    include/copendaq/opendaq/config_provider.h
    include/copendaq/opendaq/instance.h
    include/copendaq/opendaq/instance_builder.h

    include/copendaq/reader/block_reader.h
    include/copendaq/reader/block_reader_builder.h
    include/copendaq/reader/block_reader_status.h
    include/copendaq/reader/common.h
    include/copendaq/reader/multi_reader.h
    include/copendaq/reader/multi_reader_builder.h
    include/copendaq/reader/multi_reader_status.h
    include/copendaq/reader/packet_reader.h
    include/copendaq/reader/reader.h
    # include/copendaq/reader/reader_config.h
    include/copendaq/reader/reader_status.h
    include/copendaq/reader/sample_reader.h
    include/copendaq/reader/stream_reader.h
    include/copendaq/reader/stream_reader_builder.h
    include/copendaq/reader/tail_reader.h
    include/copendaq/reader/tail_reader_builder.h
    include/copendaq/reader/tail_reader_status.h

    include/copendaq/scheduler/awaitable.h
    include/copendaq/scheduler/common.h
    include/copendaq/scheduler/graph_visualization.h
    include/copendaq/scheduler/scheduler.h
    include/copendaq/scheduler/task.h
    include/copendaq/scheduler/task_graph.h
    include/copendaq/scheduler/work.h

    include/copendaq/server/common.h
    include/copendaq/server/server.h
    include/copendaq/server/server_type.h

    include/copendaq/signal/allocator.h
    include/copendaq/signal/common.h
    include/copendaq/signal/connection.h
    # include/copendaq/signal/connection_internal.h
    include/copendaq/signal/data_descriptor.h
    include/copendaq/signal/data_descriptor_builder.h
    include/copendaq/signal/data_packet.h
    include/copendaq/signal/data_rule.h
    include/copendaq/signal/data_rule_builder.h
    include/copendaq/signal/deleter.h
    include/copendaq/signal/dimension.h
    include/copendaq/signal/dimension_builder.h
    include/copendaq/signal/dimension_rule.h
    include/copendaq/signal/dimension_rule_builder.h
    include/copendaq/signal/event_packet.h
    include/copendaq/signal/input_port.h
    include/copendaq/signal/input_port_config.h
    include/copendaq/signal/input_port_notifications.h
    # include/copendaq/signal/input_port_private.h
    include/copendaq/signal/packet.h
    include/copendaq/signal/packet_destruct_callback.h
    include/copendaq/signal/range.h
    include/copendaq/signal/reference_domain_info.h
    include/copendaq/signal/reference_domain_info_builder.h
    # include/copendaq/signal/reusable_data_packet.h
    # include/copendaq/signal/rule_private.h
    include/copendaq/signal/scaling.h
    include/copendaq/signal/scaling_builder.h
    # include/copendaq/signal/scaling_calc_private.h
    include/copendaq/signal/signal.h
    include/copendaq/signal/signal_config.h
    include/copendaq/signal/signal_events.h
    # include/copendaq/signal/signal_private.h

    include/copendaq/streaming/common.h
    include/copendaq/streaming/mirrored_device.h
    include/copendaq/streaming/mirrored_device_config.h
    include/copendaq/streaming/mirrored_signal_config.h
    include/copendaq/streaming/mirrored_signal_private.h
    include/copendaq/streaming/streaming.h
    include/copendaq/streaming/streaming_type.h
    include/copendaq/streaming/subscription_event_args.h

    include/copendaq/synchronization/common.h
    include/copendaq/synchronization/sync_component.h
    include/copendaq/synchronization/sync_component_private.h

    include/copendaq/utility/common.h
)

set(SRC_OpenDAQ_Cpp
    src/copendaq/component/component.cpp
    # src/copendaq/component/component_deserialize_context.cpp # not implemented in daq
    # src/copendaq/component/component_holder.cpp
    src/copendaq/component/component_private.cpp
    src/copendaq/component/component_status_container.cpp
    src/copendaq/component/component_status_container_private.cpp
    src/copendaq/component/component_type.cpp
    src/copendaq/component/component_type_builder.cpp
    # src/copendaq/component/component_type_private.cpp
    # src/copendaq/component/component_update_context.cpp
    # src/copendaq/component/deserialize_component.cpp
    src/copendaq/component/folder.cpp
    src/copendaq/component/folder_config.cpp
    # src/copendaq/component/recursive_search.cpp
    src/copendaq/component/removable.cpp
    src/copendaq/component/search_filter.cpp
    src/copendaq/component/tags.cpp
    src/copendaq/component/tags_private.cpp
    src/copendaq/component/update_parameters.cpp

    src/copendaq/context/context.cpp

    src/copendaq/device/address_info.cpp
    src/copendaq/device/address_info_builder.cpp
    # src/copendaq/device/address_info_private.cpp
    src/copendaq/device/connection_status_container_private.cpp
    src/copendaq/device/device.cpp
    src/copendaq/device/device_domain.cpp
    src/copendaq/device/device_info.cpp
    src/copendaq/device/device_info_config.cpp
    # src/copendaq/device/device_info_internal.cpp
    src/copendaq/device/device_network_config.cpp
    src/copendaq/device/device_private.cpp
    src/copendaq/device/device_type.cpp
    src/copendaq/device/io_folder_config.cpp
    src/copendaq/device/log_file_info.cpp
    src/copendaq/device/log_file_info_builder.cpp
    src/copendaq/device/network_interface.cpp
    src/copendaq/device/server_capability.cpp
    src/copendaq/device/server_capability_config.cpp
    # src/copendaq/device/user_lock.cpp

    src/copendaq/functionblock/channel.cpp
    src/copendaq/functionblock/function_block.cpp
    src/copendaq/functionblock/function_block_type.cpp
    src/copendaq/functionblock/function_block_wrapper.cpp

    src/copendaq/logger/logger.cpp
    src/copendaq/logger/logger_component.cpp
    src/copendaq/logger/logger_sink.cpp
    # src/copendaq/logger/logger_sink_last_message_private.cpp

    # src/copendaq/modulemanager/context_internal.cpp
    src/copendaq/modulemanager/discovery_server.cpp
    src/copendaq/modulemanager/module.cpp
    src/copendaq/modulemanager/module_info.cpp
    src/copendaq/modulemanager/module_manager.cpp
    # src/copendaq/modulemanager/module_manager_utils.cpp

    src/copendaq/opendaq/config_provider.cpp
    src/copendaq/opendaq/instance.cpp
    src/copendaq/opendaq/instance_builder.cpp

    src/copendaq/reader/block_reader.cpp
    src/copendaq/reader/block_reader_builder.cpp
    src/copendaq/reader/block_reader_status.cpp
    src/copendaq/reader/multi_reader.cpp
    src/copendaq/reader/multi_reader_builder.cpp
    src/copendaq/reader/multi_reader_status.cpp
    src/copendaq/reader/packet_reader.cpp
    src/copendaq/reader/reader.cpp
    # src/copendaq/reader/reader_config.cpp
    src/copendaq/reader/reader_status.cpp
    src/copendaq/reader/sample_reader.cpp
    src/copendaq/reader/stream_reader.cpp
    src/copendaq/reader/stream_reader_builder.cpp
    src/copendaq/reader/tail_reader.cpp
    src/copendaq/reader/tail_reader_builder.cpp
    src/copendaq/reader/tail_reader_status.cpp

    src/copendaq/scheduler/awaitable.cpp
    src/copendaq/scheduler/graph_visualization.cpp
    src/copendaq/scheduler/scheduler.cpp
    src/copendaq/scheduler/task.cpp
    src/copendaq/scheduler/task_graph.cpp
    src/copendaq/scheduler/work.cpp

    src/copendaq/server/server.cpp
    src/copendaq/server/server_type.cpp

    src/copendaq/signal/allocator.cpp
    src/copendaq/signal/connection.cpp
    # src/copendaq/signal/connection_internal.cpp
    src/copendaq/signal/data_descriptor.cpp
    src/copendaq/signal/data_descriptor_builder.cpp
    src/copendaq/signal/data_packet.cpp
    src/copendaq/signal/data_rule.cpp
    src/copendaq/signal/data_rule_builder.cpp
    src/copendaq/signal/deleter.cpp
    src/copendaq/signal/dimension.cpp
    src/copendaq/signal/dimension_builder.cpp
    src/copendaq/signal/dimension_rule.cpp
    src/copendaq/signal/dimension_rule_builder.cpp
    src/copendaq/signal/event_packet.cpp
    src/copendaq/signal/input_port.cpp
    src/copendaq/signal/input_port_config.cpp
    src/copendaq/signal/input_port_notifications.cpp
    # src/copendaq/signal/input_port_private.cpp
    src/copendaq/signal/packet.cpp
    src/copendaq/signal/packet_destruct_callback.cpp
    src/copendaq/signal/range.cpp
    src/copendaq/signal/reference_domain_info.cpp
    src/copendaq/signal/reference_domain_info_builder.cpp
    # src/copendaq/signal/reusable_data_packet.cpp
    # src/copendaq/signal/rule_private.cpp
    src/copendaq/signal/scaling.cpp
    src/copendaq/signal/scaling_builder.cpp
    # src/copendaq/signal/scaling_calc_private.cpp
    src/copendaq/signal/signal.cpp
    src/copendaq/signal/signal_config.cpp
    src/copendaq/signal/signal_events.cpp
    # src/copendaq/signal/signal_private.cpp

    src/copendaq/streaming/mirrored_device.cpp
    src/copendaq/streaming/mirrored_device_config.cpp
    src/copendaq/streaming/mirrored_signal_config.cpp
    src/copendaq/streaming/mirrored_signal_private.cpp
    src/copendaq/streaming/streaming.cpp
    src/copendaq/streaming/streaming_type.cpp
    src/copendaq/streaming/subscription_event_args.cpp

    src/copendaq/synchronization/sync_component.cpp
    src/copendaq/synchronization/sync_component_private.cpp
)

add_library(${LIB_NAME} SHARED ${SRC_CoreTypes_Headers} ${SRC_CoreTypes_Cpp} ${SRC_Private_Headers} ${SRC_Private_Cpp} ${SRC_CoreObjects_Headers} ${SRC_CoreObjects_Cpp} ${SRC_OpenDAQ_Headers} ${SRC_OpenDAQ_Cpp})
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_sources(${LIB_NAME}
    INTERFACE
    FILE_SET copendaq_interface_headers TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES ${SRC_CoreTypes_Headers} ${SRC_CoreObjects_Headers} ${SRC_OpenDAQ_Headers}
)

target_link_libraries(${LIB_NAME}
    PRIVATE
    daq::coretypes
    daq::coreobjects
    daq::opendaq
)

target_include_directories(${LIB_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)

target_compile_definitions(${LIB_NAME} PRIVATE BUILDING_SHARED_LIBRARY)

install(TARGETS ${LIB_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT ${SDK_NAME}_${BASE_NAME}_Runtime
    NAMELINK_COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
    FILE_SET copendaq_interface_headers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${BASE_NAME}/copendaq
    COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
)

if(OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
