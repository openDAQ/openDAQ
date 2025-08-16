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
#include <coretypes/baseobject.h>
#include <opendaq/input_port_config.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IInputPortConfig, GenericInputPortPtr, "<opendaq/input_port_config_ptr.h>")]
 */

/*!
 * @ingroup opendaq_streamings
 * @addtogroup opendaq_streaming_input_port Mirrored Input Port
 * @{
 */

/*!
 * @brief Represents the configuration interface for mirrored input ports.
 *
 * Allows listing available streaming sources and selecting the active source for a mirrored input port.
 * Only sources that support client-to-device streaming are allowed. The active sources sends client signal
 * (and the associated domain signal) data to the device, which is then consumed by the connection formed
 * by the input port origin and the mirrored signal connected to it.
 */
DECLARE_OPENDAQ_INTERFACE(IMirroredInputPortConfig, IInputPortConfig)
{
    /*!
     * @brief Gets the global ID of the input port as it appears on the remote device.
     * @param[out] id The input port ID.
     */
    virtual ErrCode INTERFACE_FUNC getRemoteId(IString** id) const = 0;

    // [elementType(streamingConnectionStrings, IString)]
    /*!
     * @brief Gets a list of connection strings of all available streaming sources of the input port.
     * @param[out] streamingConnectionStrings The list of streaming connection strings.
     */
    virtual ErrCode INTERFACE_FUNC getStreamingSources(IList** streamingConnectionStrings) = 0;

    /*!
     * @brief Sets the active streaming source of the input port.
     * @param streamingConnectionString The connection string of streaming source to be set as active.
     * @retval OPENDAQ_ERR_NOTFOUND if the streaming source with the corresponding connection string is not
     * part of the available streaming sources for the input port.
     */
    virtual ErrCode INTERFACE_FUNC setActiveStreamingSource(IString* streamingConnectionString) = 0;

    /*!
     * @brief Gets a connection strings of the active streaming source of the input port.
     * @param[out] streamingConnectionString The connection string of active streaming source.
     */
    virtual ErrCode INTERFACE_FUNC getActiveStreamingSource(IString** streamingConnectionString) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
