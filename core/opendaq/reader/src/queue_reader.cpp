#include <opendaq/queue_reader.h>

#include <opendaq/custom_log.h>
#include <opendaq/event_packet_utils.h>

BEGIN_NAMESPACE_OPENDAQ

SignalEvent::SignalEvent(const EventPacketPtr& packet)
    : eventType(SignalEventType::NoChange)
    , domainDescriptor(nullptr)
    , valueDescriptor(nullptr)
{
    if (packet.getEventId() == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED){
        eventType = SignalEventType::Gap;
    }
    else
    {
        const auto [valueDescChanged, domainDescChanged, newValueDescriptor, newDomainDescriptor] =
                        parseDataDescriptorEventPacket(packet);
        domainDescriptor = newDomainDescriptor;
        valueDescriptor = newValueDescriptor;
        updateType();
    }
}

void SignalEvent::updateType()
{
    if (eventType == SignalEventType::Gap)
        return;
    
    if (domainDescriptor.assigned() and valueDescriptor.assigned())
    {
        eventType = SignalEventType::DomainAndValueChanged;
    }
    else if (domainDescriptor.assigned())
    {
        eventType = SignalEventType::DomainChanged;
    }
    else if (valueDescriptor.assigned())
    {
        eventType = SignalEventType::ValueChanged;
    }
    else
    {
        eventType = SignalEventType::NoChange;
    }

}

bool SignalEvent::merge(const SignalEvent& other)
{
    if (this->eventType != SignalEventType::Gap && other.eventType == SignalEventType::Gap)
        return false;
    if (this->eventType == SignalEventType::Gap && other.eventType != SignalEventType::Gap)
        return false;

    if (other.domainDescriptor.assigned())
        domainDescriptor = other.domainDescriptor;
    if (other.valueDescriptor.assigned())
        valueDescriptor = other.valueDescriptor;
    updateType();
    return true;
}

SignalEventType SignalEvent::getType() const
{
    return eventType;
}

const DataDescriptorPtr& SignalEvent::getDomainDescriptor() const
{
    return domainDescriptor;
}

const DataDescriptorPtr& SignalEvent::getValueDescriptor() const
{
    return valueDescriptor;
}

EventPacketPtr SignalEvent::toEventPacket() const
{
    return DataDescriptorChangedEventPacket(
        descriptorToEventPacketParam(valueDescriptor),
        descriptorToEventPacketParam(domainDescriptor));
}

QueueReader::QueueReader(const InputPortConfigPtr& port, // Consider using Connection instead
                 SampleType valueReadType,
                 SampleType domainReadType,
                 ReadMode mode,
                 const LoggerComponentPtr& logger,
                 bool globalIdFromSignal)
    : port(port)
    , connection(port.getConnection())
    , readMode(mode)
    , loggerComponent(logger)
{
    typeCtx.domainIn = SampleType::Undefined;
    typeCtx.domainOut = domainReadType;
    typeCtx.valueIn = SampleType::Undefined;
    typeCtx.valueOut = mode == ReadMode::RawValue ? SampleType::Undefined : valueReadType;
}

void QueueReader::packetReceived()
{
    // Take ownership of all packets
    PacketPtr packet = connection.dequeue();
    while (packet.assigned())
    {
        packets.push_back(std::move(packet));
        packet = connection.dequeue();
    }

    // Consider if the assumption that queue always starts with data simplifies anything
    consumeLeadingEventPackets();
}

DomainInfo QueueReader::getDomainInfo() const
{
    return typeCtx.domainInfo;
}

Int QueueReader::getSampleRate() const
{
    return sampleRate;
}

void QueueReader::consumeLeadingEventPackets()
{
    size_t end = 0;
    for (const auto& packet : packets){
        auto packetType = packet.getType();
        if (packetType == PacketType::Data){
            break;
        }

        EventPacketPtr eventPacket = packet.asPtr<IEventPacket>(true);
        addEncounteredEvent(eventPacket);

        ++end;
    }
    packets.erase(packets.begin(), packets.begin() + end);
}

void QueueReader::dropOutdatedDomainSegments()
{
    while (getNumberOfEventPacketsInQueue() >= 2)
    {
        auto foundEvent = dropUntilEvent();
        assert(foundEvent && "Event should have been found.");
        consumeLeadingEventPackets();
    }
    dropUntilEvent();
}

SizeT QueueReader::getAvailableSamples() const
{
    SizeT count = 0;
    SizeT packetReadingPosition = readingPosition;
    for (const auto& packet : packets)
    {
        if (packet.getType() != PacketType::Data)
            break;

        DataPacketPtr dataPacket = packet.asPtr<IDataPacket>(true);
        count += dataPacket.getSampleCount() - packetReadingPosition;

        // Only first packet may have non-zero reading position
        packetReadingPosition = 0;
    }
    return count;
}

bool QueueReader::hasPendingEvents() const
{
    return !events.empty();
}

EventPacketPtr QueueReader::popFrontEvent()
{
    if (events.empty())
        return nullptr;
    
    auto eventPacket = events.front().toEventPacket();
    events.pop_front();
    return eventPacket;
}

bool QueueReader::isValid() const
{
    return issues.empty();
}

