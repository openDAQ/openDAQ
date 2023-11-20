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
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>
#include <opendaq/event_packet.h>
#include <opendaq/streaming.h>
#include <opendaq/signal_config.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(ISignalConfig, GenericSignalConfigPtr, "<opendaq/signal_config_ptr.h>")]
 */

/*!
 * @ingroup opendaq_streamings
 * @addtogroup opendaq_streaming_signal Mirrored Signal
 * @{
 */

/*!
 * @brief Represents configuration interface for mirrored signals. Allows selecting streaming data sources per signal.
 */
DECLARE_OPENDAQ_INTERFACE(IMirroredSignalConfig, ISignalConfig)
{
    /*!
     * @brief Gets the global ID of the signal as it appears on the remote device.
     * @param[out] id The signal id.
     */
    virtual ErrCode INTERFACE_FUNC getRemoteId(IString** id) const = 0;

    // [elementType(streamingConnectionStrings, IString)]
    /*!
     * @brief Gets a list of connection strings of all available streaming sources of the signal.
     * @param[out] streamingConnectionStrings The list of streaming connection strings.
     */
    virtual ErrCode INTERFACE_FUNC getStreamingSources(IList** streamingConnectionStrings) = 0;

    /*!
     * @brief Sets the active streaming source of the signal.
     * @param streamingConnectionString The connection string of streaming source to be set as active.
     * @retval OPENDAQ_ERR_NOTFOUND if the streaming source with the corresponding connection string is not
     * part of the available streaming sources for the signal.
     */
    virtual ErrCode INTERFACE_FUNC setActiveStreamingSource(IString* streamingConnectionString) = 0;

    /*!
     * @brief Gets a connection strings of the active streaming source of the signal.
     * @param[out] streamingConnectionString The connection string of active streaming source.
     */
    virtual ErrCode INTERFACE_FUNC getActiveStreamingSource(IString** streamingConnectionString) = 0;

    /*!
     * @brief Stops the streaming and clears the active streaming source of the signal.
     */
    virtual ErrCode INTERFACE_FUNC deactivateStreaming() = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
