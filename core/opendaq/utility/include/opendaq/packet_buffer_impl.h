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
#include <queue>
#include <coretypes/deserializer.h>
#include <coreobjects/core_event_args_impl.h>
#include <coreobjects/eval_value_factory.h>
#include <coretypes/intfs.h>
#include <coretypes/validation.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/component_deserialize_context_ptr.h>
#include <opendaq/component_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/tags_private_ptr.h>
#include <opendaq/tags_ptr.h>
#include <opendaq/packet_buffer.h>

BEGIN_NAMESPACE_OPENDAQ

class PacketBufferImpl : public ImplementationOf<IPacketBuffer>
{
    PacketBufferImpl(IPacketBufferBuilder* builder);

    ErrCode INTERFACE_FUNC createPacket(SizeT SampleCount, IDataDescriptor* desc, IPacket* domainPacket, IPacket** packet) override;
    ErrCode INTERFACE_FUNC getAvailableMemory(SizeT* count) override;
    ErrCode INTERFACE_FUNC getAvailableSampleCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC resize(IPacketBufferBuilder* builder) override;

protected:

    // Write(size_t* sampleCount, void** memPos)
    // Read(void* beginningOfDelegatedSpace, size_t sampleCount, size_t rawSize)

    // Is this the place for concurency variables storage (mutex mxFlip, condition_variable cv, ...)

    // and other storage stuff...

    // std::vector<uint8_t> data, void* readPos, void* writePos

    void* readPos;
    void* writePos;
    std::vector<uint8_t> data;
    bool isFull;
    bool underReset;

    size_t sizeOfMem;
    size_t rawSampleSize;

    std::priority_queue<std::pair<void*, size_t>, std::vector<std::pair<void*, size_t>>, std::greater<std::pair<void*, size_t>>>
        oos_packets;

    std::function<void(void*)> deleterFunction;
    // size_t sizeOfMem, size_t rawSampleSize, bool isFull, bool underReset

    // priority_queue, callback function
private:

    DataDescriptorPtr descriptor;
    ContextPtr context;
    size_t sampleCount;
};

inline ErrCode PacketBufferImpl::createPacket(SizeT SampleCount, IDataDescriptor* desc, IPacket* domainPacket, IPacket** packet)
{
    
}

inline ErrCode PacketBufferImpl::getAvailableMemory(SizeT* count)
{

}

inline ErrCode PacketBufferImpl::getAvailableSampleCount(SizeT* count)
{

}

inline ErrCode PacketBufferImpl::resize(IPacketBufferBuilder* builder)
{

}

//OPENDAQ_REGISTER_DESERIALITE_FACTORY(PacketBufferImpl);

END_NAMESPACE_OPENDAQ
