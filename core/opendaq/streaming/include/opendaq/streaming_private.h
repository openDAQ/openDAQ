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
#include <opendaq/mirrored_signal_config_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/device_ptr.h>

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
     * @param signalRemoteId The global remote ID of the signal to be subscribed.
     * @param domainSignalRemoteId The global remote ID of the domain signal of the signal to be subscribed.
     */
    virtual ErrCode INTERFACE_FUNC subscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId) = 0;

    /*!
     * @brief Disables delivering packets from server to client for signal via the streaming
     * @param signalRemoteId The global remote ID of the signal to be unsubscribed.
     * @param domainSignalRemoteId The remote global ID of the domain signal of the signal to be unsubscribed.
     */
    virtual ErrCode INTERFACE_FUNC unsubscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId) = 0;

    /*!
     * @brief Removes added signal without removing the streaming source from it.
     * @param signalRemoteId The global remote ID of the removed signal.
     * @retval OPENDAQ_ERR_NOTFOUND if a signal with corresponding remote Id was not added to the Streaming.
     */
    virtual ErrCode INTERFACE_FUNC detachRemovedSignal(const StringPtr& signalRemoteId) = 0;

    /*!
     * @brief Sets the reference to the device that owns the streaming object.
     * @param device The device to which the streaming object is attached.
     */
    virtual ErrCode INTERFACE_FUNC setOwnerDevice(const DevicePtr& device) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
