#!/usr/bin/bash

export RTGEN="$PWD/../../shared/tools/RTGen/bin/rtgen.exe"
export CORE_DIR="$PWD/../../core"
export BINDINGS_DIR="$PWD"
#${RTGEN} --help

# 1: library name (CamelCase)
# 2: library/subdir
# 3: library
# 4: destination for python bindings
# 5: name
function run_rtgen {
    lib_name=$1
    lib_dir1=$2
    lib_dir2=$3
    lib_python=$4
    name=$5
    file_h=$name.h
    file_py=py_${name}.cpp
    source_dir="${CORE_DIR}/${lib_dir1}/include/${lib_dir2}"
    dest_dir="${BINDINGS_DIR}/${lib_python}"

    mkdir -p "${dest_dir}"

    command="${RTGEN} --language=python --library=${lib_name} --namespace=daq --source=${source_dir}/${file_h} --outputDir=${dest_dir}"
    echo "$command"
    $command
    #unix2dos "${dest_dir}/${file_py}"
}

#
# core types
#
core_type_files="
event_args
simple_type
#struct
struct_type
type
#type_manager
struct_builder
#enumeration
#enumeration_type
"
for file in "${core_type_files[@]}" 
do
    run_rtgen CoreTypes coretypes coretypes core_types/generated ${file}
done

#
# core objects
#
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated argument_info
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated callable_info
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated coercer
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated eval_value
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated ownable
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated property
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated property_builder
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated property_object
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated property_object_class
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated property_object_class_builder
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated property_object_protected
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated property_value_event_args
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated validator
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated unit
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated unit_builder
run_rtgen CoreObjects coreobjects coreobjects core_objects/generated component_type

