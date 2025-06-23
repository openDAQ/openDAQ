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
#include <opendaq/packet_buffer_builder.h>
#include <opendaq/packet_factory.h>
#include <opendaq/deleter_factory.h>

BEGIN_NAMESPACE_OPENDAQ

enum class PacketCreateStatus
{
    Ok = 0,
    Failed,
    Resetting,
    OutOfMemory,
};

class PacketBufferImpl : public ImplementationOf<IPacketBuffer>
{
public:

    PacketBufferImpl(IPacketBufferBuilder* builder);

    ErrCode INTERFACE_FUNC createPacket(SizeT SampleCount, IDataDescriptor* desc, IPacket* domainPacket, IDataPacket** packet) override;
    ErrCode INTERFACE_FUNC getAvailableMemory(SizeT* count) override;
    ErrCode INTERFACE_FUNC getAvailableSampleCount(IDataDescriptor* desc, SizeT* count) override;
    ErrCode INTERFACE_FUNC resize(SizeT sizeInBytes) override;

protected:


    ErrCode Write(size_t* sampleCount, void** memPos);

    ErrCode Read(void* beginningOfDelegatedSpace, size_t sampleCount, size_t rawSize);

    // Is this the place for concurency variables storage (mutex mxFlip, condition_variable cv, ...)

    // and other storage stuff...

    void* readPos;
    void* writePos;
    std::vector<uint8_t> data;
    bool isFull;
    bool underReset;

    size_t sizeOfMem;
    size_t rawSampleSize;

    std::mutex mxFlip;
    std::condition_variable cv;

    std::priority_queue<std::pair<void*, size_t>, std::vector<std::pair<void*, size_t>>, std::greater<std::pair<void*, size_t>>>
        oos_packets;

    std::function<void(void*)> deleterFunction;

    // size_t sizeOfMem, size_t rawSampleSize, bool isFull, bool underReset

    // priority_queue, callback function

    DataDescriptorPtr descriptor;
    ContextPtr context;
    size_t sampleCount;
};

PacketBufferImpl::PacketBufferImpl(IPacketBufferBuilder* builder)
{
    // Here comes the init of the buffer (that means the fresh std::vector gets called here)
    // also looks into the builder for the class
    SizeT stuff;
    builder->getSizeInBytes(&stuff);
    data = std::vector<uint8_t>(stuff);
    sizeOfMem = stuff;
    readPos = data.data();
    writePos = data.data();
    isFull = false;
    underReset = false;
    rawSampleSize = 1;
    sampleCount = stuff;
    builder->getContext(&context);
}

