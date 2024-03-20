#include "websocket_streaming/output_signal.h"
#include <opendaq/data_descriptor_ptr.h>
#include "streaming_protocol/StreamWriter.h"
#include "streaming_protocol/SynchronousSignal.hpp"
#include "streaming_protocol/LinearTimeSignal.hpp"
#include <opendaq/event_packet_params.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/signal_factory.h>
#include "websocket_streaming/signal_descriptor_converter.h"

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

using namespace daq::streaming_protocol;
using namespace daq::stream;

OutputSignalBase::OutputSignalBase(const SignalPtr& signal,
                                   const DataDescriptorPtr& domainDescriptor,
                                   BaseSignalPtr stream,
                                   daq::streaming_protocol::LogCallback logCb)
    : daqSignal(signal)
    , logCallback(logCb)
    , subscribed(false)
    , stream(stream)
    , domainDescriptor(domainDescriptor)
{
    createStreamedSignal();
    subscribeToCoreEvent();
}

OutputSignalBase::~OutputSignalBase()
{
    unsubscribeFromCoreEvent();
}

void OutputSignalBase::createStreamedSignal()
{
    const auto context = daqSignal.getContext();

    streamedDaqSignal = SignalWithDescriptor(context, daqSignal.getDescriptor(), nullptr, daqSignal.getLocalId());

    streamedDaqSignal.setName(daqSignal.getName());
    streamedDaqSignal.setDescription(daqSignal.getDescription());
}

void OutputSignalBase::subscribeToCoreEvent()
{
    daqSignal.getOnComponentCoreEvent() += event(this, &OutputSignalBase::processAttributeChangedCoreEvent);
}

void OutputSignalBase::unsubscribeFromCoreEvent()
{
    daqSignal.getOnComponentCoreEvent() -= event(this, &OutputSignalBase::processAttributeChangedCoreEvent);
}

void OutputSignalBase::processAttributeChangedCoreEvent(ComponentPtr& /*component*/, CoreEventArgsPtr& args)
{
    if (args.getEventId() != static_cast<int>(CoreEventId::AttributeChanged))
        return;

    const auto params = args.getParameters();
    const auto name = params.get("AttributeName");
    const auto value = params.get(name);

    SignalProps sigProps;
    if (name == "Name")
    {
        sigProps.name = value;
        streamedDaqSignal.setName(value);
    }
    else if (name == "Description")
    {
        sigProps.description = value;
        streamedDaqSignal.setDescription(value);
    }
    else
    {
        return;
    }

    // Streaming LT does not support attribute change forwarding for active, public, and visible
    toStreamedSignal(daqSignal, sigProps);
    submitSignalChanges();
}

SignalProps OutputSignalBase::getSignalProps(const SignalPtr& signal)
{
    SignalProps signalProps;

    signalProps.name = signal.getName();
    signalProps.description = signal.getDescription();

    return signalProps;
}

SignalPtr OutputSignalBase::getDaqSignal()
{
    return daqSignal;
}

bool OutputSignalBase::isSubscribed()
{
    std::scoped_lock lock(subscribedSync);

    return subscribed;
}

void OutputSignalBase::submitSignalChanges()
{
    stream->writeSignalMetaInformation();
}

void OutputSignalBase::writeDescriptorChangedEvent(const DataDescriptorPtr& descriptor)
{
    streamedDaqSignal.setDescriptor(descriptor);

    toStreamedSignal(streamedDaqSignal, getSignalProps(streamedDaqSignal));
    submitSignalChanges();
}

void OutputSignalBase::submitTimeConfigChange(const DataDescriptorPtr& domainDescriptor)
{
    // significant parameters of domain signal have been changed
    // reset values used for timestamp calculations

    this->domainDescriptor = domainDescriptor;
    doSetStartTime = true;
}

bool OutputSignalBase::isTimeConfigChanged(const DataDescriptorPtr& domainDescriptor)
{
    return this->domainDescriptor.getRule() != domainDescriptor.getRule() ||
           this->domainDescriptor.getTickResolution() != domainDescriptor.getTickResolution();
}

OutputValueSignalBase::OutputValueSignalBase(daq::streaming_protocol::BaseValueSignalPtr valueStream,
                                             const SignalPtr& signal,
                                             OutputDomainSignaBaselPtr outputDomainSignal,
                                             daq::streaming_protocol::LogCallback logCb)
    : OutputSignalBase(signal, outputDomainSignal->getDaqSignal().getDescriptor(), valueStream, logCb)
    , outputDomainSignal(outputDomainSignal)
    , valueStream(valueStream)
{
}

