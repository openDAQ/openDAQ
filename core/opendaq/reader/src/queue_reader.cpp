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
                 bool globalIdFromSignal) // TODO
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

void QueueReader::adoptPackets()
{
    // Take ownership of all packets
    PacketPtr packet = connection.dequeue();
    while (packet.assigned())
    {
        packets.push_back(std::move(packet));
        packet = connection.dequeue();
    }
}

DomainInfo QueueReader::getDomainInfo()
{
    checkConnection();

    drainConnection();
    return typeCtx.domainInfo;
}

std::unique_ptr<DomainValue> QueueReader::getFirstSampleDomainValue()
{
    checkConnection();
    drainConnection();

    if (packets.empty() || packets.front().getType() != PacketType::Data)
    {
        return nullptr;
    }

    DataPacketPtr domainPacket = packets.front().asPtr<IDataPacket>(true).getDomainPacket();
    if (!domainPacket.assigned())
    {
        DAQ_THROW_EXCEPTION(InvalidStateException, "Packet must have a domain packet assigned!");
    }

    return TypedReadingUtils::readDomainValue(typeCtx.domainIn,
                                              typeCtx.domainOut,
                                              typeCtx.domainLayout,
                                              domainPacket,
                                              readingPosition,
                                              typeCtx.domainInfo);
}

AdvanceResult QueueReader::advanceToDomainValue(const DomainValue* domainValue)
{
    // TODO: Add first timestamp mechanism for sync tolerance checking
    checkConnection();
    drainConnection();

    SignalEventType signalChange = SignalEventType::NoChange;

    bool found = false;
    SizeT end = 0;
    for (auto& packet: packets)
    {
        if (packet.getType() == PacketType::Data)
        {
            DataPacketPtr domainPacket = packet.asPtr<IDataPacket>(true).getDomainPacket();

            SizeT index = TypedReadingUtils::findDomainValue(typeCtx.domainIn,
                                                             typeCtx.domainOut,
                                                             typeCtx.domainLayout,
                                                             domainPacket,
                                                             domainValue,
                                                             nullptr /*TODO*/);
            
            if (index != static_cast<SizeT>(-1))
            {
                if (index < readingPosition)
                {
                    return AdvanceResult::OvershotError;
                }
                readingPosition = index;
                found = true;
                break;
            }

            readingPosition = 0;
            ++end;
            continue;
        }
        else if (packet.getType() == PacketType::Event)
        {
            auto eventPacket = packet.asPtr<IEventPacket>(true);
            signalChange = addEncounteredEvent(eventPacket);
            ++end;

            if (signalChange == SignalEventType::DomainChanged ||
                signalChange == SignalEventType::DomainAndValueChanged ||
                signalChange == SignalEventType::Gap)
            {
                break;
            }
            continue;
        }
        else
        {
            // Unexpected packet type encountered.
            // Packet should be removed and sync is not successful.
            signalChange = SignalEventType::DomainChanged;
            ++end;
            break;
        }
    }

    packets.erase(packets.begin(), packets.begin() + end);

    switch (signalChange)
    {
    case SignalEventType::DomainChanged:
    case SignalEventType::DomainAndValueChanged:
    case SignalEventType::Gap:
        return AdvanceResult::DomainChanged;
    default:
        break;
    }

    return found ? AdvanceResult::Success : AdvanceResult::NeedMoreData;
}

