#include "websocket_streaming/output_signal.h"
#include <opendaq/data_descriptor_ptr.h>
#include "streaming_protocol/StreamWriter.h"
#include "streaming_protocol/SynchronousSignal.hpp"
#include "streaming_protocol/LinearTimeSignal.hpp"
#include "streaming_protocol/ConstantSignal.hpp"
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

    std::scoped_lock lock(subscribedSync);
    if(this->subscribed)
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
    if (stream)
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
                                             OutputDomainSignalBasePtr outputDomainSignal,
                                             daq::streaming_protocol::LogCallback logCb)
    : OutputSignalBase(signal, outputDomainSignal->getDaqSignal().getDescriptor(), valueStream, logCb)
    , outputDomainSignal(outputDomainSignal)
    , valueStream(valueStream)
{
}

void OutputValueSignalBase::writeDaqDataPacket(const DataPacketPtr& packet)
{
    std::scoped_lock lock(subscribedSync);

    if (!this->subscribed)
        return;

    writeDataPacket(packet);
}

void OutputValueSignalBase::writeValueDescriptorChanges(const DataDescriptorPtr& valueDescriptor)
{
    std::scoped_lock lock(subscribedSync);

    if (!this->subscribed)
        return;

    if (valueDescriptor.assigned())
    {
        this->writeDescriptorChangedEvent(valueDescriptor);
    }
    else
    {
        throw ConversionFailedException("Unassigned value descriptor");
    }
}

