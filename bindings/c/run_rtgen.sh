#!/usr/bin/bash

export RTGEN="$PWD/../../shared/tools/RTGen/bin/rtgen.exe"
export CORE_DIR="$PWD/../../core"
export BINDINGS_DIR="$PWD"
#${RTGEN} --help

function run_rtgen {
    lib_name=$1
    lib_dir1=$2
    lib_dir2=$3
    name=$4
    out_name=$5

    file_h=$name.h
    file_c_h=${out_name}.h
    file_c_cpp=${out_name}.cpp
    source_dir="${CORE_DIR}/${lib_dir1}/include/${lib_dir2}"
    dest_dir_headers="${BINDINGS_DIR}/include/${lib_name}"
    dest_dir_sources="${BINDINGS_DIR}/src/${lib_name}"

    command_h="${RTGEN} --language=c --library=${lib_name} --namespace=daq --source=${source_dir}/${file_h} --outputDir=${dest_dir_headers} --output=${out_name} --extension=.h"
    command_cpp="${RTGEN} --language=c --library=${lib_name} --namespace=daq --source=${source_dir}/${file_h} --outputDir=${dest_dir_sources} --output=${out_name} --extension=.cpp"
    
    echo "------------------------------------------"
    echo "$command_h"
    $command_h
    echo "$command_cpp"
    $command_cpp
}

run_rtgen ccoretypes coretypes coretypes binarydata binarydata
run_rtgen ccoretypes coretypes coretypes boolean boolean
run_rtgen ccoretypes coretypes coretypes cloneable cloneable
run_rtgen ccoretypes coretypes coretypes comparable comparable
run_rtgen ccoretypes coretypes coretypes complex_number complex_number
run_rtgen ccoretypes coretypes coretypes convertible convertible
# run_rtgen ccoretypes coretypes coretypes coretype coretype # once, name collision
# run_rtgen ccoretypes coretypes coretypes cycle_detector cycle_detector # funcs could be added manually
# run_rtgen ccoretypes coretypes coretypes deserializer deserializer #need to be generated once
run_rtgen ccoretypes coretypes coretypes dict_element_type dict_element_type
run_rtgen ccoretypes coretypes coretypes enumeration enumeration
run_rtgen ccoretypes coretypes coretypes enumeration_type enumeration_type
# run_rtgen ccoretypes coretypes coretypes errorinfo errorinfo #need to be generated once and funcs added manually
# run_rtgen ccoretypes coretypes coretypes errors errors #contains error codes
run_rtgen ccoretypes coretypes coretypes event event
run_rtgen ccoretypes coretypes coretypes event_args event_args
run_rtgen ccoretypes coretypes coretypes event_handler event_handler
# run_rtgen ccoretypes coretypes coretypes float float
run_rtgen ccoretypes coretypes coretypes freezable freezable
# run_rtgen ccoretypes coretypes coretypes function function
# run_rtgen ccoretypes coretypes coretypes inspectable inspectable
run_rtgen ccoretypes coretypes coretypes integer integer
run_rtgen ccoretypes coretypes coretypes iterable iterable
run_rtgen ccoretypes coretypes coretypes iterator iterator
# run_rtgen ccoretypes coretypes coretypes json_deserializer json_deserializer
run_rtgen ccoretypes coretypes coretypes list_element_type list_element_type
# run_rtgen ccoretypes coretypes coretypes mem mem #could be added manually
run_rtgen ccoretypes coretypes coretypes number number #disable because of float
# run_rtgen ccoretypes coretypes coretypes procedure procedure
run_rtgen ccoretypes coretypes coretypes ratio ratio
run_rtgen ccoretypes coretypes coretypes serializable serializable
run_rtgen ccoretypes coretypes coretypes serialized_list serialized_list
run_rtgen ccoretypes coretypes coretypes serialized_object serialized_object
# run_rtgen ccoretypes coretypes coretypes serializer serializer #generated once
run_rtgen ccoretypes coretypes coretypes simple_type simple_type
run_rtgen ccoretypes coretypes coretypes stringobject stringobject
run_rtgen ccoretypes coretypes coretypes struct struct
run_rtgen ccoretypes coretypes coretypes struct_builder struct_builder
run_rtgen ccoretypes coretypes coretypes struct_type struct_type
run_rtgen ccoretypes coretypes coretypes type type
run_rtgen ccoretypes coretypes coretypes type_manager type_manager
run_rtgen ccoretypes coretypes coretypes type_manager_private type_manager_private #once probably
run_rtgen ccoretypes coretypes coretypes updatable updatable
# run_rtgen ccoretypes coretypes coretypes version version #functions should be added manually
run_rtgen ccoretypes coretypes coretypes version_info version_info

