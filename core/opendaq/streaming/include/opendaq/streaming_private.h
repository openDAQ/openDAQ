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
#include <opendaq/mirrored_signal_config_ptr.h>
#include <opendaq/event_packet_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_streamings
 * @addtogroup opendaq_streaming_streaming Streaming
 * @{
 */

/*!
 * @brief Internal functions used by openDAQ core. This interface should never be used in
 * client SDK or module code.
 */
DECLARE_OPENDAQ_INTERFACE(IStreamingPrivate, IBaseObject)
{
    /*!
     * @brief Enables delivering packets from server to client for signal via the streaming
     * @param signal The signal to be subscribed.
     */
    virtual ErrCode INTERFACE_FUNC subscribeSignal(const MirroredSignalConfigPtr& signal) = 0;

    /*!
     * @brief Disables delivering packets from server to client for signal via the streaming
     * @param signal The signal to be unsubscribed.
     */
    virtual ErrCode INTERFACE_FUNC unsubscribeSignal(const MirroredSignalConfigPtr& signal) = 0;

    /*!
     * @brief Creates an initial DataDescriptor Changed Event Packet for a signal using data received
     * via the streaming.
     * @param signal The signal for which the event packet should be created.
     * @return The created DataDescriptor Changed Event Packet
     */
    virtual EventPacketPtr INTERFACE_FUNC createDataDescriptorChangedEventPacket(const MirroredSignalConfigPtr& signal) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