SignalEventType QueueReader::addEncounteredEvent(const EventPacketPtr& packet)
{
    auto event = SignalEvent(packet);
    auto eventType = event.getType();

    switch (eventType)
    {
        case SignalEventType::DomainChanged:
            handleDomainDescriptorChange(event.getDomainDescriptor());
            break;
        case SignalEventType::ValueChanged:
            handleValueDescriptorChange(event.getValueDescriptor());
            break;
        case SignalEventType::DomainAndValueChanged:
            handleDomainDescriptorChange(event.getDomainDescriptor());
            handleValueDescriptorChange(event.getValueDescriptor());
            break;
        default:
            break;
    }

    // TODO: Check for invalid state

    bool addToList = true;
    if (!events.empty())
    {
        // Attempt merging with the last event and add to list if merge not possible
        addToList = !events.back().merge(event);
    }
    if (addToList)
        events.push_back(event);
    
    return eventType;
}

void QueueReader::handleDomainDescriptorChange(const DataDescriptorPtr& descriptor)
{
    const auto postScaling = descriptor.getPostScaling();
    if (!postScaling.assigned() || readMode == ReadMode::Scaled)
    {
        typeCtx.domainIn = descriptor.getSampleType();
    }
    else
    {
        typeCtx.domainIn = postScaling.getInputSampleType();
    }

    typeCtx.domainLayout.rawSampleSize = descriptor.getRawSampleSize();
    auto dimensions = descriptor.getDimensions();
    if (dimensions.assigned() && dimensions.getCount() == 1)
    {
        typeCtx.domainLayout.valuesPerSample = dimensions[0].getSize();
    }

    typeCtx.domainLayout.descriptor = descriptor;

    typeCtx.domainInfo = DomainInfo::fromDescriptor(descriptor);

    bool domainTypesConvertible = TypedReadingUtils::isSampleTypeConvertible(typeCtx.domainIn, typeCtx.domainOut, true);
    issues.set(QueueReaderIssue::DomainTypesNotConvertible, !domainTypesConvertible);

    auto newResolution = descriptor.getTickResolution();
    if (typeCtx.domainInfo.resolution != newResolution)
    {
        typeCtx.domainInfo.resolution = newResolution;
        // TODO: Unsync state
    }

    std::string origin = descriptor.getOrigin();
    auto newOrigin = reader::parseEpoch(origin);
    if (typeCtx.domainInfo.epoch != newOrigin)
    {
        typeCtx.domainInfo.epoch = newOrigin;
        // TODO: Unsync state
    }

    std::int64_t newSampleRate = 0;
    try
    {
        newSampleRate = reader::getSampleRate(descriptor);
        issues.set(QueueReaderIssue::UnsupportedDomainRule, false);
    }
    catch (const std::exception& e)
    {
        issues.set(QueueReaderIssue::UnsupportedDomainRule, true);
        LOG_D("Failed to change descriptor: {}", e.what());
        (void) e;
        return;
    }

    // TODO: Remove this. Change in sampling rate should just invalid the synchronization state/progress of the multireader.
    if (sampleRate != -1 && sampleRate != newSampleRate)
    {
        issues.set(QueueReaderIssue::SampleRateChanged, true);
    }

    if (sampleRate != newSampleRate)
    {
        sampleRate = newSampleRate;
    }

    packetDelta = 0;
    const auto domainRule = descriptor.getRule();
    if (domainRule.getType() == DataRuleType::Linear)
    {
        const auto domainRuleParams = domainRule.getParameters();
        packetDelta = domainRuleParams.get("delta");
    }
}

void QueueReader::handleValueDescriptorChange(const DataDescriptorPtr& descriptor)
{
    auto postScaling = descriptor.getPostScaling();
    if (!postScaling.assigned() || readMode == ReadMode::Scaled)
    {
        typeCtx.valueIn = descriptor.getSampleType();
    }
    else
    {
        typeCtx.valueIn = postScaling.getInputSampleType();
    }

    {
        typeCtx.valueLayout.rawSampleSize = descriptor.getRawSampleSize();
        auto dimensions = descriptor.getDimensions();
        if (dimensions.assigned() && dimensions.getCount() == 1)
        {
            typeCtx.valueLayout.valuesPerSample = dimensions[0].getSize();
        }
    
        typeCtx.valueLayout.descriptor = descriptor;
    }

    if (typeCtx.valueOut == SampleType::Undefined) // Dynamically determine output type
    {
        
        typeCtx.valueOut = typeCtx.valueIn;
    }

    bool valueTypesConvertible = TypedReadingUtils::isSampleTypeConvertible(typeCtx.valueIn, typeCtx.valueOut, false);
    issues.set(QueueReaderIssue::ValueTypesNotConvertible, !valueTypesConvertible);
}

size_t QueueReader::getNumberOfEventPacketsInQueue()
{
    size_t numberOfEventPackets = 0;
    for (const auto& packet: packets)
    {
        if (packet.getType() == PacketType::Event)
            ++numberOfEventPackets;
    }
    return numberOfEventPackets;
}

bool QueueReader::dropUntilEvent()
{
    // Queue: d1 d2 E d3 -> E d3
    bool foundEvent = false;
    size_t end = 0;
    for (const auto& packet : packets)
    {
        if (packet.getType() == PacketType::Event)
        {
            foundEvent = true;
            break;
        }
        ++end;
    }
    if (foundEvent)
        packets.erase(packets.begin(), packets.begin() + end);
    return foundEvent;
}

END_NAMESPACE_OPENDAQ