run_rtgen ccoretypes corecontainers coretypes dictobject dictobject
run_rtgen ccoretypes corecontainers coretypes listobject listobject

# core objects

run_rtgen ccoreobjects coreobjects coreobjects argument_info argument_info
run_rtgen ccoreobjects coreobjects coreobjects authentication_provider authentication_provider
run_rtgen ccoreobjects coreobjects coreobjects callable_info callable_info
run_rtgen ccoreobjects coreobjects coreobjects coercer coercer
run_rtgen ccoreobjects coreobjects coreobjects core_event_args core_event_args
run_rtgen ccoreobjects coreobjects coreobjects end_update_event_args end_update_event_args
# run_rtgen ccoreobjects coreobjects coreobjects errors errors # contains error codes
run_rtgen ccoreobjects coreobjects coreobjects eval_value eval_value
run_rtgen ccoreobjects coreobjects coreobjects ownable ownable
run_rtgen ccoreobjects coreobjects coreobjects permission_manager permission_manager
run_rtgen ccoreobjects coreobjects coreobjects permission_mask_builder permission_mask_builder
run_rtgen ccoreobjects coreobjects coreobjects permissions permissions
run_rtgen ccoreobjects coreobjects coreobjects permissions_builder permissions_builder
run_rtgen ccoreobjects coreobjects coreobjects property property
run_rtgen ccoreobjects coreobjects coreobjects property_builder property_builder
run_rtgen ccoreobjects coreobjects coreobjects property_object property_object
run_rtgen ccoreobjects coreobjects coreobjects property_object_class property_object_class
run_rtgen ccoreobjects coreobjects coreobjects property_object_class_builder property_object_class_builder
run_rtgen ccoreobjects coreobjects coreobjects property_object_protected property_object_protected
# enum collisions should be manually converted
# run_rtgen ccoreobjects coreobjects coreobjects property_value_event_args property_value_event_args  
run_rtgen ccoreobjects coreobjects coreobjects unit unit
run_rtgen ccoreobjects coreobjects coreobjects unit_builder unit_builder
run_rtgen ccoreobjects coreobjects coreobjects user user
# run_rtgen ccoreobjects coreobjects coreobjects util util #function could be added manually
run_rtgen ccoreobjects coreobjects coreobjects validator validator
# run_rtgen ccoreobjects coreobjects coreobjects version version #function should be added manually

# opendaq component

