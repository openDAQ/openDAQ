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

    static SignalEvent gapEvent(Int gapDiff);

    bool merge(const SignalEvent& otherEvent);
    SignalEventType getType() const;
    const DataDescriptorPtr& getDomainDescriptor() const;
    const DataDescriptorPtr& getValueDescriptor() const;

    EventPacketPtr toEventPacket() const;

private:
    explicit SignalEvent(Int gapDiff);

    void updateType();
private:
    SignalEventType eventType;
    DataDescriptorPtr domainDescriptor;
    DataDescriptorPtr valueDescriptor;
    Int gapDiff;
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
    UnsupportedDomainRule           = 1 << 2,
    OriginParsingFailed             = 1 << 3
};

class QueueReader
{
public:
    explicit QueueReader(const InputPortConfigPtr& port, // Consider using Connection instead
                 SampleType valueReadType,
                 SampleType domainReadType,
                 ReadMode mode,
                 const LoggerComponentPtr& logger,
                 bool globalIdFromSignal);

public:
    DomainInfo getDomainInfo();
    std::unique_ptr<DomainValue> getFirstSampleDomainValue();
    AdvanceResult advanceToDomainValue(const DomainValue* domainValue);
    Int getSampleRate();

    void dropOutdatedPacketSegments();
    
    /**
     * @brief Get the Available Samples in common rate equivalent
     * 
     * @return SizeT Available samples multiplied by the sample rate divider.
     */
    SizeT getAvailableSamples();
    
    bool hasPendingEvents();
    EventPacketPtr popFrontEvent();
    
    bool isValid();
    
    void domainChangeHandled();
    void updateConnection();
    
    void setSampleRateDivider(SizeT divider);
    SizeT getSampleRateDivider() const;

    /**
     * @brief Read common rate equivalent samples into the buffer. There will be nativeSamples = count / sampleRateDivider
     * samples read from the packets into the buffer.
     * 
     * @param buffer Buffer that has capacity of at least count / sampleRateDivider
     * @param count Desired sample count in common rate equivalent. 
     * @return AdvanceResult 
     */
    AdvanceResult read(void* valueBuffer, void* domainBuffer, SizeT* count);
    AdvanceResult skip(SizeT* count);

    /**
     * @brief Drop the current data segment, if there are fewer than samplesInBlock samples available (common rate equivalent).
     * 
     * For example: If there are 3 samples available before next event, samplesInBlock=10 (this is dividerLCM in terms of multireading)
     * and divider for the queue reader is 2, then 5 native samples are required as minimum aligned read. Since 3 < 5, we drop the 3 samples
     * and create a synchronization GAP event in the event queue.
     * 
     * @param samplesInBlock Number of samples (common rate equivalent).
     * @return true If samples were dropped.
     * @return false If samples were not dropped - data segment is long enough or there is no event in the queue to end the segment.
     */
    bool dropLeftoverSegment(SizeT samplesInBlock);
    
private:
    void drainConnection();
    void adoptPackets();
    void consumeLeadingEventPackets();
    
    SizeT getAvailableSamplesNative();
    AdvanceResult readNative(void* valueBuffer, void* domainBuffer, SizeT* count);

    void checkConnection() const;
    
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

    SizeT sampleRateDivider = 1;
};

END_NAMESPACE_OPENDAQ