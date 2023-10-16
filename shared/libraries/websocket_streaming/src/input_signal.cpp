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
    assert(!currentDataDescriptor.isStructDescriptor());

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

PacketPtr InputSignal::createDecriptorChangedPacket()
{
    return DataDescriptorChangedEventPacket(currentDataDescriptor, currentDomainDataDescriptor);
}

void InputSignal::setDataDescriptor(const daq::streaming_protocol::SubscribedSignal &dataSignal)
{
    auto sInfo = SignalDescriptorConverter::ToDataDescriptor(dataSignal);
    auto descriptor = sInfo.dataDescriptor;
    currentDataDescriptor = descriptor;
}

void InputSignal::setDomainDescriptor(const daq::streaming_protocol::SubscribedSignal& timeSignal)
{
    auto sInfo = SignalDescriptorConverter::ToDataDescriptor(timeSignal);
    auto descriptor = sInfo.dataDescriptor;
    currentDomainDataDescriptor = descriptor;
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

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
