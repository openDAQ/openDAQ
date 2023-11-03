set(LIB_NAME py_opendaq)

set(SRC_Headers
    include/py_opendaq/py_opendaq.h
)
set(SRC_Cpp
    py_opendaq.cpp
    generated/opendaq/py_instance.cpp
    generated/component/py_component.cpp
    generated/component/py_removable.cpp
    generated/component/py_folder.cpp
    generated/component/py_folder_config.cpp
    generated/context/py_context.cpp
    generated/device/py_device.cpp
    generated/device/py_device_domain.cpp
    generated/device/py_device_info.cpp
    generated/device/py_device_info_config.cpp
    generated/device/py_device_type.cpp
    generated/functionblock/py_channel.cpp
    generated/functionblock/py_function_block.cpp
    generated/functionblock/py_function_block_type.cpp
    generated/logger/py_logger.cpp
    generated/logger/py_logger_component.cpp
    generated/logger/py_logger_sink.cpp
    generated/logger/py_logger_thread_pool.cpp
    generated/modulemanager/py_module.cpp
    generated/modulemanager/py_module_manager.cpp
    generated/reader/py_block_reader.cpp
    generated/reader/py_packet_reader.cpp
    generated/reader/py_reader.cpp
    generated/reader/py_sample_reader.cpp
    generated/reader/py_stream_reader.cpp
    generated/reader/py_tail_reader.cpp
    generated/scheduler/py_awaitable.cpp
    generated/scheduler/py_graph_visualization.cpp
    generated/scheduler/py_scheduler.cpp
    generated/scheduler/py_task.cpp
    generated/scheduler/py_task_graph.cpp
    generated/server/py_server.cpp
    generated/server/py_server_type.cpp
    generated/signal/py_allocator.cpp
    generated/signal/py_data_descriptor.cpp
    generated/signal/py_data_descriptor_builder.cpp
    generated/signal/py_connection.cpp
    generated/signal/py_data_packet.cpp
    generated/signal/py_data_rule.cpp
    generated/signal/py_data_rule_builder.cpp
    generated/signal/py_dimension.cpp
    generated/signal/py_dimension_builder.cpp
    generated/signal/py_dimension_rule.cpp
    generated/signal/py_dimension_rule_builder.cpp
    generated/signal/py_event_packet.cpp
    generated/signal/py_input_port.cpp
    generated/signal/py_input_port_config.cpp
    generated/signal/py_input_port_notifications.cpp
    generated/signal/py_packet.cpp
    generated/signal/py_range.cpp
    generated/signal/py_scaling.cpp
    generated/signal/py_scaling_builder.cpp
    generated/signal/py_signal.cpp
    generated/signal/py_signal_config.cpp
    generated/signal/py_signal_events.cpp
    generated/signal/py_packet_destruct_callback.cpp
    generated/streaming/py_streaming.cpp
    generated/streaming/py_streaming_info.cpp
    generated/streaming/py_streaming_info_config.cpp
    generated/streaming/py_signal_remote.cpp
    generated/utility/py_tags.cpp
    generated/utility/py_tags_config.cpp
)

add_library(${LIB_NAME} STATIC ${SRC_Headers} ${SRC_Cpp})
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME}
    PUBLIC
        daq::py_core_types
        daq::py_core_objects
        daq::opendaq
    PRIVATE
        pybind11::headers
        pybind11::module
        pybind11::lto
)

if(NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
    set_target_properties(${LIB_NAME} PROPERTIES CXX_VISIBILITY_PRESET "hidden")
endif()

if(MSVC)
    target_link_libraries(${LIB_NAME}
        PRIVATE
            pybind11::windows_extras
    )
endif()

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)
