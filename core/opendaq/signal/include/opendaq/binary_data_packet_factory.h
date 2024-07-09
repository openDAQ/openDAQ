/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/data_packet_ptr.h>
#include <opendaq/binary_data_packet.h>
#include <opendaq/deleter_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_packets
 * @addtogroup opendaq_packet_factories Factories
 * @{
 */

/*!
 * @brief Creates a data packet with binary sample type with a given descriptor and memory size of a sample.
 * @param descriptor The descriptor of the signal sending the data.
 * @param sampleSize the memory size of the binary sample.
 *
 * Binary value packet contains exactly one sample of binary type. The size of the sample is provided with
 * `sampleSize` argument.
 */
inline DataPacketPtr BinaryDataPacket(
    const DataPacketPtr& domainPacket,
    const DataDescriptorPtr& descriptor,
    uint64_t sampleSize)
{
    DataPacketPtr obj(BinaryDataPacket_Create(domainPacket, descriptor, sampleSize));
    return obj;
}

/*!
 * @brief Creates a data packet with binary sample type with a given descriptor, memory size of a sample,
 * pointer to an existing memory location and a deleter.
 * @param descriptor The descriptor of the signal sending the data.
 * @param sampleSize the memory size of the binary sample.
 * @param data Pointer to a custom memory location
 * @param delter Deleter callback to free the memory
 *
 * Binary data packet contains exactly one sample of binary type. The size of the sample is provided with
 * `sampleSize` argument. This factory does not allocate any memory for the data. Instead the memory is
 * provided through `data` parameter. Deleter callback is called when the packet is destroyed. The callback
 * should free the memory.
 */
inline DataPacketPtr BinaryDataPacketWithExternalMemory(
    const DataPacketPtr& domainPacket,
    const DataDescriptorPtr& descriptor, uint64_t sampleSize,
    void* data,
    const DeleterPtr& deleter)
{
    DataPacketPtr obj(BinaryDataPacketWithExternalMemory_Create(domainPacket, descriptor, sampleSize, data, deleter));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
