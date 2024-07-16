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
#include <coretypes/common.h>
#include <opendaq/data_packet_ptr.h>

#include <mutex>
#include <condition_variable>
#include <chrono>

BEGIN_NAMESPACE_OPENDAQ

struct ReadInfo
{
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    DataPacketPtr dataPacket;
    SizeT prevSampleIndex{};

    SizeT remainingToRead{};

    void* values{};
    void* domainValues{};

    Clock::duration timeout;
    Clock::time_point startTime;

    void prepare(void* outValues, SizeT count, std::chrono::milliseconds timeoutTime)
    {
        remainingToRead = count;
        values = outValues;

        domainValues = nullptr;

        timeout = std::chrono::duration_cast<Duration>(timeoutTime);
        startTime = std::chrono::steady_clock::now();
    }

    void prepareWithDomain(void* outValues, void* domain, SizeT count, std::chrono::milliseconds timeoutTime)
    {
        remainingToRead = count;
        values = outValues;

        domainValues = domain;

        timeout = std::chrono::duration_cast<Duration>(timeoutTime);
        startTime = std::chrono::steady_clock::now();
    }

    void reset()
    {
        dataPacket = nullptr;
        prevSampleIndex = 0;
    }

    [[nodiscard]] Duration durationFromStart() const
    {
        return std::chrono::duration_cast<Duration>(Clock::now() - startTime);
    }
};

struct NotifyInfo
{
    NotifyInfo() = default;

    NotifyInfo(const NotifyInfo& other)
        : packetReady(other.packetReady)
    {
    }

    NotifyInfo(NotifyInfo&& other) noexcept
        : packetReady(other.packetReady)
    {
    }

    NotifyInfo& operator=(const NotifyInfo& other)
    {
        if (this == &other)
            return *this;

        packetReady = other.packetReady;
        return *this;
    }

    NotifyInfo& operator=(NotifyInfo&& other) noexcept
    {
        if (this == &other)
            return *this;

        packetReady = other.packetReady;
        return *this;
    }

    std::mutex mutex;
    std::condition_variable condition;

    bool packetReady{};
};

END_NAMESPACE_OPENDAQ
