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
#include <opendaq/event_packet_ptr.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/multi_typed_reader.h>
#include <opendaq/read_info.h>
#include <opendaq/reader_domain_info.h>
#include <opendaq/reader_status.h>

#include <chrono>

BEGIN_NAMESPACE_OPENDAQ

enum class SyncStatus
{
    Unsynchronized,
    Synchronizing,
    Synchronized
};

struct SignalInfo
{
    InputPortConfigPtr port;
    FunctionPtr valueTransformFunction;
    FunctionPtr domainTransformFunction;
    ReadMode readMode;
    LoggerComponentPtr loggerComponent;
};

struct SignalReader
{
    SignalReader(const InputPortConfigPtr& port,
                 SampleType valueReadType,
                 SampleType domainReadType,
                 ReadMode mode,
                 const LoggerComponentPtr& logger);

    SignalReader(const SignalReader& old,
                 const InputPortNotificationsPtr& listener,
                 SampleType valueReadType,
                 SampleType domainReadType);

    SignalReader(const SignalInfo& old,
                 const InputPortNotificationsPtr& listener,
                 SampleType valueReadType,
                 SampleType domainReadType);

    void readDescriptorFromPort();

    SizeT getAvailable(bool acrossDescriptorChanges) const;
    void handleDescriptorChanged(const EventPacketPtr& eventPacket);
    bool trySetDomainSampleType(const daq::DataPacketPtr& domainPacket) const;
    void setCommonSampleRate(const std::int64_t commonSampleRate);

    void prepare(void* outValues, SizeT count);
    void prepareWithDomain(void* outValues, void* domain, SizeT count);

    void setStartInfo(std::chrono::system_clock::time_point minEpoch, const RatioPtr& maxResolution);

    std::unique_ptr<Comparable> readStartDomain();
    bool isFirstPacketEvent();
    EventPacketPtr readUntilNextDataPacket();
    void skipUntilNextEventPacket();
    bool sync(const Comparable& commonStart);

    ErrCode readPackets();
    ErrCode readPacketData();
    ErrCode handlePacket(const PacketPtr& packet, bool& firstData);

    void* getValuePacketData(const DataPacketPtr& packet) const;
    void reset();

    LoggerComponentPtr loggerComponent;

    std::unique_ptr<Reader> valueReader;
    std::unique_ptr<Reader> domainReader;

    InputPortConfigPtr port;
    ConnectionPtr connection;

    ReadInfo info{};
    ReadMode readMode;
    ReaderDomainInfo domainInfo;

    std::int64_t sampleRate;
    std::int64_t commonSampleRate;
    std::int32_t sampleRateDivider;
    
    bool invalid{false};
    SyncStatus synced{SyncStatus::Unsynchronized};

    NumberPtr packetDelta {0};
};

END_NAMESPACE_OPENDAQ
