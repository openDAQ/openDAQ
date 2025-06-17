/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <ccommon.h>

#include <ccoretypes/base_object.h>
#include <ccoretypes/binarydata.h>
#include <ccoretypes/boolean.h>
#include <ccoretypes/cloneable.h>
#include <ccoretypes/comparable.h>
#include <ccoretypes/complex_number.h>
#include <ccoretypes/convertible.h>
#include <ccoretypes/coretype.h>
#include <ccoretypes/dictobject.h>
#include <ccoretypes/enumeration_type.h>
#include <ccoretypes/enumeration.h>
#include <ccoretypes/event_args.h>
#include <ccoretypes/event_handler.h>
#include <ccoretypes/event.h>
#include <ccoretypes/float.h>
#include <ccoretypes/freezable.h>
#include <ccoretypes/function.h>
#include <ccoretypes/integer.h>
#include <ccoretypes/iterable.h>
#include <ccoretypes/iterator.h>
#include <ccoretypes/listobject.h>
#include <ccoretypes/number.h>
#include <ccoretypes/procedure.h>
#include <ccoretypes/ratio.h>
#include <ccoretypes/recursive_search.h>
#include <ccoretypes/search_filter.h>
#include <ccoretypes/serializable.h>
#include <ccoretypes/serialized_list.h>
#include <ccoretypes/serialized_object.h>
#include <ccoretypes/serializer.h>
#include <ccoretypes/simple_type.h>
#include <ccoretypes/stringobject.h>
#include <ccoretypes/struct_builder.h>
#include <ccoretypes/struct_type.h>
#include <ccoretypes/struct.h>
#include <ccoretypes/type_manager.h>
#include <ccoretypes/type.h>
#include <ccoretypes/updatable.h>
#include <ccoretypes/version_info.h>

#include <ccoretypes/factories.h>

// #include <ccoreobjects/user_internal.h>
#include <ccoreobjects/argument_info.h>
#include <ccoreobjects/authentication_provider.h>
#include <ccoreobjects/callable_info.h>
#include <ccoreobjects/coercer.h>
#include <ccoreobjects/common.h>
#include <ccoreobjects/core_event_args.h>
#include <ccoreobjects/end_update_event_args.h>
#include <ccoreobjects/eval_value.h>
#include <ccoreobjects/object_lock_guard.h>
#include <ccoreobjects/ownable.h>
#include <ccoreobjects/permission_manager_internal.h>
#include <ccoreobjects/permission_manager.h>
#include <ccoreobjects/permission_mask_builder.h>
#include <ccoreobjects/permissions_builder.h>
#include <ccoreobjects/permissions.h>
#include <ccoreobjects/property_builder.h>
#include <ccoreobjects/property_internal.h>
#include <ccoreobjects/property_object_class_builder.h>
#include <ccoreobjects/property_object_class.h>
#include <ccoreobjects/property_object_internal.h>
#include <ccoreobjects/property_object_protected.h>
#include <ccoreobjects/property_object.h>
#include <ccoreobjects/property_value_event_args.h>
#include <ccoreobjects/property.h>
#include <ccoreobjects/unit_builder.h>
#include <ccoreobjects/unit.h>
#include <ccoreobjects/user.h>
#include <ccoreobjects/validator.h>

#include <copendaq/component/component_private.h>
#include <copendaq/component/component_status_container_private.h>
#include <copendaq/component/component_status_container.h>
#include <copendaq/component/component_type_builder.h>
#include <copendaq/component/component_type.h>
#include <copendaq/component/component.h>
#include <copendaq/component/folder_config.h>
#include <copendaq/component/folder.h>
#include <copendaq/component/removable.h>
#include <copendaq/component/search_filter.h>
#include <copendaq/component/tags_private.h>
#include <copendaq/component/tags.h>
#include <copendaq/component/update_parameters.h>

#include <copendaq/context/context.h>

#include <copendaq/device/address_info_builder.h>
#include <copendaq/device/address_info.h>
#include <copendaq/device/connected_client_info.h>
#include <copendaq/device/connection_status_container_private.h>
#include <copendaq/device/device_domain.h>
#include <copendaq/device/device_info_config.h>
#include <copendaq/device/device_info.h>
#include <copendaq/device/device_network_config.h>
#include <copendaq/device/device_private.h>
#include <copendaq/device/device_type.h>
#include <copendaq/device/device.h>
#include <copendaq/device/io_folder_config.h>
#include <copendaq/device/log_file_info_builder.h>
#include <copendaq/device/log_file_info.h>
#include <copendaq/device/network_interface.h>
#include <copendaq/device/server_capability_config.h>
#include <copendaq/device/server_capability.h>