run_rtgen copendaq/component opendaq/component opendaq component component
run_rtgen copendaq/component opendaq/component opendaq component_deserialize_context component_deserialize_context
run_rtgen copendaq/component opendaq/component opendaq component_errors component_errors
run_rtgen copendaq/component opendaq/component opendaq component_exceptions component_exceptions
run_rtgen copendaq/component opendaq/component opendaq component_holder component_holder
run_rtgen copendaq/component opendaq/component opendaq component_keys component_keys
run_rtgen copendaq/component opendaq/component opendaq component_private component_private
run_rtgen copendaq/component opendaq/component opendaq component_ptr.custom component_ptr.custom
run_rtgen copendaq/component opendaq/component opendaq component_status_container component_status_container
run_rtgen copendaq/component opendaq/component opendaq component_status_container_private component_status_container_private
run_rtgen copendaq/component opendaq/component opendaq component_type component_type
run_rtgen copendaq/component opendaq/component opendaq component_type_builder component_type_builder
run_rtgen copendaq/component opendaq/component opendaq component_type_private component_type_private
run_rtgen copendaq/component opendaq/component opendaq component_update_context component_update_context
run_rtgen copendaq/component opendaq/component opendaq deserialize_component deserialize_component
run_rtgen copendaq/component opendaq/component opendaq folder folder
run_rtgen copendaq/component opendaq/component opendaq folder_config folder_config
run_rtgen copendaq/component opendaq/component opendaq recursive_search recursive_search
run_rtgen copendaq/component opendaq/component opendaq removable removable
run_rtgen copendaq/component opendaq/component opendaq search_filter search_filter
run_rtgen copendaq/component opendaq/component opendaq tags tags
run_rtgen copendaq/component opendaq/component opendaq tags_private tags_private
run_rtgen copendaq/component opendaq/component opendaq update_parameters update_parameters

# opendaq context

run_rtgen copendaq/context opendaq/context opendaq context context
run_rtgen copendaq/context opendaq/context opendaq context_ptr.fwd_declare context_ptr.fwd_declare

# opendaq device

run_rtgen copendaq/device opendaq/device opendaq address_info address_info
run_rtgen copendaq/device opendaq/device opendaq address_info_builder address_info_builder
run_rtgen copendaq/device opendaq/device opendaq address_info_private address_info_private
run_rtgen copendaq/device opendaq/device opendaq connection_status_container_private connection_status_container_private
run_rtgen copendaq/device opendaq/device opendaq core_opendaq_event_args core_opendaq_event_args
run_rtgen copendaq/device opendaq/device opendaq device device
run_rtgen copendaq/device opendaq/device opendaq device_domain device_domain
run_rtgen copendaq/device opendaq/device opendaq device_errors device_errors
run_rtgen copendaq/device opendaq/device opendaq device_exceptions device_exceptions
run_rtgen copendaq/device opendaq/device opendaq device_info device_info
run_rtgen copendaq/device opendaq/device opendaq device_info_config device_info_config
run_rtgen copendaq/device opendaq/device opendaq device_info_internal device_info_internal
run_rtgen copendaq/device opendaq/device opendaq device_network_config device_network_config
run_rtgen copendaq/device opendaq/device opendaq device_private device_private
run_rtgen copendaq/device opendaq/device opendaq device_type device_type
run_rtgen copendaq/device opendaq/device opendaq io_folder_config io_folder_config
run_rtgen copendaq/device opendaq/device opendaq log_file_info log_file_info
run_rtgen copendaq/device opendaq/device opendaq log_file_info_builder log_file_info_builder
run_rtgen copendaq/device opendaq/device opendaq network_interface network_interface
run_rtgen copendaq/device opendaq/device opendaq server_capability server_capability
run_rtgen copendaq/device opendaq/device opendaq server_capability_config server_capability_config
run_rtgen copendaq/device opendaq/device opendaq user_lock user_lock

# opendaq functionblock

run_rtgen copendaq/functionblock opendaq/functionblock opendaq channel channel
run_rtgen copendaq/functionblock opendaq/functionblock opendaq channel_impl channel_impl
run_rtgen copendaq/functionblock opendaq/functionblock opendaq function_block function_block
run_rtgen copendaq/functionblock opendaq/functionblock opendaq function_block_errors function_block_errors
run_rtgen copendaq/functionblock opendaq/functionblock opendaq function_block_type function_block_type
run_rtgen copendaq/functionblock opendaq/functionblock opendaq function_block_wrapper function_block_wrapper

# opendaq logger

