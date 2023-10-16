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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/coretypes.h>
#include <opendaq/signal.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_streamings
 * @addtogroup opendaq_streaming_streaming Streaming
 * @{
 */

/*!
 * @brief Represents the client-side part of a streaming service responsible for initiating
 * communication with the openDAQ device streaming server and processing the received data.
 * Wraps the client-side implementation details of the particular data transfer protocol used by
 * openDAQ to send processed/acquired data from devices running an openDAQ Server to an
 * openDAQ Client.
 *
 * The Streaming is used as a selectable data source for mirrored signals. For this, it
 * provides methods, allowing mirrored signals to be added/removed dynamically,
 * to enable/disable the use of Streaming as a data source of these signals.
 * Forwarding of packets received from the remote device through the data transfer protocol down to
 * the signal path is enabled when the following conditions are met:
 * - Streaming object itself is in active state.
 * - Streaming is selected as an active source of the corresponding signal.
 *
 * Usually, the data transfer protocol provides information about the signals whose data can be sent
 * over the protocol. It allows the implementation to reject unsupported signals from being added to
 * the streaming. Each Streaming object provides the string representation of a connection address
 * used to connect to the streaming service of the device. This string representation is used as
 * a unique ID to determine the streaming source for the mirrored signal.
 */

DECLARE_OPENDAQ_INTERFACE(IStreaming, IBaseObject)
{
    /*!
     * @brief Gets the active state of the Streaming.
     * @param[out] active True if the Streaming is active; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC getActive(Bool* active) = 0;

    /*!
     * @brief Sets the Streaming to be either active or inactive.
     * @param active The new active state of the Streaming.
     */
    virtual ErrCode INTERFACE_FUNC setActive(Bool active) = 0;

    // [elementType(signals, ISignal)]
    /*!
     * @brief Adds signals to the Streaming.
     * @param signals The list of signals to be added.
     * @retval OPENDAQ_ERR_DUPLICATEITEM if a signal on the list is already added to the Streaming.
     * @retval OPENDAQ_ERR_NOINTERFACE if a signal on the list is not a mirrored signal.
     *
     * After a signal is added to the Streaming, the Streaming automatically appears in the list of
     * available streaming sources of a signal.
     */
    virtual ErrCode INTERFACE_FUNC addSignals(IList* signals) = 0;

    // [elementType(signals, ISignal)]
    /*!
     * @brief Removes signals from the Streaming.
     * @param signals The list of signals to be removed.
     * @retval OPENDAQ_ERR_NOTFOUND if a signal on the list was not added to the Streaming.
     */
    virtual ErrCode INTERFACE_FUNC removeSignals(IList* signals) = 0;

    /*!
     * @brief Removes all added signals from the Streaming.
     */
    virtual ErrCode INTERFACE_FUNC removeAllSignals() = 0;

    /*!
     * @brief Gets the string representation of a connection address used to connect to the streaming
     * service of the device.
     * @param[out] connectionString The string used to connect to the streaming service.
     */
    virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) const = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