void OutputValueSignalBase::writeDomainDescriptorChanges(const DataDescriptorPtr& domainDescriptor)
{
    std::scoped_lock lock(subscribedSync);

    if (!this->subscribed)
        return;

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
    else
    {
        throw ConversionFailedException("Unassigned domain descriptor");
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
    std::scoped_lock lock(subscribedSync);

    if (this->subscribed != subscribed)
    {
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

void OutputDomainSignalBase::writeDaqDataPacket(const DataPacketPtr& packet)
{
    throw InvalidOperationException("Streaming-lt: explicit streaming of domain signals is not supported");
}

void OutputDomainSignalBase::writeDomainDescriptorChanges(const DataDescriptorPtr& valueDescriptor)
{
    throw InvalidOperationException("Streaming-lt: explicit streaming of domain signals is not supported");
}

void OutputDomainSignalBase::writeValueDescriptorChanges(const DataDescriptorPtr& domainDescriptor)
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
        {
            stream->subscribe();
        }
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
        {
            stream->unsubscribe();
        }
    }
}

void OutputDomainSignalBase::setSubscribed(bool subscribed)
{
    std::scoped_lock lock(subscribedSync);

    if (this->subscribed != subscribed)
    {
        this->subscribed = subscribed;
        if (subscribed)
        {
            if (subscribedByDataSignalCount == 0)
            {
                stream->subscribe();
            }
        }
        else
        {
            if (subscribedByDataSignalCount == 0)
            {
                stream->unsubscribe();
            }
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
        throw InvalidParameterException("Unsupported domain signal sample type - only 64bit integer types are supported");

    auto dataRule = descriptor.getRule();
    if (dataRule.getType() != DataRuleType::Linear)
        throw InvalidParameterException("Invalid domain signal data rule - linear rule only is supported");

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
            throw InvalidTypeException("Unsupported data signal sample type - only real numeric types are supported");
    }

    SignalDescriptorConverter::ToStreamedValueSignal(signal, syncStream, getSignalProps(signal));

    return syncStream;
}

OutputSyncValueSignal::OutputSyncValueSignal(const daq::streaming_protocol::StreamWriterPtr& writer,
                                             const SignalPtr& signal, OutputDomainSignalBasePtr outputDomainSignal,
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

BaseConstantSignalPtr OutputConstValueSignal::createSignalStream(
    const daq::streaming_protocol::StreamWriterPtr& writer,
    const SignalPtr& signal,
    const std::string& tableId,
    daq::streaming_protocol::LogCallback logCb)
{
    BaseConstantSignalPtr constStream;

    const auto valueDescriptor = signal.getDescriptor();
    auto sampleType = valueDescriptor.getSampleType();
    if (valueDescriptor.getPostScaling().assigned())
        sampleType = valueDescriptor.getPostScaling().getInputSampleType();

    const auto signalId = signal.getGlobalId();

    const auto lastValue = signal.getLastValue();
    nlohmann::json defaultStartValue(nullptr);

    switch (sampleType)
    {
        case daq::SampleType::Int8:
            if (lastValue.assigned())
                defaultStartValue = static_cast<int8_t>(lastValue.asPtr<IInteger>());
            constStream = std::make_shared<ConstantSignal<int8_t>>(signalId, tableId, *writer, defaultStartValue, logCb);
            break;
        case daq::SampleType::UInt8:
            if (lastValue.assigned())
                defaultStartValue = static_cast<uint8_t>(lastValue.asPtr<IInteger>());
            constStream = std::make_shared<ConstantSignal<uint8_t>>(signalId, tableId, *writer, defaultStartValue, logCb);
            break;
        case daq::SampleType::Int16:
            if (lastValue.assigned())
                defaultStartValue = static_cast<int16_t>(lastValue.asPtr<IInteger>());
            constStream = std::make_shared<ConstantSignal<int16_t>>(signalId, tableId, *writer, defaultStartValue, logCb);
            break;
        case daq::SampleType::UInt16:
            if (lastValue.assigned())
                defaultStartValue = static_cast<uint16_t>(lastValue.asPtr<IInteger>());
            constStream = std::make_shared<ConstantSignal<uint16_t>>(signalId, tableId, *writer, defaultStartValue, logCb);
            break;
        case daq::SampleType::Int32:
            if (lastValue.assigned())
                defaultStartValue = static_cast<int32_t>(lastValue.asPtr<IInteger>());
            constStream = std::make_shared<ConstantSignal<int32_t>>(signalId, tableId, *writer, defaultStartValue, logCb);
            break;
        case daq::SampleType::UInt32:
            if (lastValue.assigned())
                defaultStartValue = static_cast<uint32_t>(lastValue.asPtr<IInteger>());
            constStream = std::make_shared<ConstantSignal<uint32_t>>(signalId, tableId, *writer, defaultStartValue, logCb);
            break;
        case daq::SampleType::Int64:
            if (lastValue.assigned())
                defaultStartValue = static_cast<int64_t>(lastValue.asPtr<IInteger>());
            constStream = std::make_shared<ConstantSignal<int64_t>>(signalId, tableId, *writer, defaultStartValue, logCb);
            break;
        case daq::SampleType::UInt64:
            if (lastValue.assigned())
                defaultStartValue = static_cast<uint64_t>(lastValue.asPtr<IInteger>());
            constStream = std::make_shared<ConstantSignal<uint64_t>>(signalId, tableId, *writer, defaultStartValue, logCb);
            break;
        case daq::SampleType::Float32:
            if (lastValue.assigned())
                defaultStartValue = static_cast<float>(lastValue.asPtr<IFloat>());
            constStream = std::make_shared<ConstantSignal<float>>(signalId, tableId, *writer, defaultStartValue, logCb);
            break;
        case daq::SampleType::Float64:
            if (lastValue.assigned())
                defaultStartValue = static_cast<double>(lastValue.asPtr<IFloat>());
            constStream = std::make_shared<ConstantSignal<double>>(signalId, tableId, *writer, defaultStartValue, logCb);
            break;
        case daq::SampleType::ComplexFloat32:
        case daq::SampleType::ComplexFloat64:
        case daq::SampleType::Binary:
        case daq::SampleType::Invalid:
        case daq::SampleType::String:
        case daq::SampleType::RangeInt64:
        case daq::SampleType::Struct:
        default:
            throw InvalidTypeException("Unsupported data signal sample type - only real numeric types are supported");
    }

    SignalDescriptorConverter::ToStreamedValueSignal(signal, constStream, getSignalProps(signal));

    return constStream;
}

OutputConstValueSignal::OutputConstValueSignal(const daq::streaming_protocol::StreamWriterPtr& writer,
                                               const SignalPtr& signal, OutputDomainSignalBasePtr outputDomainSignal,
                                               const std::string& tableId,
                                               daq::streaming_protocol::LogCallback logCb)
    : OutputValueSignalBase(createSignalStream(writer, signal, tableId, logCb), signal, outputDomainSignal, logCb)
    , constStream(std::dynamic_pointer_cast<daq::streaming_protocol::BaseConstantSignal>(stream))
{
}

template <typename DataType>
std::vector<std::pair<DataType, uint64_t>>
OutputConstValueSignal::extractConstValuesFromDataPacket(const DataPacketPtr& packet)
{
    std::vector<std::pair<DataType, uint64_t>> result;

    const auto packetData = reinterpret_cast<DataType*>(packet.getData());
    result.push_back({packetData[0], 0});

    for (size_t index = 1; index < packet.getSampleCount(); ++index)
    {
        DataType packetDataValue = packetData[index];
        if (result.back().first != packetDataValue)
            result.push_back({packetDataValue, static_cast<uint64_t>(index)});
    }

    return result;
}

template <typename DataType>
void OutputConstValueSignal::writeData(const DataPacketPtr& packet, uint64_t firstValueIndex)
{
    if (doSetStartTime)
    {
        lastConstValue.reset();
        doSetStartTime = false;
    }

    const auto values = extractConstValuesFromDataPacket<DataType>(packet);
    const DataType packetFirstValue = values[0].first;

    if (!lastConstValue.has_value() || std::get<DataType>(lastConstValue.value()) != packetFirstValue || values.size() > 1)
    {
        size_t startFrom = 0;
        if (lastConstValue.has_value() && std::get<DataType>(lastConstValue.value()) == packetFirstValue)
            startFrom = 1;

        auto valuesCount = values.size();

        std::vector<DataType> constants;
        std::vector<uint64_t> indices;

        for (size_t i = startFrom; i < valuesCount; ++i)
        {
            constants.push_back(static_cast<DataType>(values[i].first));
            indices.push_back(values[i].second + firstValueIndex);
        }

        constStream->addData(constants.data(), indices.data(), valuesCount);
    }

    lastConstValue = values.back().first;
}

void OutputConstValueSignal::writeDataPacket(const DataPacketPtr& packet)
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

    uint64_t timeStamp = domainPacket.getOffset();
    auto timeValueOffset = outputDomainSignal->calcStartTimeOffset(timeStamp);
    Int deltaInTicks = packetDomainDescriptor.getRule().getParameters().get("delta");
    uint64_t firstValueIndex = timeValueOffset / deltaInTicks;

    auto sampleType = packet.getDataDescriptor().getSampleType();
    switch (sampleType) {
        case daq::SampleType::Int8:
            writeData<int8_t>(packet, firstValueIndex);
            break;
        case daq::SampleType::Int16:
            writeData<int16_t>(packet, firstValueIndex);
            break;
        case daq::SampleType::Int32:
            writeData<int32_t>(packet, firstValueIndex);
            break;
        case daq::SampleType::Int64:
            writeData<int64_t>(packet, firstValueIndex);
            break;
        case daq::SampleType::UInt8:
            writeData<uint8_t>(packet, firstValueIndex);
            break;
        case daq::SampleType::UInt16:
            writeData<uint16_t>(packet, firstValueIndex);
            break;
        case daq::SampleType::UInt32:
            writeData<uint32_t>(packet, firstValueIndex);
            break;
        case daq::SampleType::UInt64:
            writeData<uint64_t>(packet, firstValueIndex);
            break;
        case daq::SampleType::Float32:
            writeData<float>(packet, firstValueIndex);
            break;
        case daq::SampleType::Float64:
            writeData<double>(packet, firstValueIndex);
            break;
        default:
            STREAMING_PROTOCOL_LOG_E("Unsupported sample type, skip data packet");
            break;
    }
}

void OutputConstValueSignal::setSubscribed(bool subscribed)
{
    std::scoped_lock lock(subscribedSync);

    if (this->subscribed != subscribed)
    {
        this->subscribed = subscribed;
        doSetStartTime = true;
        lastConstValue.reset();

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

OutputNullSignal::OutputNullSignal(const SignalPtr& signal, daq::streaming_protocol::LogCallback logCb)
    : OutputSignalBase(signal, nullptr, nullptr, logCb)
{
}

void OutputNullSignal::writeDomainDescriptorChanges(const DataDescriptorPtr& valueDescriptor)
{
}

void OutputNullSignal::writeValueDescriptorChanges(const DataDescriptorPtr& domainDescriptor)
{
}

void OutputNullSignal::writeDaqDataPacket(const DataPacketPtr& packet)
{
}

void OutputNullSignal::setSubscribed(bool subscribed)
{
    std::scoped_lock lock(subscribedSync);

    this->subscribed = subscribed;
}

bool OutputNullSignal::isDataSignal()
{
    return daqSignal.getDomainSignal().assigned();
}

void OutputNullSignal::toStreamedSignal(const SignalPtr& signal, const SignalProps& sigProps)
{
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
