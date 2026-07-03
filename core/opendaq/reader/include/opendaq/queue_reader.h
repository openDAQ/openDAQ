/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/enum_flags.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/sample_type.h>
#include <opendaq/sample_reader.h>
#include <opendaq/typed_reading_utils.h>

#include <deque>

BEGIN_NAMESPACE_OPENDAQ

enum class SignalEventType
{
    NoChange = 0,
    ValueChanged,
    DomainChanged,
    DomainAndValueChanged,
    Gap
};

class SignalEvent
{
public:
    SignalEvent(const EventPacketPtr& packet);

    bool merge(const SignalEvent& otherEvent);
    SignalEventType getType() const;
    const DataDescriptorPtr& getDomainDescriptor() const;
    const DataDescriptorPtr& getValueDescriptor() const;

    EventPacketPtr toEventPacket() const;

private:
    void updateType();
private:
    SignalEventType eventType;
    DataDescriptorPtr domainDescriptor;
    DataDescriptorPtr valueDescriptor;
};


enum class AdvanceResult
{
    Success = 0,
    NeedMoreData,
    DomainChanged,
    OvershotError,
    Error
};

enum class QueueReaderIssue : uint32_t
{
    None                            = 0,
    ValueTypesNotConvertible        = 1 << 0,
    DomainTypesNotConvertible       = 1 << 1,
    SampleRateChanged               = 1 << 2,
    UnsupportedDomainRule           = 1 << 3,
    OriginParsingFailed             = 1 << 4
};

class QueueReader
{
public:
    QueueReader(const InputPortConfigPtr& port, // Consider using Connection instead
                 SampleType valueReadType,
                 SampleType domainReadType,
                 ReadMode mode,
                 const LoggerComponentPtr& logger,
                 bool globalIdFromSignal);

public:
    void packetReceived();

    DomainInfo getDomainInfo() const;
    std::unique_ptr<DomainValue> getFirstSampleDomainValue() const;
    AdvanceResult advanceToDomainValue(const DomainValue* domainValue);
    Int getSampleRate() const;

    void consumeLeadingEventPackets();
    void dropOutdatedDomainSegments();
    SizeT getAvailableSamples() const;

    bool hasPendingEvents() const;
    EventPacketPtr popFrontEvent();

    bool isValid() const;

    void domainChangeHandled();

private:
    SignalEventType addEncounteredEvent(const EventPacketPtr& packet);
    void parseDomainDescriptor();
    void parseValueDescriptor();
    void parseCachedDescriptors();
    size_t getNumberOfEventPacketsInQueue();
    bool dropUntilEvent();

private:
    std::deque<PacketPtr> packets;
    std::deque<SignalEvent> events;

    SizeT readingPosition = 0;

    InputPortConfigPtr port;
    ConnectionPtr connection;

    LoggerComponentPtr loggerComponent;

    EnumFlags<QueueReaderIssue> issues;

    ReadMode readMode;

    struct TypedReadingContext
    {
        SampleType domainIn;
        SampleType domainOut;
        ReadLayout domainLayout;
        DomainInfo domainInfo;
        FunctionPtr domainTransform = nullptr;
        
        SampleType valueIn;
        SampleType valueOut;
        ReadLayout valueLayout;
        FunctionPtr valueTransform = nullptr;
    };
    TypedReadingContext typeCtx;

    Int sampleRate = -1;
    Int packetDelta{0};
    bool domainChanged = false;
};

END_NAMESPACE_OPENDAQ