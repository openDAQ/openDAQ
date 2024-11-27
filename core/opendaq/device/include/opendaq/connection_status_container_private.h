/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

#include <coretypes/coretypes.h>
#include <opendaq/streaming.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IEnumeration, EnumerationPtr, "<coretypes/enumeration_ptr.h>")]
 */

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_connection_status_container Devices connections status container private
 * @{
 */

/*!
 * @brief Provides access to private methods for managing the Device's connection statuses.
 *
 * Enables adding, removing, and updating statuses stored in the connection status container.
 * Statuses are identified by unique connection strings and can be accessed via
 * IComponentStatusContainer using name aliases. A configuration status, accessible via alias "ConfigurationStatus",
 * is unique per container and cannot be removed if added.
 *
 * On the other hand, multiple streaming statuses are supported, one per each streaming source attached to the device.
 * Aliases for these follow the pattern "StreamingStatus_n," with optional protocol-based prefixes (e.g.,
 * "OpenDAQNativeStreamingStatus_1", "OpenDAQLTStreamingStatus_2", "StreamingStatus_3" etc.).
 *
 * "ConnectionStatusChanged" Core events are triggered whenever there is a change in the connection status of the openDAQ Device,
 * including parameters such as status name alias, value, protocol type, connection string, and streaming
 * object (nullptr for configuration statuses). Removing streaming statuses also triggers said Core event with
 * "Removed" as the value and nullptr for the streaming object parameters.
 */
DECLARE_OPENDAQ_INTERFACE(IConnectionStatusContainerPrivate, IBaseObject)
{
    /*!
     * @brief Adds a new configuration connection status with the specified connection string and initial value.
     * @param connectionString The connection string identifying the status.
     * @param initialValue The initial value of the status.
     */
    virtual ErrCode INTERFACE_FUNC addConfigurationConnectionStatus(IString* connectionString, IEnumeration* initialValue) = 0;

    /*!
     * @brief Adds a new streaming connection status with the specified connection string, initial value, and streaming object.
     * @param connectionString The connection string identifying the status.
     * @param initialValue The initial value of the status.
     * @param streamingObject The streaming object associated with the status.
     */
    virtual ErrCode INTERFACE_FUNC addStreamingConnectionStatus(IString* connectionString, IEnumeration* initialValue, IStreaming* streamingObject) = 0;

    /*!
     * @brief Removes a streaming connection status associated with the specified connection string.
     * @param connectionString The connection string identifying the status to remove.
     */
    virtual ErrCode INTERFACE_FUNC removeStreamingConnectionStatus(IString* connectionString) = 0;

    /*!
     * @brief Updates the value of an existing connection status.
     * @param connectionString The connection string identifying the status to update.
     * @param value The new value of the status.
     * @param streamingObject The streaming object associated with the connection, used in triggered Core events.
     * Set to nullptr for configuration connections.
     */
    virtual ErrCode INTERFACE_FUNC updateConnectionStatus(IString* connectionString, IEnumeration* value, IStreaming* streamingObject) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
