/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

#include <opendaq/instance_factory.h>
#include <opendaq/context_factory.h>

#include <opendaq/channel_ptr.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/function_block_type_factory.h>

#include <opendaq/module_manager_factory.h>
#include <opendaq/module_ptr.h>

#include <opendaq/scheduler_ptr.h>
#include <opendaq/task_ptr.h>
#include <opendaq/awaitable_ptr.h>
#include <opendaq/graph_visualization_ptr.h>

#include <opendaq/logger_factory.h>

#include <opendaq/device_ptr.h>
#include <opendaq/device_info_factory.h>

#include <opendaq/server_ptr.h>
#include <opendaq/server_type_ptr.h>

#include <opendaq/signal_factory.h>
#include <opendaq/signal_events_ptr.h>
#include <opendaq/connection_factory.h>
#include <opendaq/input_port_factory.h>

#include <opendaq/component_ptr.h>
#include <opendaq/removable_ptr.h>

#include <opendaq/data_descriptor_factory.h>

#include <opendaq/packet_factory.h>

#include <opendaq/dimension_factory.h>
#include <opendaq/range_factory.h>
#include <coreobjects/coreobjects.h>
#include <opendaq/tags_factory.h>
#include <opendaq/scaling_factory.h>

#include <opendaq/dimension_rule_factory.h>
#include <opendaq/data_rule_factory.h>

#include <opendaq/reader_factory.h>
#include <opendaq/time_reader.h>

#include <opendaq/deleter_factory.h>
#include <opendaq/binary_data_packet_factory.h>

#include <opendaq/function_block_wrapper_factory.h>

#include <opendaq/component_factory.h>
#include <opendaq/folder_factory.h>
#include <opendaq/io_folder_factory.h>

#include <opendaq/streaming_ptr.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <opendaq/streaming_info_factory.h>

#include <opendaq/core_opendaq_event_args_factory.h>


/*!
 * @ingroup opendaq
 * @defgroup opendaq_utility Utility
 */

/*!
 * @ingroup opendaq_utility
 * @defgroup opendaq_scheduler_components Scheduler components
 */

/*!
 * @ingroup opendaq
 * @defgroup opendaq_signal_path Signal path
 */

/*!
 * @ingroup opendaq
 * @defgroup opendaq_devices Devices
 */

/*!
 * @ingroup opendaq
 * @defgroup opendaq_function_blocks Function blocks
 */

/*!
 * @ingroup opendaq
 * @defgroup opendaq_modules Modules
 */

 /*!
  * @ingroup opendaq
  * @defgroup structure_servers Servers
  */

/*!
 * @ingroup opendaq
 * @defgroup opendaq_data Data
 */

/*!
 * @ingroup opendaq_data
 * @defgroup opendaq_packets Packets
 */

/*!
 * @ingroup opendaq_data
 * @defgroup opendaq_readers Readers
 */

/*!
 * @ingroup opendaq_data
 * @defgroup opendaq_writers Writers
 */

/*!
 * @ingroup opendaq_signal_path
 * @defgroup opendaq_signals Signals
 */

/*!
 * @ingroup opendaq
 * @defgroup opendaq_errors_group Errors
 */

/*!
 * @ingroup opendaq
 * @defgroup opendaq_components openDAQ Components
 */

/*!
 * @ingroup opendaq_errors_group
 * @defgroup opendaq_error_codes Error Codes
 * @{
 * @page opendaq_errors Error Codes
 * @brief openDAQ error codes
 * @}
 */

/*!
 * @ingroup opendaq
 * @defgroup opendaq_logger Logger
 */

/*!
 * @ingroup opendaq
 * @defgroup opendaq_streamings Streamings
 */
