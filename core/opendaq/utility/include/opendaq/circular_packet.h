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

#include <opendaq/data_packet_ptr.h>
#include <opendaq/context_ptr.h>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <chrono>
#include <iostream>
#include <functional>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/logger_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/deleter_factory.h>
#include <opendaq/packet_factory.h>

BEGIN_NAMESPACE_OPENDAQ

// Should be in namespace "buffer"!
// Namespaces should be as short as possible
// Namespaces form groupings for commonly used names
namespace bufferReturnCodes
{
    enum class ReturnCodesPacketBuffer : EnumType
    {
        Ok = 0,
        AdjustedSize = 1,
        OutOfMemory = 2,
        Failure = 3
    };
}

// COMMENT: Init should be the only used param for  constructor. The contents should be described in comments.
// Structs have the benefit of being extendable in the future; putting all params in the constructor defeats the purpose of the struct.
struct PacketBufferInit
{
    /*??*/ PUBLIC_EXPORT PacketBufferInit(size_t memSize, const ContextPtr& context);
    PUBLIC_EXPORT PacketBufferInit(size_t sampleSize, size_t sampleSizeInMilliseconds, const ContextPtr& context);
    PUBLIC_EXPORT PacketBufferInit(size_t sampleSize, std::chrono::milliseconds duration, const ContextPtr& context);

    PUBLIC_EXPORT PacketBufferInit(const DataDescriptorPtr& descriptor, const ContextPtr& context);

private:
    friend class PacketBuffer;

    DataDescriptorPtr descriptor;
    ContextPtr context;
    size_t sampleCount;
};


// COMMENT: Testing methods belong in test suite
// This is a Testing Mock Packet, it is not intended for actual use
class Packet;

class PacketBuffer
{
    // Dangling comment?
    

public:

    PUBLIC_EXPORT PacketBuffer(const PacketBufferInit& instructions);

    PUBLIC_EXPORT ~PacketBuffer();

    // When calling resize, reset gets called internally
    // COMMENT: Is this really resize? It accepts a whole new instruction set.
    PUBLIC_EXPORT void resize(const PacketBufferInit& instructions);

    // int => return code
    // COMMENT: Explain what this does at edge conditions.
    PUBLIC_EXPORT size_t getAvailableSampleCount() const;

    // Do we need the data descriptor here?
    // const& params
    PUBLIC_EXPORT DataPacketPtr createPacket(size_t* sampleCount,
                                             DataDescriptorPtr dataDescriptor,
                                             DataPacketPtr& domainPacket);
    bool isEmpty() const;

    int reset();

protected:
    // COMMENT: Testing methods belong in test suite
    // This is a test function, it has no intended use outside of unit tests for internal logic of the buffer
    //Packet createPacket(size_t* sampleCount, size_t dataDescriptor);

    bufferReturnCodes::ReturnCodesPacketBuffer Write(size_t* sampleCount, void** memPos);

    bufferReturnCodes::ReturnCodesPacketBuffer Read(void* beginningOfDelegatedSpace, size_t sampleCount, size_t rawSize);

    std::mutex mxFlip;
    std::condition_variable cv;
    
    std::vector<uint8_t> data;
    void* writePos;
    void* readPos;

    // COMMENT: Naming? 
    size_t sizeOfMem;       // This is not appearent what the name is (torej kaj naj bi to predstavljalo)
    size_t rawSampleSize;
    
    // COMMENT: Vars should be renamed to be consistent
    bool bIsFull;
    bool bUnderReset;

    // COMMENT: Naming? Are both needed?
    // This is a temporary solution for
    // situation of the sampleCount being too big to fit
    size_t sizeAdjusted;
    bool bAdjustedSize;

    // Out-of-scope packets (oos abbreviation)
    std::priority_queue<std::pair<void*, size_t>, std::vector<std::pair<void*, size_t>>, std::greater<std::pair<void*, size_t>>> oos_packets; // COMMENT: snake_case?
    std::function<void(void*)> deleterFunction;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
    ContextPtr context;
};


END_NAMESPACE_OPENDAQ
