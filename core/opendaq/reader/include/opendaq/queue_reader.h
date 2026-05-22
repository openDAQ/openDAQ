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
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/sample_type.h>
#include <opendaq/sample_reader.h>

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
    DataDescriptorPtr getDomainDescriptor() const;
    DataDescriptorPtr getValueDescriptor() const;

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

    // getDomainInfo()
    // getFirstSampleDomainValue()
    // advanceToDomainValue()

    void consumeLeadingEventPackets();
    void dropOutdatedDomainSegments();
    SizeT getAvailableSamples() const;

    bool hasPendingEvents() const;
    EventPacketPtr popFrontEvent();

private:
    SignalEventType addEncounteredEvent(const EventPacketPtr& packet);
    void handleDomainDescriptorChange(const DataDescriptorPtr& descriptor);
    void handleValueDescriptorChange(const DataDescriptorPtr& descriptor);
    size_t getNumberOfEventPacketsInQueue();
    bool dropUntilEvent();

private:
    std::deque<PacketPtr> packets;
    std::deque<SignalEvent> events;

    InputPortConfigPtr port;
    ConnectionPtr connection;
};

END_NAMESPACE_OPENDAQ