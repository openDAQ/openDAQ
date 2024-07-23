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
#include <opendaq/block_reader.h>
#include <opendaq/block_reader_builder_ptr.h>
#include <opendaq/block_reader_status_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/reader_impl.h>
#include <opendaq/signal_ptr.h>

#include <condition_variable>
#include <chrono>
#include <list>

BEGIN_NAMESPACE_OPENDAQ

struct BlockReadInfo
{
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;
    using DataPacketsQueueType = std::list<DataPacketPtr>;

    DataPacketsQueueType dataPacketsQueue;
    DataPacketsQueueType::iterator currentDataPacketIter{dataPacketsQueue.end()};

    SizeT prevSampleIndex{};
    SizeT writtenSampleCount{};

    SizeT remainingSamplesToRead{};

    void* values{};
    void* domainValues{};

    Clock::duration timeout;

    Clock::time_point startTime;

    BlockReadInfo() = default;

    BlockReadInfo(const BlockReadInfo& other)
        : dataPacketsQueue(other.dataPacketsQueue)
        , prevSampleIndex(other.prevSampleIndex)
        , writtenSampleCount(other.writtenSampleCount)
        , remainingSamplesToRead(other.remainingSamplesToRead)
        , values(other.values)
        , domainValues(other.domainValues)
        , timeout(other.timeout)
        , startTime(other.startTime)
    {
        using ConstIterType = DataPacketsQueueType::const_iterator;
        auto dataPacketIterPos = std::distance<ConstIterType>(other.dataPacketsQueue.begin(), other.currentDataPacketIter);
        currentDataPacketIter = std::next(dataPacketsQueue.begin(), dataPacketIterPos);
    }

    BlockReadInfo& operator=(const BlockReadInfo& other)
    {
        if (this == &other)
            return *this;

        dataPacketsQueue = other.dataPacketsQueue;
        prevSampleIndex = other.prevSampleIndex;
        writtenSampleCount = other.writtenSampleCount;
        remainingSamplesToRead = other.remainingSamplesToRead;
        values = other.values;
        domainValues = other.domainValues;
        timeout = other.timeout;
        startTime = other.startTime;

        using ConstIterType = DataPacketsQueueType::const_iterator;
        auto dataPacketIterPos = std::distance<ConstIterType>(other.dataPacketsQueue.begin(), other.currentDataPacketIter);
        currentDataPacketIter = std::next(dataPacketsQueue.begin(), dataPacketIterPos);

        return *this;
    }

    void rewindQueue(SizeT blockSize, SizeT overlappedBlockSize)
    {
        if (writtenSampleCount % blockSize == 0)
        {
            const auto initialRewindSamples = overlappedBlockSize;
            auto rewindSamples = initialRewindSamples;
            do
            {
                auto packetSampleCount = currentDataPacketIter->getSampleCount();

                if (rewindSamples > packetSampleCount)
                {
                    --currentDataPacketIter;
                    rewindSamples -= packetSampleCount;
                }
                else
                {
                    if (prevSampleIndex < rewindSamples)
                    {
                        --currentDataPacketIter;
                        rewindSamples -= prevSampleIndex;
                        prevSampleIndex = currentDataPacketIter->getSampleCount();
                    }
                    else
                    {
                        prevSampleIndex -= rewindSamples;
                        rewindSamples = 0;
                    }
                }
            } while (rewindSamples != 0);

            if (remainingSamplesToRead)
                remainingSamplesToRead += initialRewindSamples;
        }
    }

    void trimQueue(SizeT packetSize, SizeT overlapSize)
    {
        auto packetCount = overlapSize / packetSize;
        packetCount += overlapSize % packetSize ? 1 : 0;
        while (dataPacketsQueue.size() > packetCount)
            dataPacketsQueue.pop_front();
    }

    void prepare(void* outValues, SizeT sampleCount, SizeT blockSize, std::chrono::milliseconds timeoutTime)
    {
        remainingSamplesToRead = sampleCount;
        values = outValues;

        domainValues = nullptr;

        timeout = std::chrono::duration_cast<Duration>(timeoutTime);
        startTime = std::chrono::steady_clock::now();
    }

    void prepareWithDomain(void* outValues, void* domain, SizeT sampleCount, SizeT blockSize, std::chrono::milliseconds timeoutTime)
    {
        remainingSamplesToRead = sampleCount;
        values = outValues;

        domainValues = domain;

        timeout = std::chrono::duration_cast<Duration>(timeoutTime);
        startTime = std::chrono::steady_clock::now();
    }

    void resetSampleIndex()
    {
        prevSampleIndex = 0;
    }

    void clean()
    {
        writtenSampleCount = 0;
        currentDataPacketIter = dataPacketsQueue.end();
        dataPacketsQueue.clear();
        resetSampleIndex();
    }

    [[nodiscard]] Duration durationFromStart() const
    {
        return std::chrono::duration_cast<Duration>(Clock::now() - startTime);
    }
};

struct BlockNotifyInfo
{
    std::mutex mutex;
    std::condition_variable condition;

    bool dataReady{};
};

extern template class ReaderImpl<IBlockReader>;

class BlockReaderImpl final : public ReaderImpl<IBlockReader>
{
public:
    using Super = ReaderImpl<IBlockReader>;

    explicit BlockReaderImpl(const SignalPtr& signal, 
                             SizeT blockSize, 
                             SampleType valueReadType, 
                             SampleType domainReadType, 
                             ReadMode readMode, 
                             SizeT overlap = 0,
                             Bool skipEvents = true);

    explicit BlockReaderImpl(IInputPortConfig* port, 
                             SizeT blockSize, 
                             SampleType valueReadType, 
                             SampleType domainReadType, 
                             ReadMode readMode, 
                             SizeT overlap = 0,
                             Bool skipEvents = false);

    BlockReaderImpl(const ReaderConfigPtr& readerConfig, 
                    SampleType valueReadType, 
                    SampleType domainReadType, 
                    SizeT blockSize, 
                    ReadMode mode, 
                    SizeT overlap = 0);

    BlockReaderImpl(BlockReaderImpl* old, SampleType valueReadType, SampleType domainReadType, SizeT blockSize, SizeT overlap = 0);

    ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) override;

    ErrCode INTERFACE_FUNC connected(IInputPort* inputPort) override;
    ErrCode INTERFACE_FUNC disconnected(IInputPort* inputPort) override;
    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override;
    
    ErrCode INTERFACE_FUNC getEmpty(Bool* empty) override;

    ErrCode INTERFACE_FUNC read(void* blocks, SizeT* count, SizeT timeoutMs = 0, IBlockReaderStatus** status = nullptr) override;
    ErrCode INTERFACE_FUNC
    readWithDomain(void* dataBlocks, void* domainBlocks, SizeT* count, SizeT timeoutMs = 0, IBlockReaderStatus** status = nullptr) override;

    ErrCode INTERFACE_FUNC getBlockSize(SizeT* size) override;
    ErrCode INTERFACE_FUNC getOverlap(SizeT* overlap) override;

private:
    BlockReaderStatusPtr readPackets();
    ErrCode readPacketData();

    SizeT getAvailable() const;
    SizeT getAvailableSamples() const;
    SizeT getTotalSamples() const;

    void initOverlap();
    SizeT calculateBlockCount(SizeT sampleCount) const;

    SizeT blockSize;
    SizeT overlap;
    SizeT overlappedBlockSize;
    SizeT overlappedBlockSizeRemainder;
    BlockReadInfo info{};
    BlockNotifyInfo notify{};
};
END_NAMESPACE_OPENDAQ
