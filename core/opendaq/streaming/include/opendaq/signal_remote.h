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

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_streamings
 * @addtogroup opendaq_streaming_signal Signal Remote
 * @{
 */

/*!
 * @brief Represents an internal interface for a signal. Provides methods that are called only for
 * mirrored signals.
 * Allows adding/removing streaming data sources per signal.
 */
DECLARE_OPENDAQ_INTERFACE(ISignalRemote, IBaseObject)
{
    /*!
     * @brief Gets the global ID of the signal as it appears on the remote device.
     * @param[out] id The signal id.
     */
    virtual ErrCode INTERFACE_FUNC getRemoteId(IString** id) const = 0;

    /*!
     * @brief Handles event packet e.g. packet with changes of the signals descriptors or
     * signal properties
     * @param eventPacket The event packet to be handled.
     * @param[out] forwardEvent True if the eventPacket should be sent along the signal path; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC triggerEvent(IEventPacket* eventPacket, Bool* forwardEvent) = 0;

    /*!
     * @brief Adds streaming source for signal.
     * @param streaming The Streaming object representing the data source.
     */
    virtual ErrCode INTERFACE_FUNC addStreamingSource(IStreaming* streaming) = 0;

    /*!
     * @brief Removes streaming source for signal.
     * @param streaming The Streaming object representing the data source.
     */
    virtual ErrCode INTERFACE_FUNC removeStreamingSource(IStreaming* streaming) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
