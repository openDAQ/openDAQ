#include "websocket_streaming/output_signal.h"
#include "websocket_streaming/signal_descriptor_converter.h"
#include <opendaq/packet_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include "streaming_protocol/StreamWriter.h"
#include "streaming_protocol/SynchronousSignal.hpp"
#include <opendaq/event_packet_params.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/signal_factory.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

using namespace daq::streaming_protocol;
using namespace daq::stream;

OutputSignal::OutputSignal(const StreamPtr& stream, const SignalPtr& signal,
                           daq::streaming_protocol::LogCallback logCb)
    : OutputSignal(std::make_shared<StreamWriter>(stream), signal, logCb)
{
}

OutputSignal::OutputSignal(const daq::streaming_protocol::StreamWriterPtr& writer, const SignalPtr& signal,
                           daq::streaming_protocol::LogCallback logCb)
    : signal(signal)
    , writer(writer)
    , logCallback(logCb)
{
    createSignalStream();
    createStreamedSignal();
}

void OutputSignal::write(const PacketPtr& packet)
{
    const auto type = packet.getType();

    switch (type)
    {
        case PacketType::Data:
            writeDataPacket(packet);
            break;
        case PacketType::Event:
            writeEventPacket(packet);
            break;
        default:
            STREAMING_PROTOCOL_LOG_E("Failed to write a packet of unsupported type.");
    }
}

void OutputSignal::write(const void* data, size_t sampleCount)
{
    stream->addData(data, sampleCount);
}

DataDescriptorPtr OutputSignal::getValueDescriptor()
{
    auto dataDescriptor = signal.getDescriptor();
    if (!dataDescriptor.assigned())
        throw InvalidParameterException("Signal descriptor not set.");

    if (dataDescriptor.isStructDescriptor())
        throw InvalidParameterException("Signal cannot be a struct.");

    return dataDescriptor;
}

DataDescriptorPtr OutputSignal::getDomainDescriptor()
{
    auto domainSignal = signal.getDomainSignal();
    if (!domainSignal.assigned())
        throw InvalidParameterException("Domain signal not set.");

    auto domainDataDescriptor = domainSignal.getDescriptor();
    if (!domainDataDescriptor.assigned())
        throw InvalidParameterException("Domain signal descriptor not set.");

    if (domainDataDescriptor.isStructDescriptor())
        throw InvalidParameterException("Signal cannot be a struct.");

    return domainDataDescriptor;
}

uint64_t OutputSignal::getRuleDelta()
{
    auto valueDescriptor = getDomainDescriptor();

    auto dataRule = valueDescriptor.getRule();
    if (dataRule.getType() != DataRuleType::Linear)
        throw InvalidParameterException("Invalid data rule.");

    return dataRule.getParameters().get("delta");
}

uint64_t OutputSignal::getTickResolution()
{
    auto valueDescriptor = getDomainDescriptor();
    auto resolution = valueDescriptor.getTickResolution();
    return resolution.getDenominator() / resolution.getNumerator();
}

