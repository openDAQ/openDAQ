/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#include <opendaq/data_packet_impl.h>
#include <opendaq/data_packet_ptr.h>

#include <memory>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_packets
 * @addtogroup opendaq_packet_factories Factories
 * @{
 */

/*!
 * @brief Creates a Data packet over external memory that stays valid for as long as @p owner lives.
 * @param domainPacket The Data packet carrying domain data that describes the rate and position of the data.
 * @param descriptor The descriptor of the data the packet contains.
 * @param sampleCount The number of samples in the packet.
 * @param data The external memory holding the samples; it must outlive @p owner.
 * @param owner Any shared pointer whose lifetime bounds the validity of @p data. The packet keeps a copy and
 *              drops it when the packet is destroyed.
 * @param offset Optional packet offset applied by the data rule.
 * @param bufferSize Size of @p data in bytes when it differs from sample count times raw sample size.
 */
inline DataPacketPtr DataPacketWithExternalMemoryOwner(const DataPacketPtr& domainPacket,
                                                       const DataDescriptorPtr& descriptor,
                                                       uint64_t sampleCount,
                                                       void* data,
                                                       std::shared_ptr<void> owner,
                                                       NumberPtr offset = nullptr,
                                                       SizeT bufferSize = std::numeric_limits<SizeT>::max())
{
    return createWithImplementation<IDataPacket, DataPacketImpl<IDataPacket>>(static_cast<IDataPacket*>(domainPacket),
                                                                              static_cast<IDataDescriptor*>(descriptor),
                                                                              static_cast<SizeT>(sampleCount),
                                                                              static_cast<INumber*>(offset),
                                                                              data,
                                                                              std::move(owner),
                                                                              bufferSize);
}

/*!@}*/

END_NAMESPACE_OPENDAQ