Int QueueReader::getSampleRate()
{
    checkConnection();
    drainConnection();

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

void QueueReader::checkConnection() const
{
    if (!connection.assigned())
        DAQ_THROW_EXCEPTION(InvalidOperationException, "Connection must be assigned for this operation.");
}

void QueueReader::dropOutdatedPacketSegments()
{
    checkConnection();
    drainConnection();

    while (getNumberOfEventPacketsInQueue() >= 2)
    {
        auto foundEvent = dropUntilEvent();
        assert(foundEvent && "Event should have been found.");
        consumeLeadingEventPackets();
    }
    dropUntilEvent();
    consumeLeadingEventPackets();
}

SizeT QueueReader::getAvailableSamples()
{
    checkConnection();
    drainConnection();

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

bool QueueReader::hasPendingEvents()
{
    checkConnection();
    drainConnection();
    return !events.empty();
}

EventPacketPtr QueueReader::popFrontEvent()
{
    checkConnection();
    drainConnection();
    if (events.empty())
        return nullptr;
    
    auto eventPacket = events.front().toEventPacket();
    events.pop_front();
    return eventPacket;
}

bool QueueReader::isValid()
{
    if (!connection.assigned())
        return false;
    
    drainConnection();
    return issues.empty();
}

void QueueReader::domainChangeHandled()
{
    domainChanged = false;
}

void QueueReader::updateConnection()
{
    connection = port.getConnection();
    drainConnection();
}

void QueueReader::drainConnection()
{
    if (!connection.assigned())
        return;

    if (!connection.peek().assigned())
        return;

    adoptPackets();
    consumeLeadingEventPackets();
}

SignalEventType QueueReader::addEncounteredEvent(const EventPacketPtr& packet)
{
    auto event = SignalEvent(packet);
    auto eventType = event.getType();

    switch (eventType)
    {
        case SignalEventType::DomainChanged:
            typeCtx.domainLayout.descriptor = event.getDomainDescriptor();
            break;
        case SignalEventType::ValueChanged:
            typeCtx.valueLayout.descriptor = event.getValueDescriptor();
            break;
        case SignalEventType::DomainAndValueChanged:
            typeCtx.domainLayout.descriptor = event.getDomainDescriptor();
            typeCtx.valueLayout.descriptor = event.getValueDescriptor();
            break;
        default:
            break;
    }
    parseCachedDescriptors();

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

void QueueReader::parseDomainDescriptor()
{
    auto& descriptor = typeCtx.domainLayout.descriptor;
    if (!descriptor.assigned())
        return;

    // Type conversion
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

    typeCtx.domainInfo = DomainInfo::fromDescriptor(descriptor);

    bool domainTypesConvertible = TypedReadingUtils::isSampleTypeConvertible(typeCtx.domainIn, typeCtx.domainOut, true);
    issues.set(QueueReaderIssue::DomainTypesNotConvertible, !domainTypesConvertible);
    // END Type Conversion

    // Resolution and origin
    auto newResolution = descriptor.getTickResolution();
    if (typeCtx.domainInfo.resolution != newResolution)
    {
        typeCtx.domainInfo.resolution = newResolution;
        domainChanged = true;
    }

    std::string origin = descriptor.getOrigin();
    auto newOrigin = reader::tryParseEpoch(origin);
    if (newOrigin.has_value() && typeCtx.domainInfo.epoch != newOrigin.value())
    {
        typeCtx.domainInfo.epoch = newOrigin.value();
        domainChanged = true;
    }
    issues.set(QueueReaderIssue::OriginParsingFailed, !newOrigin.has_value());
    // END Resolution and origin

    // Sample rate and delta
    {
        std::int64_t newSampleRate = 0;

        NumberPtr delta = 1;
        auto rule = descriptor.getRule();
        const bool ruleIsLinear = rule.assigned() && rule.getType() == DataRuleType::Linear;
    
        if (ruleIsLinear)
        {
            delta = rule.getParameters()["delta"];
        }

        double sr = static_cast<double>(typeCtx.domainInfo.resolution.getDenominator()) /
                            (static_cast<double>(typeCtx.domainInfo.resolution.getNumerator()) *
                            delta.getFloatValue());
        
        const bool deltaIsInteger = (delta.getFloatValue() == static_cast<double>(delta.getIntValue()));
        const bool sampleRateIsInteger = (sr == static_cast<double>(static_cast<std::int64_t>(sr)));

        newSampleRate = static_cast<std::int64_t>(sr);
        
        if (sampleRate != newSampleRate)
        {
            sampleRate = newSampleRate;
            domainChanged = true;
        }

        if (packetDelta != delta.getIntValue())
        {
            packetDelta = delta.getIntValue();
            domainChanged = true;
        }

        issues.set(QueueReaderIssue::UnsupportedDomainRule, !ruleIsLinear || !deltaIsInteger || !sampleRateIsInteger);
    }
    // END Sample rate and delta
}

void QueueReader::parseValueDescriptor()
{
    auto& descriptor = typeCtx.valueLayout.descriptor;

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
    }

    if (typeCtx.valueOut == SampleType::Undefined) // Dynamically determine output type
    {
        
        typeCtx.valueOut = typeCtx.valueIn;
    }

    bool valueTypesConvertible = TypedReadingUtils::isSampleTypeConvertible(typeCtx.valueIn, typeCtx.valueOut, false);
    issues.set(QueueReaderIssue::ValueTypesNotConvertible, !valueTypesConvertible);
}

void QueueReader::parseCachedDescriptors()
{
    parseDomainDescriptor();
    parseValueDescriptor();
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