void OutputSignal::createSignalStream()
{
    const auto valueDescriptor = getValueDescriptor();
    auto sampleType = valueDescriptor.getSampleType();
    if (valueDescriptor.getPostScaling().assigned())
        sampleType = valueDescriptor.getPostScaling().getInputSampleType();
    const auto id = signal.getGlobalId();
    const auto outputRate = getRuleDelta(); // from streaming library side, output rate is defined as number of tics between two samples
    const auto resolution = getTickResolution();
    sampleSize = getSampleSize(sampleType);

    switch (sampleType)
    {
        case daq::SampleType::Int8:
            stream = std::make_shared<SynchronousSignal<int8_t>>(id, outputRate, resolution, *writer, logCallback);
            break;
        case daq::SampleType::UInt8:
            stream = std::make_shared<SynchronousSignal<uint8_t>>(id, outputRate, resolution, *writer, logCallback);
            break;
        case daq::SampleType::Int16:
            stream = std::make_shared<SynchronousSignal<int16_t>>(id, outputRate, resolution, *writer, logCallback);
            break;
        case daq::SampleType::UInt16:
            stream = std::make_shared<SynchronousSignal<uint16_t>>(id, outputRate, resolution, *writer, logCallback);
            break;
        case daq::SampleType::Int32:
            stream = std::make_shared<SynchronousSignal<int32_t>>(id, outputRate, resolution, *writer, logCallback);
            break;
        case daq::SampleType::UInt32:
            stream = std::make_shared<SynchronousSignal<uint32_t>>(id, outputRate, resolution, *writer, logCallback);
            break;
        case daq::SampleType::Int64:
            stream = std::make_shared<SynchronousSignal<int64_t>>(id, outputRate, resolution, *writer, logCallback);
            break;
        case daq::SampleType::UInt64:
            stream = std::make_shared<SynchronousSignal<uint64_t>>(id, outputRate, resolution, *writer, logCallback);
            break;
        case daq::SampleType::Float32:
            stream = std::make_shared<SynchronousSignal<float>>(id, outputRate, resolution, *writer, logCallback);
            break;
        case daq::SampleType::Float64:
            stream = std::make_shared<SynchronousSignal<double>>(id, outputRate, resolution, *writer, logCallback);
            break;
        case daq::SampleType::ComplexFloat32:
        case daq::SampleType::ComplexFloat64:
        case daq::SampleType::Binary:
        case daq::SampleType::Invalid:
        case daq::SampleType::String:
        case daq::SampleType::RangeInt64:
        default:
            throw InvalidTypeException();
    }

    SignalProps sigProps;
    sigProps.name = signal.getName();
    sigProps.description = signal.getDescription();
    SignalDescriptorConverter::ToStreamedSignal(signal, stream, sigProps);
    stream->subscribe();
}

void OutputSignal::createStreamedSignal()
{
    const auto context = signal.getContext();

    auto domainSignal = Signal(context, nullptr, "domain");
    streamedSignal = Signal(context, nullptr, signal.getLocalId());
    streamedSignal.setDomainSignal(domainSignal);
}

void OutputSignal::writeEventPacket(const EventPacketPtr& packet)
{
    const auto eventId = packet.getEventId();

    if (eventId == event_packet_id::DATA_DESCRIPTOR_CHANGED)
        writeDescriptorChangedPacket(packet);
    else if (eventId == event_packet_id::PROPERTY_CHANGED)
        writePropertyChangedPacket(packet);
    else
    {
        STREAMING_PROTOCOL_LOG_E("Event type {} is not supported by streaming.", eventId);
    }
}

void OutputSignal::writeDataPacket(const DataPacketPtr& packet)
{
    const auto domainPacket = packet.getDomainPacket();
    if (domainPacket.assigned())
        stream->setTimeStart(domainPacket.getOffset());

    stream->addData(packet.getRawData(), packet.getSampleCount());
}

void OutputSignal::writeDescriptorChangedPacket(const EventPacketPtr& packet)
{
    const auto params = packet.getParameters();
    const auto valueDescriptor = params.get(event_packet_param::DATA_DESCRIPTOR);
    const auto domainDescriptor = params.get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

    if (valueDescriptor.assigned())
        streamedSignal.setDescriptor(valueDescriptor);
    SignalConfigPtr domainSignal = streamedSignal.getDomainSignal();
    if (domainSignal.assigned() && domainDescriptor.assigned())
        domainSignal.setDescriptor(domainDescriptor);

    SignalDescriptorConverter::ToStreamedSignal(streamedSignal, stream, SignalProps{});
    stream->writeSignalMetaInformation();
}

void OutputSignal::writePropertyChangedPacket(const EventPacketPtr& packet)
{
    const auto params = packet.getParameters();
    const auto name = params.get(event_packet_param::NAME);
    const auto value = params.get(event_packet_param::VALUE);

    SignalProps sigProps;
    if (name == "Name")
        sigProps.name = value;
    else if (name == "Description")
        sigProps.description = value;

    SignalDescriptorConverter::ToStreamedSignal(signal, stream, sigProps);
    stream->writeSignalMetaInformation();
}

SignalPtr OutputSignal::getCoreSignal()
{
    return signal;
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
