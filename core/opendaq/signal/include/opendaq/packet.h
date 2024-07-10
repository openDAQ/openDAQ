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
#include <opendaq/allocator.h>
#include <opendaq/packet_destruct_callback.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup opendaq_packets
 * @{
 */
enum class PacketType
{
    None = 0, ///< Undefined packet type
    Data,     ///< Packet is a Data packet
    Event     ///< Packet is an Event packet
};
/*!@}*/

/*#
 * [templated(defaultAliasName: PacketPtr)]
 * [interfaceSmartPtr(IPacket, GenericPacketPtr)]
 */

/*!
 * @ingroup opendaq_packets
 * @addtogroup opendaq_packet Packet
 * @{
 */

/*!
 * @brief Base packet type. Data, Value, and Event packets are all also packets. Provides
 * the packet's unique ID that is unique to a given device, as well as the packet type.
 */
DECLARE_OPENDAQ_INTERFACE(IPacket, IBaseObject)
{
    /*!
     * @brief Gets the packet's type.
     * @param[out] type The packet type.
     */
    virtual ErrCode INTERFACE_FUNC getType(PacketType* type) = 0;

    /*!
     * @brief Subscribes for notification when the packet is destroyed.
     * @param packetDestructCallback The callback that is called when the packet is destroyed.
     */
    virtual ErrCode INTERFACE_FUNC subscribeForDestructNotification(IPacketDestructCallback* packetDestructCallback) = 0;

    /*!
     * @brief Gets the reference count of the packet.
     * @param[out] refCount The reference count of the packet.
     */
    virtual ErrCode INTERFACE_FUNC getRefCount(SizeT* refCount) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