#
# opendaq
#
run_rtgen opendaq opendaq/opendaq         opendaq opendaq/generated/opendaq       instance
run_rtgen opendaq opendaq/opendaq         opendaq opendaq/generated/opendaq       instance_builder
run_rtgen opendaq opendaq/opendaq         opendaq opendaq/generated/opendaq       config_provider
run_rtgen opendaq opendaq/component       opendaq opendaq/generated/component     component
run_rtgen opendaq opendaq/component       opendaq opendaq/generated/component     removable
run_rtgen opendaq opendaq/component       opendaq opendaq/generated/component     folder
run_rtgen opendaq opendaq/component       opendaq opendaq/generated/component     folder_config
run_rtgen opendaq opendaq/component       opendaq opendaq/generated/component     search_filter
run_rtgen opendaq opendaq/component       opendaq opendaq/generated/component     component_private
run_rtgen opendaq opendaq/component       opendaq opendaq/generated/component     component_status_container
run_rtgen opendaq opendaq/component       opendaq opendaq/generated/component     component_status_container_private
run_rtgen opendaq opendaq/context         opendaq opendaq/generated/context       context
run_rtgen opendaq opendaq/device          opendaq opendaq/generated/device        device
run_rtgen opendaq opendaq/device          opendaq opendaq/generated/device        device_domain
run_rtgen opendaq opendaq/device          opendaq opendaq/generated/device        device_info
run_rtgen opendaq opendaq/device          opendaq opendaq/generated/device        device_info_config
run_rtgen opendaq opendaq/device          opendaq opendaq/generated/device        server_capability
run_rtgen opendaq opendaq/device          opendaq opendaq/generated/device        device_type
run_rtgen opendaq opendaq/device          opendaq opendaq/generated/device        address_info
run_rtgen opendaq opendaq/device          opendaq opendaq/generated/device        address_info_builder
run_rtgen opendaq opendaq/functionblock   opendaq opendaq/generated/functionblock channel
run_rtgen opendaq opendaq/functionblock   opendaq opendaq/generated/functionblock function_block
run_rtgen opendaq opendaq/functionblock   opendaq opendaq/generated/functionblock function_block_type
run_rtgen opendaq opendaq/logger          opendaq opendaq/generated/logger        logger
run_rtgen opendaq opendaq/logger          opendaq opendaq/generated/logger        logger_component
run_rtgen opendaq opendaq/logger          opendaq opendaq/generated/logger        logger_sink
run_rtgen opendaq opendaq/logger          opendaq opendaq/generated/logger        logger_thread_pool
run_rtgen opendaq opendaq/modulemanager   opendaq opendaq/generated/modulemanager module
run_rtgen opendaq opendaq/modulemanager   opendaq opendaq/generated/modulemanager module_manager
run_rtgen opendaq opendaq/modulemanager   opendaq opendaq/generated/modulemanager discovery_server
run_rtgen opendaq opendaq/reader          opendaq opendaq/generated/reader        sample_reader
#run_rtgen opendaq opendaq/reader         opendaq opendaq/generated/reader        block_reader
run_rtgen opendaq opendaq/reader          opendaq opendaq/generated/reader        packet_reader
run_rtgen opendaq opendaq/reader          opendaq opendaq/generated/reader        reader
#run_rtgen opendaq opendaq/reader         opendaq opendaq/generated/reader        stream_reader
#run_rtgen opendaq opendaq/reader         opendaq opendaq/generated/reader        tail_reader
#run_rtgen opendaq opendaq/reader         opendaq opendaq/generated/reader        multi_reader
run_rtgen opendaq opendaq/reader          opendaq opendaq/generated/reader        block_reader_builder
run_rtgen opendaq opendaq/reader          opendaq opendaq/generated/reader        stream_reader_builder
run_rtgen opendaq opendaq/reader          opendaq opendaq/generated/reader        tail_reader_builder
run_rtgen opendaq opendaq/reader          opendaq opendaq/generated/reader        multi_reader_builder
run_rtgen opendaq opendaq/reader          opendaq opendaq/generated/reader        reader_status
run_rtgen opendaq opendaq/reader          opendaq opendaq/generated/reader        block_reader_status
run_rtgen opendaq opendaq/reader          opendaq opendaq/generated/reader        tail_reader_status
run_rtgen opendaq opendaq/reader          opendaq opendaq/generated/reader        multi_reader_status
run_rtgen opendaq opendaq/scheduler       opendaq opendaq/generated/scheduler     awaitable
run_rtgen opendaq opendaq/scheduler       opendaq opendaq/generated/scheduler     graph_visualization
run_rtgen opendaq opendaq/scheduler       opendaq opendaq/generated/scheduler     scheduler
run_rtgen opendaq opendaq/scheduler       opendaq opendaq/generated/scheduler     task
run_rtgen opendaq opendaq/scheduler       opendaq opendaq/generated/scheduler     task_graph
run_rtgen opendaq opendaq/server          opendaq opendaq/generated/server        server
run_rtgen opendaq opendaq/server          opendaq opendaq/generated/server        server_type
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        allocator
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        connection
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        data_descriptor
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        data_descriptor_builder
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        packet_destruct_callback
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        data_packet
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        data_rule
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        data_rule_builder
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        dimension
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        dimension_builder
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        dimension_rule
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        dimension_rule_builder
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        event_packet
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        input_port
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        input_port_config
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        input_port_notifications
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        packet
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        range
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        scaling
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        scaling_builder
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        signal
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        signal_config
run_rtgen opendaq opendaq/signal          opendaq opendaq/generated/signal        signal_events
run_rtgen opendaq opendaq/streaming       opendaq opendaq/generated/streaming     streaming
run_rtgen opendaq opendaq/streaming       opendaq opendaq/generated/streaming     mirrored_signal_config
run_rtgen opendaq opendaq/streaming       opendaq opendaq/generated/streaming     mirrored_signal_private
run_rtgen opendaq opendaq/streaming       opendaq opendaq/generated/streaming     subscription_event_args
run_rtgen opendaq opendaq/streaming       opendaq opendaq/generated/streaming     mirrored_device
run_rtgen opendaq opendaq/streaming       opendaq opendaq/generated/streaming     mirrored_device_config
run_rtgen opendaq opendaq/streaming       opendaq opendaq/generated/streaming     streaming_type
run_rtgen opendaq opendaq/component       opendaq opendaq/generated/component     tags
run_rtgen opendaq opendaq/component       opendaq opendaq/generated/component     tags_private
