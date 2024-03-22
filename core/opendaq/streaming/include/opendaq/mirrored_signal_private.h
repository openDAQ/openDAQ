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
#include <opendaq/event_packet_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <opendaq/streaming_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_streamings
 * @addtogroup opendaq_streaming_signal Mirrored Signal private
 * @{
 */

/*!
 * @brief Internal functions used by openDAQ core. This interface should never be used in
 * client SDK or module code.
 */
DECLARE_OPENDAQ_INTERFACE(IMirroredSignalPrivate, IBaseObject)
{
    /*!
     * @brief Handles event packet e.g. packet with changes of the signals descriptors or
     * signal properties
     * @param eventPacket The event packet to be handled.
     * @returns True if the eventPacket should be sent along the signal path; False otherwise.
     */
    virtual Bool INTERFACE_FUNC triggerEvent(const EventPacketPtr& eventPacket) = 0;

    /*!
     * @brief Adds streaming source for signal.
     * @param streaming The Streaming object representing the data source.
     */
    virtual ErrCode INTERFACE_FUNC addStreamingSource(const StreamingPtr& streaming) = 0;

    /*!
     * @brief Removes streaming source for signal.
     * @param streamingConnectionString The connection string of streaming source to be removed.
     */
    virtual ErrCode INTERFACE_FUNC removeStreamingSource(const StringPtr& streamingConnectionString) = 0;

    /*!
     * @brief Handles the completion of subscription acknowledged by the specified streaming source.
     * @param streamingConnectionString The connection string of the streaming source that completed
     * the subscription for the signal.
     */
    virtual void INTERFACE_FUNC subscribeCompleted(const StringPtr& streamingConnectionString) = 0;

    /*!
     * @brief Handles the completion of unsubscription acknowledged by the specified streaming source.
     * @param streamingConnectionString The connection string of the streaming source that completed
     * the unsubscription for the signal.
     */
    virtual void INTERFACE_FUNC unsubscribeCompleted(const StringPtr& streamingConnectionString) = 0;

    virtual DataDescriptorPtr INTERFACE_FUNC getMirroredDataDescriptor() = 0;
    virtual void INTERFACE_FUNC setMirroredDataDescriptor(const DataDescriptorPtr& descriptor) = 0;
    virtual MirroredSignalConfigPtr INTERFACE_FUNC getMirroredDomainSignal() = 0;
    virtual void INTERFACE_FUNC setMirroredDomainSignal(const MirroredSignalConfigPtr& domainSignal) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
