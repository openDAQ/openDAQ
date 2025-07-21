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
#include <opendaq/bulk_data_packet.h>
#include <opendaq/data_packet_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates value data packet and associated implicit domain packet using single heap allocation.
 * @param valueDescriptor The value descriptor.
 * @param domainDescriptor The domain descriptor.
 * @param sampleCount The sample count for value/domain packet.
 * @param offset The offset for the implicit domain packet.
 * @param dataAlign The alignment in bytes of underlying memory allocated on the heap for the packet.
 * @returns The data packet created.
 *
 * This factory function can be used to create (and allocate underlying data memory) value packet and its associated domain packet with a
 * single heap allocation. The value and domain packet returned share the reference to the underlying memory. The memory for data and
 * packets is freed when both packets are released.
 *
 * `dataAlign` parameter specifies the alignment in bytes of packet data memory.
 */
inline DataPacketPtr DataPacketWithImplicitDomainPacket(const DataDescriptorPtr& valueDescriptor,
                                                        const DataDescriptorPtr& domainDescriptor,
                                                        size_t sampleCount,
                                                        int64_t offset,
                                                        size_t dataAlign)
{
    DataPacketPtr dataPacket;
    checkErrorInfo(daqCreateValuePacketWithImplicitDomainPacket(&dataPacket, valueDescriptor, domainDescriptor, sampleCount, offset, dataAlign));
    return dataPacket;
}

/*!
 * @brief Creates value data packet and associated explicit domain packet using single heap allocation.
 * @param valueDescriptor The value descriptor.
 * @param domainDescriptor The domain descriptor.
 * @param sampleCount The sample count for value/domain packet.
 * @param dataAlign The alignment in bytes of underlying memory allocated on the heap for the packet.
 * @returns The data packet created.
 *
 * This factory function can be used to create (and allocate underlying data memory) value packet and its associated domain packet with a
 * single heap allocation. The value and domain packet returned share the reference to the underlying memory. The memory for data and
 * packets is freed when both packets are released.
 *
 * `dataAlign` parameter specifies the alignment in bytes of packet data memory.
 */
inline DataPacketPtr
DataPacketWithExplicitDomainPacket(const DataDescriptorPtr& valueDescriptor,
                                   const DataDescriptorPtr& domainDescriptor,
                                   size_t sampleCount,
                                   size_t dataAlign)
{
    DataPacketPtr dataPacket;
    checkErrorInfo(daqCreateValuePacketWithExplicitDomainPacket(&dataPacket, valueDescriptor, domainDescriptor, sampleCount, dataAlign));
    return dataPacket;
}

END_NAMESPACE_OPENDAQ