run_rtgen copendaq/logger opendaq/logger opendaq custom_log custom_log
run_rtgen copendaq/logger opendaq/logger opendaq log log
run_rtgen copendaq/logger opendaq/logger opendaq log_level log_level
run_rtgen copendaq/logger opendaq/logger opendaq logger logger
run_rtgen copendaq/logger opendaq/logger opendaq logger_component logger_component
run_rtgen copendaq/logger opendaq/logger opendaq logger_errors logger_errors
run_rtgen copendaq/logger opendaq/logger opendaq logger_sink logger_sink
run_rtgen copendaq/logger opendaq/logger opendaq logger_sink_base_private logger_sink_base_private
run_rtgen copendaq/logger opendaq/logger opendaq logger_sink_last_message_private logger_sink_last_message_private
run_rtgen copendaq/logger opendaq/logger opendaq logger_thread_pool logger_thread_pool
run_rtgen copendaq/logger opendaq/logger opendaq logger_thread_pool_private logger_thread_pool_private
run_rtgen copendaq/logger opendaq/logger opendaq source_location source_location

# opendaq modulemanager

run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq context_internal context_internal
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq discovery_server discovery_server
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq format format
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq icmp_header icmp_header
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq icmp_ping icmp_ping
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq ipv4_header ipv4_header
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq module module
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq module_check_dependencies module_check_dependencies
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq module_exports module_exports
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq module_info module_info
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq module_library module_library
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq module_manager module_manager
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq module_manager_errors module_manager_errors
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq module_manager_exceptions module_manager_exceptions
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq module_manager_init module_manager_init
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq module_manager_utils module_manager_utils
run_rtgen copendaq/modulemanager opendaq/modulemanager opendaq orphaned_modules orphaned_modules

# opendaq opendaq

run_rtgen copendaq/opendaq opendaq/opendaq opendaq client_type client_type
run_rtgen copendaq/opendaq opendaq/opendaq opendaq config_provider config_provider
run_rtgen copendaq/opendaq opendaq/opendaq opendaq create_device create_device
run_rtgen copendaq/opendaq opendaq/opendaq opendaq errors errors
run_rtgen copendaq/opendaq opendaq/opendaq opendaq exceptions exceptions
run_rtgen copendaq/opendaq opendaq/opendaq opendaq instance instance
run_rtgen copendaq/opendaq opendaq/opendaq opendaq instance_builder instance_builder
run_rtgen copendaq/opendaq opendaq/opendaq opendaq opendaq opendaq
run_rtgen copendaq/opendaq opendaq/opendaq opendaq opendaq_config.h opendaq_config.h
run_rtgen copendaq/opendaq opendaq/opendaq opendaq opendaq_init opendaq_init
run_rtgen copendaq/opendaq opendaq/opendaq opendaq path_tool path_tool
run_rtgen copendaq/opendaq opendaq/opendaq opendaq version version

# opendaq reader

run_rtgen copendaq/reader opendaq/reader opendaq block_reader block_reader
run_rtgen copendaq/reader opendaq/reader opendaq block_reader_builder block_reader_builder
run_rtgen copendaq/reader opendaq/reader opendaq block_reader_status block_reader_status
run_rtgen copendaq/reader opendaq/reader opendaq multi_reader multi_reader
run_rtgen copendaq/reader opendaq/reader opendaq multi_reader_builder multi_reader_builder
run_rtgen copendaq/reader opendaq/reader opendaq multi_reader_status multi_reader_status
run_rtgen copendaq/reader opendaq/reader opendaq multi_typed_reader multi_typed_reader
run_rtgen copendaq/reader opendaq/reader opendaq packet_reader packet_reader
run_rtgen copendaq/reader opendaq/reader opendaq read_info read_info
run_rtgen copendaq/reader opendaq/reader opendaq reader reader
run_rtgen copendaq/reader opendaq/reader opendaq reader_config reader_config
run_rtgen copendaq/reader opendaq/reader opendaq reader_domain_info reader_domain_info
run_rtgen copendaq/reader opendaq/reader opendaq reader_errors reader_errors
run_rtgen copendaq/reader opendaq/reader opendaq reader_exceptions reader_exceptions
run_rtgen copendaq/reader opendaq/reader opendaq reader_status reader_status
run_rtgen copendaq/reader opendaq/reader opendaq reader_utils reader_utils
run_rtgen copendaq/reader opendaq/reader opendaq sample_reader sample_reader
run_rtgen copendaq/reader opendaq/reader opendaq signal_reader signal_reader
run_rtgen copendaq/reader opendaq/reader opendaq stream_reader stream_reader
run_rtgen copendaq/reader opendaq/reader opendaq stream_reader_builder stream_reader_builder
run_rtgen copendaq/reader opendaq/reader opendaq tail_reader tail_reader
run_rtgen copendaq/reader opendaq/reader opendaq tail_reader_builder tail_reader_builder
run_rtgen copendaq/reader opendaq/reader opendaq tail_reader_status tail_reader_status
run_rtgen copendaq/reader opendaq/reader opendaq time_reader time_reader
run_rtgen copendaq/reader opendaq/reader opendaq typed_reader typed_reader