#include <copendaq/functionblock/channel.h>
#include <copendaq/functionblock/function_block_type.h>
#include <copendaq/functionblock/function_block_wrapper.h>
#include <copendaq/functionblock/function_block.h>
#include <copendaq/functionblock/recorder.h>

#include <copendaq/logger/logger_component.h>
#include <copendaq/logger/logger_sink.h>
#include <copendaq/logger/logger_thread_pool.h>
#include <copendaq/logger/logger.h>

#include <copendaq/modulemanager/discovery_server.h>
#include <copendaq/modulemanager/module_info.h>
#include <copendaq/modulemanager/module_manager.h>
#include <copendaq/modulemanager/module.h>

#include <copendaq/opendaq/config_provider.h>
#include <copendaq/opendaq/errors.h>
#include <copendaq/opendaq/instance_builder.h>
#include <copendaq/opendaq/instance.h>

#include <copendaq/reader/block_reader_builder.h>
#include <copendaq/reader/block_reader_status.h>
#include <copendaq/reader/block_reader.h>
#include <copendaq/reader/multi_reader_builder.h>
#include <copendaq/reader/multi_reader_status.h>
#include <copendaq/reader/multi_reader.h>
#include <copendaq/reader/packet_reader.h>
#include <copendaq/reader/reader_status.h>
#include <copendaq/reader/reader.h>
#include <copendaq/reader/sample_reader.h>
#include <copendaq/reader/stream_reader_builder.h>
#include <copendaq/reader/stream_reader.h>
#include <copendaq/reader/tail_reader_builder.h>
#include <copendaq/reader/tail_reader_status.h>
#include <copendaq/reader/tail_reader.h>

#include <copendaq/scheduler/awaitable.h>
#include <copendaq/scheduler/graph_visualization.h>
#include <copendaq/scheduler/scheduler.h>
#include <copendaq/scheduler/task_graph.h>
#include <copendaq/scheduler/task.h>
#include <copendaq/scheduler/work.h>

#include <copendaq/server/server_type.h>
#include <copendaq/server/server.h>

#include <copendaq/signal/allocator.h>
#include <copendaq/signal/connection.h>
#include <copendaq/signal/data_descriptor_builder.h>
#include <copendaq/signal/data_descriptor.h>
#include <copendaq/signal/data_packet.h>
#include <copendaq/signal/data_rule_builder.h>
#include <copendaq/signal/data_rule.h>
#include <copendaq/signal/deleter.h>
#include <copendaq/signal/dimension_builder.h>
#include <copendaq/signal/dimension_rule_builder.h>
#include <copendaq/signal/dimension_rule.h>
#include <copendaq/signal/dimension.h>
#include <copendaq/signal/event_packet.h>
#include <copendaq/signal/input_port_config.h>
#include <copendaq/signal/input_port_notifications.h>
#include <copendaq/signal/input_port.h>
#include <copendaq/signal/packet_destruct_callback.h>
#include <copendaq/signal/packet.h>
#include <copendaq/signal/range.h>
#include <copendaq/signal/reference_domain_info_builder.h>
#include <copendaq/signal/reference_domain_info.h>
#include <copendaq/signal/scaling_builder.h>
#include <copendaq/signal/scaling.h>
#include <copendaq/signal/signal_config.h>
#include <copendaq/signal/signal_events.h>
#include <copendaq/signal/signal.h>

#include <copendaq/streaming/mirrored_device_config.h>
#include <copendaq/streaming/mirrored_device.h>
#include <copendaq/streaming/mirrored_signal_config.h>
#include <copendaq/streaming/mirrored_signal_private.h>
#include <copendaq/streaming/streaming_type.h>
#include <copendaq/streaming/streaming.h>
#include <copendaq/streaming/subscription_event_args.h>

#include <copendaq/synchronization/sync_component_private.h>
#include <copendaq/synchronization/sync_component.h>

#ifdef __cplusplus
}
#endif
