#include <websocket_streaming/input_signal.h>
#include <streaming_protocol/SubscribedSignal.hpp>
#include <websocket_streaming/signal_descriptor_converter.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/sample_type_traits.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

using namespace daq;
using namespace daq::streaming_protocol;

InputSignal::InputSignal()
{
}

PacketPtr InputSignal::asPacket(uint64_t packetOffset, const uint8_t* data, size_t size)
{
    auto sampleType = currentDataDescriptor.getSampleType();
    if (currentDataDescriptor.getPostScaling().assigned())
        sampleType = currentDataDescriptor.getPostScaling().getInputSampleType();

    const auto sampleSize = getSampleSize(sampleType);
    const auto sampleCount = size / sampleSize;

    auto domainPacket = DataPacket(currentDomainDataDescriptor, sampleCount, (Int) packetOffset);
    auto dataPacket = DataPacketWithDomain(domainPacket, currentDataDescriptor, sampleCount);
    std::memcpy(dataPacket.getRawData(), data, sampleCount*sampleSize);
    return dataPacket;
}

EventPacketPtr InputSignal::createDecriptorChangedPacket()
{
    return DataDescriptorChangedEventPacket(currentDataDescriptor, currentDomainDataDescriptor);
}

void InputSignal::setDataDescriptor(const DataDescriptorPtr& dataDescriptor)
{
    currentDataDescriptor = dataDescriptor;
}

void InputSignal::setDomainDescriptor(const DataDescriptorPtr& domainDescriptor)
{
    currentDomainDataDescriptor = domainDescriptor;
}

bool InputSignal::hasDescriptors()
{
    return currentDataDescriptor.assigned() && currentDomainDataDescriptor.assigned();
}

DataDescriptorPtr InputSignal::getSignalDescriptor()
{
    return currentDataDescriptor;
}

DataDescriptorPtr InputSignal::getDomainSignalDescriptor()
{
    return currentDomainDataDescriptor;
}

void InputSignal::setTableId(std::string id)
{
    tableId = id;
}

std::string InputSignal::getTableId()
{
    return tableId;
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