ErrCode PacketBufferImpl::Write(size_t* sampleCount, void** memPos)
{
    auto writePosVirtuallyAdjusted = (static_cast<uint8_t*>(writePos) + rawSampleSize * *sampleCount);
    auto endOfBuffer = (reinterpret_cast<uint8_t*>(&data) + rawSampleSize * sizeOfMem);
    auto readPosWritePosDiff = (static_cast<uint8_t*>(writePos) - static_cast<uint8_t*>(readPos)) / static_cast<uint8_t>(rawSampleSize);
    //auto writePosEndBufferDiff = (endOfBuffer - static_cast<uint8_t*>(writePos)) / static_cast<uint8_t>(rawSampleSize);
    size_t availableSamples;

    bool sizeAdjusted = false;

    if (readPosWritePosDiff < 0)
    {
        availableSamples = static_cast<size_t>(std::abs(readPosWritePosDiff));
    }
    else
    {
        if (isFull && readPosWritePosDiff == 0)
        {
            DAQ_THROW_EXCEPTION(InvalidStateException);
            // Return problem
        }
        availableSamples = static_cast<size_t>(readPosWritePosDiff);
    }

    if (availableSamples < *sampleCount)
    {

        // Check if at the end of the buffer for write ahead (so write at the beginning of the buffer if the wated size does not fit)
    }
    else
    {
        *memPos = writePos;
        writePos = writePosVirtuallyAdjusted;
    }

    if (writePos == readPos)
    {
        isFull = true;
        if (sizeAdjusted)
        {

            // Is this endpoint still necessary
        }
        return OPENDAQ_SUCCESS;
    }
    if (writePos == (void*) endOfBuffer)
    {
        writePos = &data;
        if (readPos == writePos)
        {
            isFull = true;
        }
        if (sizeAdjusted)
        {
            
            // This endpoint might also be redundant
        }
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::Read(void* beginningOfDelegatedSpace, size_t sampleCount, size_t rawSize)
{
    {
        std::lock_guard<std::mutex> lock(mxFlip);
        if (beginningOfDelegatedSpace != readPos)
        {
            bool scenario = beginningOfDelegatedSpace < readPos;
            if (scenario)
            {
                oos_packets.push(std::make_pair(((uint8_t*) &data + sizeOfMem) + ((uint8_t*) beginningOfDelegatedSpace - (uint8_t*) &data),
                                                sampleCount * rawSize));
            }
            oos_packets.push(std::make_pair(beginningOfDelegatedSpace, sampleCount * rawSize));
            return OPENDAQ_SUCCESS;
        }
        else
        {
            sampleCount *= rawSize;
            auto mm = static_cast<void*>(static_cast<uint8_t*>(beginningOfDelegatedSpace) + sampleCount);

            while (!oos_packets.empty())
            {
                mm = static_cast<void*>(static_cast<uint8_t*>(beginningOfDelegatedSpace) + sampleCount);
                if (oos_packets.top().first == mm)
                {
                    sampleCount += oos_packets.top().second;
                    oos_packets.pop();
                }
                else
                {
                    break;
                }
            }
        }
        if (readPos == writePos && !isFull)
        {
            // Return error codes
        }

        readPos = static_cast<void*>(static_cast<uint8_t*>(readPos) + rawSize * sampleCount);

        if (static_cast<uint8_t*>(readPos) >= static_cast<uint8_t*>(data.data()) + rawSize * sizeOfMem)
            readPos = static_cast<void*>(static_cast<uint8_t*>(readPos) - (rawSize * sizeOfMem));

        isFull = false;
    }
    cv.notify_all();
    return OPENDAQ_SUCCESS;
}

inline ErrCode PacketBufferImpl::createPacket(SizeT SampleCount, IDataDescriptor* desc, IPacket* domainPacket, IDataPacket** packet)
{
    daq::DataRulePtr rule;
    desc->getRule(&rule);
    if (rule.getType() == daq::DataRuleType::Linear)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Packet Buffer does not support Linear Data Rule Type packets.");

    std::unique_lock<std::mutex> lock(mxFlip);
    if (underReset)
    {
        // Here there should be a reintroduction of Logging stuff
        
        *packet =  daq::DataPacketPtr();
        return OPENDAQ_SUCCESS; // This maybe needs to be changed to reflect that a packet was trying to be created 
    }
    desc->getRawSampleSize(&rawSampleSize);
    void* startOfSpace = nullptr;

    this->Write(&sampleCount, &startOfSpace);
    deleterFunction = [&, sampleCnt = sampleCount, startOfSpace = startOfSpace, rawSizeOfSample = rawSampleSize](void*)
        {
            Read(startOfSpace, sampleCnt, rawSizeOfSample);
        };
    auto deleter = daq::Deleter(std::move(deleterFunction));

    *packet = daq::DataPacketWithExternalMemory(domainPacket, desc, sampleCount, startOfSpace, deleter);

    return OPENDAQ_SUCCESS;
}

inline ErrCode PacketBufferImpl::getAvailableMemory(SizeT* count)
{
    *count = ((static_cast<uint8_t*>(writePos) - static_cast<uint8_t*>(data.data())) <=
              (static_cast<uint8_t*>(data.data()) + sizeOfMem - static_cast<uint8_t*>(readPos)))
                 ? (static_cast<uint8_t*>(data.data()) + sizeOfMem - static_cast<uint8_t*>(readPos))
                 : (static_cast<uint8_t*>(writePos) - static_cast<uint8_t*>(data.data()));
    return OPENDAQ_SUCCESS;
}

inline ErrCode PacketBufferImpl::getAvailableSampleCount(IDataDescriptor* desc, SizeT* count)
{
    auto allAvailableSamples = static_cast<uint8_t*>(data.data()) + sizeOfMem;  // End of buffer

    if (writePos == readPos)
    {
        if (isFull)
        {
            *count = 0;
        }
        auto fromEndToPos = allAvailableSamples - static_cast<uint8_t*>(readPos);
        auto fromStartToPos = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());

        *count = (fromStartToPos <= fromEndToPos)
                     ? fromEndToPos
                     : fromStartToPos;
    }
    else
    {
        if (writePos > readPos)
        {
            auto fromEndToPos = allAvailableSamples - static_cast<uint8_t*>(writePos);
            auto fromStartToPos = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());

            *count = (fromStartToPos <= fromEndToPos) ? fromEndToPos : fromStartToPos;
        }
        *count = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(writePos);
    }
    return OPENDAQ_SUCCESS;
}

inline ErrCode PacketBufferImpl::resize(SizeT sizeInBytes)
{
    std::unique_lock<std::mutex> lock(mxFlip);
    underReset = true;

    this->cv.wait(lock, [&]
        {
            return ((readPos == writePos) && (!isFull));
        });

    underReset = false;
    data = std::vector<uint8_t>(sizeInBytes);
    sizeOfMem = sizeInBytes;

    cv.notify_all();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