void OutputValueSignalBase::writeEventPacket(const EventPacketPtr& packet)
{
    const auto eventId = packet.getEventId();

    if (eventId == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        writeDescriptorChangedPacket(packet);
    }
    else
    {
        STREAMING_PROTOCOL_LOG_E("Event type {} is not supported by streaming.", eventId);
    }
}

void OutputValueSignalBase::writeDescriptorChangedPacket(const EventPacketPtr& packet)
{
    const auto params = packet.getParameters();
    const DataDescriptorPtr valueDescriptor = params.get(event_packet_param::DATA_DESCRIPTOR);
    const DataDescriptorPtr domainDescriptor = params.get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

    if (valueDescriptor.assigned())
    {
        this->writeDescriptorChangedEvent(valueDescriptor);
    }
    if (domainDescriptor.assigned())
    {
        if (outputDomainSignal->isTimeConfigChanged(domainDescriptor))
        {
            outputDomainSignal->submitTimeConfigChange(domainDescriptor);
        }

        if (isTimeConfigChanged(domainDescriptor))
        {
            submitTimeConfigChange(domainDescriptor);
        }
        outputDomainSignal->writeDescriptorChangedEvent(domainDescriptor);
    }
}

void OutputValueSignalBase::writeDaqPacket(const PacketPtr& packet)
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
            break;
    }
}

// bool subscribed; / void setSubscribed(bool); / bool isSubscribed() :
// To prevent client side streaming protocol error the server should not send data before
// the subscribe acknowledgment is sent neither after the unsubscribe acknowledgment is sent.
// The `subscribed` member variable functions as a flag that enables/disables data streaming
// within the OutputSignal.
// Mutex locking ensures that data is sent only when the specified conditions are satisfied.
void OutputValueSignalBase::setSubscribed(bool subscribed)
{
    if (this->subscribed != subscribed)
    {
        std::scoped_lock lock(subscribedSync);

        this->subscribed = subscribed;
        doSetStartTime = true;
        if (subscribed)
        {
            outputDomainSignal->subscribeByDataSignal();
            stream->subscribe();
        }
        else
        {
            stream->unsubscribe();
            outputDomainSignal->unsubscribeByDataSignal();
        }
    }
}

bool OutputValueSignalBase::isDataSignal()
{
    return true;
}

void OutputValueSignalBase::toStreamedSignal(const SignalPtr& signal, const SignalProps& sigProps)
{
    SignalDescriptorConverter::ToStreamedValueSignal(signal, valueStream, sigProps);
}

OutputDomainSignalBase::OutputDomainSignalBase(daq::streaming_protocol::BaseDomainSignalPtr domainStream,
                                       const SignalPtr& signal,
                                       daq::streaming_protocol::LogCallback logCb)
    : OutputSignalBase(signal, signal.getDescriptor(), domainStream, logCb)
    , domainStream(domainStream)
{
}

void OutputDomainSignalBase::writeDaqPacket(const PacketPtr& packet)
{
    throw InvalidOperationException("Streaming-lt: explicit streaming of domain signals is not supported");
}

uint64_t OutputDomainSignalBase::calcStartTimeOffset(uint64_t dataPacketTimeStamp)
{
    if (doSetStartTime)
    {
        STREAMING_PROTOCOL_LOG_I("time signal {}: reset start timestamp: {}", daqSignal.getGlobalId(), dataPacketTimeStamp);

        domainStream->setTimeStart(dataPacketTimeStamp);
        doSetStartTime = false;
        return 0;
    }
    else
    {
        auto signalStartTime = domainStream->getTimeStart();
        if (dataPacketTimeStamp < signalStartTime)
        {
            STREAMING_PROTOCOL_LOG_E(
                "Unable to calc start time index: domain signal start time {}, time stamp from packet {}",
                signalStartTime,
                dataPacketTimeStamp);
            return 0;
        }
        return (dataPacketTimeStamp - signalStartTime);
    }
}

// bool subscribed; / void setSubscribed(bool); / bool isSubscribed(); / (un)subscribeByDataSignal() :
// To prevent client side streaming protocol error the server should not send data before
// the subscribe acknowledgment is sent neither after the unsubscribe acknowledgment is sent.
// The `subscribed` member variable functions as a flag that enables/disables data streaming
// within the OutputSignal.
// Mutex locking ensures that data is sent only when the specified conditions are satisfied.
void OutputDomainSignalBase::subscribeByDataSignal()
{
    std::scoped_lock lock(subscribedSync);

    if (subscribedByDataSignalCount == 0)
    {
        doSetStartTime = true;
        if (!this->subscribed)
            stream->subscribe();
    }

    subscribedByDataSignalCount++;
}

