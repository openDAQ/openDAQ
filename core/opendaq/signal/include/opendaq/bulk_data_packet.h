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
#include <opendaq/data_packet.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates multiple data packets with associated domain packets using single heap allocation.
 * @param[out] dataPackets The pointer to the array of data packets.
 * @param valueDescriptors The array of value descriptors.
 * @param domainDescriptors The array of domain descriptors.
 * @param sampleCounts The array of sample counts for each value/domain packet.
 * @param offsets Optional array of offsets for implicit domain packets.
 * @param count The number of packets to create.
 * @param dataAlign The alignment in bytes of underlying memory allocated on the heap for each packet.
 *
 * This factory function can be used to create (and allocate underlying data memory) multiple packets with a single heap allocation.
 * The packets returned share the reference to the underlying memory. The memory for data and packets is freed when all the packets
 * are released.
 *
 * The caller must provide arrays of valid value and domain descriptors. The array size must be of `count` size.
 * The caller must also provide an array of `IDataPacket*` of `count` size, which is filled by the factory when it returns. The reference
 * count of each packet returned is 1.
 *
 * `dataAlign` parameter specifies the alignment in bytes of packet data memory for each packet.
 *
 * `offsets` parameter should be `nullptr` if the packets created have explicit domain. If the domain is implicit, the parameter should point
 * to array of 64 bit offsets, and it should be of size `count`. The function will fail if the domain is not of the same type for all packets,
 * i.e. all domain descriptors must be either implicit or explicit.
 */
extern "C" PUBLIC_EXPORT ErrCode daqBulkCreateDataPackets(
    IDataPacket** dataPackets,
    IDataDescriptor** valueDescriptors,
    IDataDescriptor** domainDescriptors,
    size_t* sampleCounts,
    int64_t* offsets,
    size_t count,
    size_t dataAlign);


/*!
 * @brief Creates value data packet and associated implicit domain packet using single heap allocation.
 * @param[out] dataPacket The pointer to the data packet.
 * @param valueDescriptor The value descriptor.
 * @param domainDescriptor The domain descriptor.
 * @param sampleCount The sample count for value/domain packet.
 * @param offset The offset for the implicit domain packet.
 * @param dataAlign The alignment in bytes of underlying memory allocated on the heap for the packet.
 *
 * This factory function can be used to create (and allocate underlying data memory) value packet and its associated domain packet with a single heap allocation.
 * The value and domain packet returned share the reference to the underlying memory. The memory for data and packets is freed when both packets
 * are released.
 *
 * `dataAlign` parameter specifies the alignment in bytes of packet data memory.
 */
extern "C" PUBLIC_EXPORT ErrCode daqCreateValuePacketWithImplicitDomainPacket(
    IDataPacket** dataPacket,
    IDataDescriptor* valueDescriptor,
    IDataDescriptor* domainDescriptor,
    size_t sampleCount,
    int64_t offset,
    size_t dataAlign);

/*!
 * @brief Creates value data packet and associated explicit domain packet using single heap allocation.
 * @param[out] dataPacket The pointer to the data packet.
 * @param valueDescriptor The value descriptor.
 * @param domainDescriptor The domain descriptor.
 * @param sampleCount The sample count for value/domain packet.
 * @param dataAlign The alignment in bytes of underlying memory allocated on the heap for the packet.
 *
 * This factory function can be used to create (and allocate underlying data memory) value packet and its associated domain packet with a single
 * heap allocation. The value and domain packet returned share the reference to the underlying memory. The memory for data and packets is freed when both
 * packets are released.
 *
 * `dataAlign` parameter specifies the alignment in bytes of packet data memory.
 */
extern "C" PUBLIC_EXPORT ErrCode daqCreateValuePacketWithExplicitDomainPacket(
    IDataPacket** dataPacket,
    IDataDescriptor* valueDescriptor,
    IDataDescriptor* domainDescriptor,
    size_t sampleCount,
    size_t dataAlign);

END_NAMESPACE_OPENDAQ
