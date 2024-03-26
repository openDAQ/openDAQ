/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/event_packet_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/allocator_ptr.h>
#include <opendaq/deleter_ptr.h>
#include <opendaq/external_allocator_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_packets
 * @addtogroup opendaq_packet_factories Factories
 * @{
 */

/*!
 * @brief Creates a Data packet with a given descriptor, sample count, and an optional
 * packet offset.
 * @param descriptor The descriptor of the signal sending the data.
 * @param sampleCount The number of samples in the packet.
 * @param offset Optional packet offset parameter, used to calculate the data of the packet
 * if the Data rule of the Signal descriptor is not explicit.
 */
inline DataPacketPtr DataPacket(const DataDescriptorPtr& descriptor,
                                uint64_t sampleCount,
                                const NumberPtr& offset = nullptr)
{
    DataPacketPtr obj(DataPacket_Create(descriptor, sampleCount, offset));
    return obj;
}

/*!
 * @brief Creates a Data packet with a given descriptor, sample count,
 * a reference to a packet that describes the domain (time) data, and an optional packet offset.
 * @param domainPacket The Data packet carrying domain data.
 * @param descriptor The descriptor of the signal sending the data.
 * @param sampleCount The number of samples in the packet.
 * @param offset Optional packet offset parameter, used to calculate the data of the packet
 * if the Data rule of the Signal descriptor is not explicit.
 */
inline DataPacketPtr DataPacketWithDomain(const DataPacketPtr& domainPacket,
                                          const DataDescriptorPtr& descriptor,
                                          uint64_t sampleCount,
                                          NumberPtr offset = nullptr)
{
    DataPacketPtr obj(DataPacketWithDomain_Create(domainPacket, descriptor, sampleCount, offset));
    return obj;
}

template <class T>
struct ConstantPosAndValue
{
    uint32_t pos;
    T value;
};

/*!
 * @brief Creates a Data packet with a given constat rule descriptor, initial constant value,
 * and other constant values.
 * @param domainPacket The Data packet carrying domain data.
 * @param descriptor The descriptor of the signal sending the data.
 * @param initialValue The initial constant value.
 * @param otherValues The other constant values.
 *
 * The values in the packet are calculated by constant rule. The initial value is taken as a constant.
 * Other values are used to change the constant value within the packet. Any number of other constant values
 * can be used. Other values are passed as an array of struct with uint32_t sample position field and value field.
 * Value field (as well as initial value) are of type defined as sample type in the descriptor.
 */
template <class T>
DataPacketPtr ConstantDataPacketWithDomain(const DataPacketPtr& domainPacket,
                                           const DataDescriptorPtr& descriptor,
                                           uint64_t sampleCount,
                                           T initialValue,
                                           const std::vector<ConstantPosAndValue<T>>& otherValues = {})
{
    DataPacketPtr obj(ConstantDataPacketWithDomain_Create(
        domainPacket,
        descriptor,
        sampleCount,
        reinterpret_cast<void*>(&initialValue),
        reinterpret_cast<void*>(const_cast<ConstantPosAndValue<T>*>(otherValues.data())),
        otherValues.size()));

    return obj;
}

/*!
 * @brief Creates a Data packet with a given descriptor, sample count,
 * a reference to a packet that describes the domain (time) data,
 * pointer to an existing memory location and a deleter, and an optional packet offset.
 * 
 * @param domainPacket The Data packet carrying domain data.
 * @param descriptor The descriptor of the signal sending the data.
 * @param sampleCount The number of samples in the packet.
 * @param data Pointer to a custom memory location
 * @param deleter Deleter callback to free the memory
 * @param offset Optional packet offset parameter, used to calculate the data of the packet
 * if the Data rule of the Signal descriptor is not explicit.
 */
inline DataPacketPtr DataPacketWithExternalMemory(const DataPacketPtr& domainPacket,
                                          const DataDescriptorPtr& descriptor,
                                          uint64_t sampleCount,
                                          void* data,
                                          const DeleterPtr& deleter,
                                          NumberPtr offset = nullptr,
                                          SizeT bufferSize = std::numeric_limits<SizeT>::max())
{
    DataPacketPtr obj(DataPacketWithExternalMemory_Create(domainPacket,
                                                          descriptor,
                                                          sampleCount,
                                                          offset,
                                                          data,
                                                          deleter,
                                                          bufferSize));
    return obj;
}

/*!
 * @brief Creates and Event packet with a given id and parameter dictionary.
 * @param id The ID of the event.
 * @param parameters The <String, BaseObject> dictionary containing the event parameters.
 */
inline EventPacketPtr EventPacket(const StringPtr& id, const DictPtr<IString, IBaseObject>& parameters)
{
    EventPacketPtr obj(EventPacket_Create(id, parameters));
    return obj;
}

/*!
 * @brief Creates a DataDescriptorChanged Event packet.
 * @param dataDescriptor The data descriptor of the value signal.
 * @param domainDataDescriptor The data descriptor of the domain signal that carries domain data of the value signal.
 *
 * The ID of the packet is "DATA_DESCRIPTOR_CHANGED". Its parameters dictionary contains the keys "DataDescriptor"
 * and "DomainDataDescriptor", carrying their respective data descriptor objects as values.
 */
inline EventPacketPtr DataDescriptorChangedEventPacket(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDataDescriptor)
{
    EventPacketPtr obj(DataDescriptorChangedEventPacket_Create(dataDescriptor, domainDataDescriptor));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