void OutputDomainSignalBase::unsubscribeByDataSignal()
{
    std::scoped_lock lock(subscribedSync);

    if (subscribedByDataSignalCount == 0)
    {
        STREAMING_PROTOCOL_LOG_E("Cannot unsubscribe domain signal by data signal - already has 0 subscribers");
        return;
    }

    subscribedByDataSignalCount--;

    if (subscribedByDataSignalCount == 0)
    {
        if (!this->subscribed)
            stream->unsubscribe();
    }
}

void OutputDomainSignalBase::setSubscribed(bool subscribed)
{
    if (this->subscribed != subscribed)
    {
        std::scoped_lock lock(subscribedSync);

        this->subscribed = subscribed;
        if (subscribed)
        {
            if (subscribedByDataSignalCount == 0)
                stream->subscribe();
        }
        else
        {
            if (subscribedByDataSignalCount == 0)
                stream->unsubscribe();
        }
    }
}

bool OutputDomainSignalBase::isDataSignal()
{
    return false;
}

OutputLinearDomainSignal::OutputLinearDomainSignal(const daq::streaming_protocol::StreamWriterPtr& writer,
                                                   const SignalPtr& signal,
                                                   const std::string& tableId,
                                                   daq::streaming_protocol::LogCallback logCb)
    : OutputDomainSignalBase(createSignalStream(writer, signal, tableId, logCb), signal, logCb)
    , linearStream(std::dynamic_pointer_cast<daq::streaming_protocol::LinearTimeSignal>(stream))
{}

void OutputLinearDomainSignal::toStreamedSignal(const SignalPtr& signal, const SignalProps& sigProps)
{
    SignalDescriptorConverter::ToStreamedLinearSignal(signal, linearStream, sigProps);
}

LinearTimeSignalPtr OutputLinearDomainSignal::createSignalStream(
    const daq::streaming_protocol::StreamWriterPtr& writer,
    const SignalPtr& signal,
    const std::string& tableId,
    daq::streaming_protocol::LogCallback logCb)
{
    LinearTimeSignalPtr linearStream;

    const auto signalId = signal.getGlobalId();
    auto descriptor = signal.getDescriptor();

    // streaming-lt supports only 64bit domain values
    daq::SampleType daqSampleType = descriptor.getSampleType();
    if (daqSampleType != daq::SampleType::Int64 &&
        daqSampleType != daq::SampleType::UInt64)
        throw InvalidParameterException("Unsupported domain signal sample type");

    auto dataRule = descriptor.getRule();
    if (dataRule.getType() != DataRuleType::Linear)
        throw InvalidParameterException("Invalid domain signal data rule {}.", (size_t)dataRule.getType());

    auto unit = descriptor.getUnit();
    if (!unit.assigned() ||
        /*unit.getId() != streaming_protocol::Unit::UNIT_ID_SECONDS ||*/
        unit.getSymbol() != "s" ||
        unit.getQuantity() != "time")
    {
        throw InvalidParameterException(
            "Domain signal unit parameters: {}, does not match the predefined values for linear time signal",
            unit.assigned() ? unit.toString() : "not assigned");
    }

    // from streaming library side, output rate is defined as nanoseconds between two samples
    const auto outputRate = dataRule.getParameters().get("delta");
    const auto resolution =
        descriptor.getTickResolution().getDenominator() / descriptor.getTickResolution().getNumerator();

    auto outputRateInNs = BaseDomainSignal::nanosecondsFromTimeTicks(outputRate, resolution);
    linearStream = std::make_shared<LinearTimeSignal>(signalId, tableId, resolution, outputRateInNs, *writer, logCb);

    SignalDescriptorConverter::ToStreamedLinearSignal(signal, linearStream, getSignalProps(signal));

    return linearStream;
}

