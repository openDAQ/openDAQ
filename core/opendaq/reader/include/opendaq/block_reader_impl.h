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
#include <opendaq/block_reader.h>
#include <opendaq/reader_impl.h>
#include <opendaq/signal_ptr.h>
#include <opendaq/data_packet_ptr.h>

#include <condition_variable>
#include <deque>

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

    void reset()
    {
        prevSampleIndex = 0;
    }

    void clean() {
        writtenSampleCount = 0;
        currentDataPacketIter = dataPacketsQueue.end();
        dataPacketsQueue.clear();
        reset();
    }

    [[nodiscard]] Duration durationFromStart() const { return std::chrono::duration_cast<Duration>(Clock::now() - startTime); }
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
                             SizeT overlap,
                             SampleType valueReadType,
                             SampleType domainReadType,
                             ReadMode readMode);

    explicit BlockReaderImpl(IInputPortConfig* port,
                             SizeT blockSize,
                             SizeT overlap,
                             SampleType valueReadType,
                             SampleType domainReadType,
                             ReadMode readMode);

    BlockReaderImpl(const ReaderConfigPtr& readerConfig,
                    SampleType valueReadType,
                    SampleType domainReadType,
                    SizeT blockSize,
                    SizeT overlap,
                    ReadMode mode);

    BlockReaderImpl(BlockReaderImpl* old,
                    SampleType valueReadType,
                    SampleType domainReadType,
                    SizeT blockSize,
                    SizeT overlap);

    ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) override;

    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override;

    ErrCode INTERFACE_FUNC read(void* blocks, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr) override;
    ErrCode INTERFACE_FUNC readWithDomain(void* dataBlocks, void* domainBlocks, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr) override;

    ErrCode INTERFACE_FUNC getBlockSize(SizeT* size) override;
    ErrCode INTERFACE_FUNC getOverlap(SizeT* size) override;

private:
    ErrCode readPackets(IReaderStatus** status, SizeT* count);
    ErrCode readPacketData();

    SizeT getAvailable() const;
    SizeT getAvailableSamples() const;

    SizeT calculateBlockCount(SizeT sampleCount) const;

    SizeT blockSize;
    SizeT overlap;
    SizeT overlappedBlockSize;
    SizeT overlappedBlockSizeRemainder;
    BlockReadInfo info{};
    BlockNotifyInfo notify{};
};

END_NAMESPACE_OPENDAQ