# opendaq scheduler

run_rtgen copendaq/scheduler opendaq/scheduler opendaq awaitable awaitable
run_rtgen copendaq/scheduler opendaq/scheduler opendaq graph_visualization graph_visualization
run_rtgen copendaq/scheduler opendaq/scheduler opendaq scheduler scheduler
run_rtgen copendaq/scheduler opendaq/scheduler opendaq scheduler_errors scheduler_errors
run_rtgen copendaq/scheduler opendaq/scheduler opendaq scheduler_exceptions scheduler_exceptions
run_rtgen copendaq/scheduler opendaq/scheduler opendaq task task
run_rtgen copendaq/scheduler opendaq/scheduler opendaq task_flow task_flow
run_rtgen copendaq/scheduler opendaq/scheduler opendaq task_graph task_graph
run_rtgen copendaq/scheduler opendaq/scheduler opendaq task_internal task_internal
run_rtgen copendaq/scheduler opendaq/scheduler opendaq task_ptr.custom task_ptr.custom
run_rtgen copendaq/scheduler opendaq/scheduler opendaq work work

# opendaq server

run_rtgen copendaq/server opendaq/server opendaq server server
run_rtgen copendaq/server opendaq/server opendaq server_type server_type

# opendaq signal

run_rtgen copendaq/signal opendaq/signal opendaq allocator allocator
run_rtgen copendaq/signal opendaq/signal opendaq binary_data_packet binary_data_packet
run_rtgen copendaq/signal opendaq/signal opendaq connection connection
run_rtgen copendaq/signal opendaq/signal opendaq connection_internal connection_internal
run_rtgen copendaq/signal opendaq/signal opendaq data_descriptor data_descriptor
run_rtgen copendaq/signal opendaq/signal opendaq data_descriptor_builder data_descriptor_builder
run_rtgen copendaq/signal opendaq/signal opendaq data_packet data_packet
run_rtgen copendaq/signal opendaq/signal opendaq data_rule data_rule
run_rtgen copendaq/signal opendaq/signal opendaq data_rule_builder data_rule_builder
run_rtgen copendaq/signal opendaq/signal opendaq data_rule_calc data_rule_calc
run_rtgen copendaq/signal opendaq/signal opendaq data_rule_calc_private data_rule_calc_private
run_rtgen copendaq/signal opendaq/signal opendaq deleter deleter
run_rtgen copendaq/signal opendaq/signal opendaq dimension dimension
run_rtgen copendaq/signal opendaq/signal opendaq dimension_builder dimension_builder
run_rtgen copendaq/signal opendaq/signal opendaq dimension_rule dimension_rule
run_rtgen copendaq/signal opendaq/signal opendaq dimension_rule_builder dimension_rule_builder
run_rtgen copendaq/signal opendaq/signal opendaq event_packet event_packet
run_rtgen copendaq/signal opendaq/signal opendaq event_packet_ids event_packet_ids
run_rtgen copendaq/signal opendaq/signal opendaq event_packet_params event_packet_params
run_rtgen copendaq/signal opendaq/signal opendaq event_packet_utils event_packet_utils
run_rtgen copendaq/signal opendaq/signal opendaq input_port input_port
run_rtgen copendaq/signal opendaq/signal opendaq input_port_config input_port_config
run_rtgen copendaq/signal opendaq/signal opendaq input_port_config_ptr.custom input_port_config_ptr.custom
run_rtgen copendaq/signal opendaq/signal opendaq input_port_notifications input_port_notifications
run_rtgen copendaq/signal opendaq/signal opendaq input_port_private input_port_private
run_rtgen copendaq/signal opendaq/signal opendaq packet packet
run_rtgen copendaq/signal opendaq/signal opendaq packet_destruct_callback packet_destruct_callback
run_rtgen copendaq/signal opendaq/signal opendaq range range
run_rtgen copendaq/signal opendaq/signal opendaq range_type range_type
run_rtgen copendaq/signal opendaq/signal opendaq reference_domain_info reference_domain_info
run_rtgen copendaq/signal opendaq/signal opendaq reference_domain_info_builder reference_domain_info_builder
run_rtgen copendaq/signal opendaq/signal opendaq reference_domain_offset_adder reference_domain_offset_adder
run_rtgen copendaq/signal opendaq/signal opendaq reusable_data_packet reusable_data_packet
run_rtgen copendaq/signal opendaq/signal opendaq rule_private rule_private
run_rtgen copendaq/signal opendaq/signal opendaq sample_type sample_type
run_rtgen copendaq/signal opendaq/signal opendaq sample_type_traits sample_type_traits
run_rtgen copendaq/signal opendaq/signal opendaq scaling scaling
run_rtgen copendaq/signal opendaq/signal opendaq scaling_builder scaling_builder
run_rtgen copendaq/signal opendaq/signal opendaq scaling_calc scaling_calc
run_rtgen copendaq/signal opendaq/signal opendaq scaling_calc_private scaling_calc_private
run_rtgen copendaq/signal opendaq/signal opendaq signal signal
run_rtgen copendaq/signal opendaq/signal opendaq signal_config signal_config
run_rtgen copendaq/signal opendaq/signal opendaq signal_errors signal_errors
run_rtgen copendaq/signal opendaq/signal opendaq signal_events signal_events
run_rtgen copendaq/signal opendaq/signal opendaq signal_exceptions signal_exceptions
run_rtgen copendaq/signal opendaq/signal opendaq signal_private signal_private
run_rtgen copendaq/signal opendaq/signal opendaq signal_utils signal_utils