BaseSynchronousSignalPtr OutputSyncValueSignal::createSignalStream(
    const daq::streaming_protocol::StreamWriterPtr& writer,
    const SignalPtr& signal,
    const std::string& tableId,
    daq::streaming_protocol::LogCallback logCb)
{
    BaseSynchronousSignalPtr syncStream;

    const auto valueDescriptor = signal.getDescriptor();
    auto sampleType = valueDescriptor.getSampleType();
    if (valueDescriptor.getPostScaling().assigned())
        sampleType = valueDescriptor.getPostScaling().getInputSampleType();

    const auto signalId = signal.getGlobalId();

    switch (sampleType)
    {
        case daq::SampleType::Int8:
            syncStream = std::make_shared<SynchronousSignal<int8_t>>(signalId, tableId, *writer, logCb);
            break;
        case daq::SampleType::UInt8:
            syncStream = std::make_shared<SynchronousSignal<uint8_t>>(signalId, tableId, *writer, logCb);
            break;
        case daq::SampleType::Int16:
            syncStream = std::make_shared<SynchronousSignal<int16_t>>(signalId, tableId, *writer, logCb);
            break;
        case daq::SampleType::UInt16:
            syncStream = std::make_shared<SynchronousSignal<uint16_t>>(signalId, tableId, *writer, logCb);
            break;
        case daq::SampleType::Int32:
            syncStream = std::make_shared<SynchronousSignal<int32_t>>(signalId, tableId, *writer, logCb);
            break;
        case daq::SampleType::UInt32:
            syncStream = std::make_shared<SynchronousSignal<uint32_t>>(signalId, tableId, *writer, logCb);
            break;
        case daq::SampleType::Int64:
            syncStream = std::make_shared<SynchronousSignal<int64_t>>(signalId, tableId, *writer, logCb);
            break;
        case daq::SampleType::UInt64:
            syncStream = std::make_shared<SynchronousSignal<uint64_t>>(signalId, tableId, *writer, logCb);
            break;
        case daq::SampleType::Float32:
            syncStream = std::make_shared<SynchronousSignal<float>>(signalId, tableId, *writer, logCb);
            break;
        case daq::SampleType::Float64:
            syncStream = std::make_shared<SynchronousSignal<double>>(signalId, tableId, *writer, logCb);
            break;
        case daq::SampleType::ComplexFloat32:
        case daq::SampleType::ComplexFloat64:
        case daq::SampleType::Binary:
        case daq::SampleType::Invalid:
        case daq::SampleType::String:
        case daq::SampleType::RangeInt64:
        case daq::SampleType::Struct:
        default:
            throw InvalidTypeException("Unsupported data signal sample type");
    }

    SignalDescriptorConverter::ToStreamedValueSignal(signal, syncStream, getSignalProps(signal));

    return syncStream;
}

OutputSyncValueSignal::OutputSyncValueSignal(const daq::streaming_protocol::StreamWriterPtr& writer,
                                             const SignalPtr& signal, OutputDomainSignaBaselPtr outputDomainSignal,
                                             const std::string& tableId,
                                             daq::streaming_protocol::LogCallback logCb)
    : OutputValueSignalBase(createSignalStream(writer, signal, tableId, logCb), signal, outputDomainSignal, logCb)
    , syncStream(std::dynamic_pointer_cast<daq::streaming_protocol::BaseSynchronousSignal>(stream))
{
}

void OutputSyncValueSignal::writeDataPacket(const DataPacketPtr& packet)
{
    const auto domainPacket = packet.getDomainPacket();
    if (!domainPacket.assigned() || !domainPacket.getDataDescriptor().assigned())
    {
        STREAMING_PROTOCOL_LOG_E("streaming-lt: cannot stream data packet without domain packet / descriptor");
        return;
    }
    const auto packetDomainDescriptor = domainPacket.getDataDescriptor();
    if (outputDomainSignal->isTimeConfigChanged(packetDomainDescriptor) ||
        isTimeConfigChanged(packetDomainDescriptor))
    {
        STREAMING_PROTOCOL_LOG_E("Domain signal config mismatched, skip data packet");
        return;
    }

    if (doSetStartTime)
    {
        uint64_t timeStamp = domainPacket.getOffset();
        auto timeValueOffset = outputDomainSignal->calcStartTimeOffset(timeStamp);
        Int deltaInTicks = packetDomainDescriptor.getRule().getParameters().get("delta");
        uint64_t timeValueIndex = timeValueOffset / deltaInTicks;
        syncStream->setValueIndex(timeValueIndex);
        submitSignalChanges();

        STREAMING_PROTOCOL_LOG_I("data signal {}: reset time value index: {}", daqSignal.getGlobalId(), timeValueIndex);

        doSetStartTime = false;
    }

    syncStream->addData(packet.getRawData(), packet.getSampleCount());
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
