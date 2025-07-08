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
#include <condition_variable>
#include <coretypes/deserializer.h>
#include <coreobjects/core_event_args_impl.h>
#include <coreobjects/eval_value_factory.h>
#include <coretypes/intfs.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/packet_buffer.h>
#include <opendaq/packet_buffer_builder_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/deleter_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class PacketBufferImpl : public ImplementationOf<IPacketBuffer>
{
public:

    PacketBufferImpl(const PacketBufferBuilderPtr& builder);

    ErrCode INTERFACE_FUNC createPacket(SizeT sampleCount, IDataDescriptor* desc, IPacket* domainPacket, IDataPacket** packet) override;
    ErrCode INTERFACE_FUNC getAvailableMemory(SizeT* count) override;
    ErrCode INTERFACE_FUNC getAvailableSampleCount(IDataDescriptor* desc, SizeT* count) override;
    ErrCode INTERFACE_FUNC resize(SizeT sizeInBytes) override;

    ErrCode INTERFACE_FUNC getMaxAvailableContinousSampleCount(IDataDescriptor* desc, SizeT* count) override;
    ErrCode INTERFACE_FUNC getAvailableContinousSampleLeft(IDataDescriptor* desc, SizeT* count) override;
    ErrCode INTERFACE_FUNC getAvailableContinousSampleRight(IDataDescriptor* desc, SizeT* count) override;

protected:

    ErrCode Write(size_t sizeOfPackets, void** memPos);

    ErrCode Read(void* beginningOfDelegatedSpace, size_t sizeOfPackets);

    ErrCode CleanOosPackets();

    std::vector<uint8_t> data;
    void* readPos;
    void* writePos;
    bool isFull;
    bool underReset;

    size_t sizeInBytes;

    std::mutex readWriteMutex;
    std::condition_variable resizeSync;

    std::priority_queue<std::pair<uint8_t*, size_t>, std::vector<std::pair<uint8_t*, size_t>>, std::greater<std::pair<uint8_t*, size_t>>>
        oosPackets;

    ContextPtr context;
};

END_NAMESPACE_OPENDAQ