# opendaq streaming

run_rtgen copendaq/streaming opendaq/streaming opendaq mirrored_device mirrored_device
run_rtgen copendaq/streaming opendaq/streaming opendaq mirrored_device_config mirrored_device_config
run_rtgen copendaq/streaming opendaq/streaming opendaq mirrored_signal_config mirrored_signal_config
run_rtgen copendaq/streaming opendaq/streaming opendaq mirrored_signal_private mirrored_signal_private
run_rtgen copendaq/streaming opendaq/streaming opendaq streaming streaming
run_rtgen copendaq/streaming opendaq/streaming opendaq streaming_private streaming_private
run_rtgen copendaq/streaming opendaq/streaming opendaq streaming_type streaming_type
run_rtgen copendaq/streaming opendaq/streaming opendaq subscription_event_args subscription_event_args

# opendaq synchronization

run_rtgen copendaq/synchronization opendaq/synchronization opendaq sync_component sync_component
run_rtgen copendaq/synchronization opendaq/synchronization opendaq sync_component_private sync_component_private

# opendaq utility

run_rtgen copendaq/utility opendaq/utility opendaq ids_parser ids_parser
run_rtgen copendaq/utility opendaq/utility opendaq mem_pool_allocator mem_pool_allocator
run_rtgen copendaq/utility opendaq/utility opendaq utility_errors utility_errors
run_rtgen copendaq/utility opendaq/utility opendaq utility_exceptions utility_exceptions

