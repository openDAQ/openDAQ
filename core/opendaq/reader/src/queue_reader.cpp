#include <opendaq/queue_reader.h>
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

DataDescriptorPtr SignalEvent::getDomainDescriptor() const
{
    return domainDescriptor;
}

DataDescriptorPtr SignalEvent::getValueDescriptor() const
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
{
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
    for (const auto& packet : packets)
    {
        if (packet.getType() != PacketType::Data)
            break;

        DataPacketPtr dataPacket = packet.asPtr<IDataPacket>(true);
        count += dataPacket.getSampleCount();
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

    bool addToList = true;
    if (!events.empty())
    {
        // Attempt merging with the last event
        addToList = !events.back().merge(event);
    }
    if (addToList)
        events.push_back(event);
    
    return eventType;
}

void QueueReader::handleDomainDescriptorChange(const DataDescriptorPtr& descriptor)
{
    // Parse into domain info
}

void QueueReader::handleValueDescriptorChange(const DataDescriptorPtr& descriptor)
{
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