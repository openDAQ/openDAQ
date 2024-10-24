#include <websocket_streaming/input_signal.h>
#include <streaming_protocol/SubscribedSignal.hpp>
#include <websocket_streaming/signal_descriptor_converter.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/event_packet_utils.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

using namespace daq;
using namespace daq::streaming_protocol;

InputSignalBase::InputSignalBase(const std::string& signalId,
                                 const std::string& tabledId,
                                 const SubscribedSignalInfo& signalInfo,
                                 const InputSignalBasePtr& domainSignal,
                                 streaming_protocol::LogCallback logCb)
    : signalId(signalId)
    , tableId(tabledId)
    , currentDataDescriptor(signalInfo.dataDescriptor)
    , inputDomainSignal(domainSignal)
    , name(signalInfo.signalProps.name.value_or(signalInfo.signalName))
    , description(signalInfo.signalProps.description.value_or(""))
    , logCallback(logCb)
    , subscribed(false)
{
}

void InputSignalBase::processSamples(const NumberPtr& /*startDomainValue*/, const uint8_t* /*data*/, size_t /*sampleCount*/)
{
}

EventPacketPtr InputSignalBase::createDecriptorChangedPacket(bool valueChanged, bool domainChanged) const
{
    std::scoped_lock lock(descriptorsSync);

    if (isDomainSignal())
    {
        const auto valueDescParam = descriptorToEventPacketParam(currentDataDescriptor);
        return DataDescriptorChangedEventPacket(valueChanged ? valueDescParam : nullptr, nullptr);
    }
    else
    {
        const auto valueDescParam = descriptorToEventPacketParam(currentDataDescriptor);
        const auto domainDesc = inputDomainSignal->getSignalDescriptor();
        const auto domainDescParam = descriptorToEventPacketParam(domainDesc);
        return DataDescriptorChangedEventPacket(valueChanged ? valueDescParam : nullptr,
                                                domainChanged ? domainDescParam : nullptr);
    }
}

void InputSignalBase::setDataDescriptor(const DataDescriptorPtr& dataDescriptor)
{
    std::scoped_lock lock(descriptorsSync);

    currentDataDescriptor = dataDescriptor;
}

bool InputSignalBase::hasDescriptors() const
{
    std::scoped_lock lock(descriptorsSync);

    if (isDomainSignal())
        return currentDataDescriptor.assigned();
    else
        return currentDataDescriptor.assigned() &&
               inputDomainSignal && inputDomainSignal->getSignalDescriptor().assigned();
}

DataDescriptorPtr InputSignalBase::getSignalDescriptor() const
{
    std::scoped_lock lock(descriptorsSync);
    return currentDataDescriptor;
}

std::string InputSignalBase::getTableId() const
{
    return tableId;
}

std::string InputSignalBase::getSignalId() const
{
    return signalId;
}

InputSignalBasePtr InputSignalBase::getInputDomainSignal() const
{
    return inputDomainSignal;
}

void InputSignalBase::setSubscribed(bool subscribed)
{
    this->subscribed = subscribed;
}

bool InputSignalBase::getSubscribed()
{
    return subscribed;
}

InputDomainSignal::InputDomainSignal(const std::string& signalId,
                                     const std::string& tabledId,
                                     const SubscribedSignalInfo& signalInfo,
                                     streaming_protocol::LogCallback logCb)
    : InputSignalBase(signalId, tabledId, signalInfo, nullptr, logCb)
{
}

DataPacketPtr InputDomainSignal::generateDataPacket(const NumberPtr& packetOffset,
                                                    const uint8_t* /*data*/,
                                                    size_t sampleCount,
                                                    const DataPacketPtr& /*domainPacket*/)
{
    std::scoped_lock lock(descriptorsSync);

    if (!lastDomainPacket.assigned() || lastDomainPacket.getOffset() != packetOffset)
    {
        lastDomainPacket = DataPacket(currentDataDescriptor, sampleCount, packetOffset);
    }

    return lastDomainPacket;
}

bool InputDomainSignal::isDomainSignal() const
{
    return true;
}

bool InputDomainSignal::isCountable() const
{
    return false;
}

InputExplicitDataSignal::InputExplicitDataSignal(const std::string& signalId,
                                                 const std::string& tabledId,
                                                 const SubscribedSignalInfo& signalInfo,
                                                 const InputSignalBasePtr& domainSignal,
                                                 streaming_protocol::LogCallback logCb)
    : InputSignalBase(signalId, tabledId, signalInfo, domainSignal, logCb)
{
}

DataPacketPtr InputExplicitDataSignal::generateDataPacket(const NumberPtr& /*packetOffset*/,
                                                          const uint8_t* data,
                                                          size_t sampleCount,
                                                          const DataPacketPtr& domainPacket)
{
    std::scoped_lock lock(descriptorsSync);

    auto sampleType = currentDataDescriptor.getSampleType();
    if (currentDataDescriptor.getPostScaling().assigned())
        sampleType = currentDataDescriptor.getPostScaling().getInputSampleType();

    auto dataPacket = DataPacketWithDomain(domainPacket, currentDataDescriptor, sampleCount);
    const auto sampleSize = getSampleSize(sampleType);
    std::memcpy(dataPacket.getRawData(), data, sampleCount * sampleSize);
    return dataPacket;
}

bool InputExplicitDataSignal::isDomainSignal() const
{
    return false;
}

bool InputExplicitDataSignal::isCountable() const
{
    return true;
}

InputConstantDataSignal::InputConstantDataSignal(const std::string& signalId,
                                                 const std::string& tabledId,
                                                 const SubscribedSignalInfo& signalInfo,
                                                 const InputSignalBasePtr& domainSignal,
                                                 streaming_protocol::LogCallback logCb)
    : InputSignalBase(signalId, tabledId, signalInfo, domainSignal, logCb)
{
}

NumberPtr InputConstantDataSignal::calcDomainValue(const NumberPtr& startDomainValue, const uint64_t sampleIndex)
{
    NumberPtr domainRuleDelta = getDomainRuleDelta();

    if (startDomainValue.getCoreType() == CoreType::ctFloat)
        return startDomainValue.getFloatValue() + sampleIndex * domainRuleDelta.getFloatValue();
    else
        return startDomainValue.getIntValue() + sampleIndex * domainRuleDelta.getIntValue();
}

void InputConstantDataSignal::processSamples(const NumberPtr& absoluteStartDomainValue, const uint8_t* data, size_t sampleCount)
{
    auto sampleType = currentDataDescriptor.getSampleType();
    const auto sampleSize = getSampleSize(sampleType);
    const auto bufferSize = sampleCount * (sampleSize + sizeof(uint64_t));

    for (size_t addrOffset = 0; addrOffset < bufferSize; addrOffset += sizeof(uint64_t) + sampleSize)
    {
        const uint64_t* pIndex = reinterpret_cast<const uint64_t*>(data + addrOffset);
        const uint8_t* pSignalValue = data + addrOffset + sizeof(uint64_t);
        auto domainValue = calcDomainValue(absoluteStartDomainValue, *pIndex);

        SignalValueType signalValue;
        switch (sampleType)
        {
            case daq::SampleType::Int8:
                signalValue = extractConstantValue<int8_t>(pSignalValue);
                break;
            case daq::SampleType::Int16:
                signalValue = extractConstantValue<int16_t>(pSignalValue);
                break;
            case daq::SampleType::Int32:
                signalValue = extractConstantValue<int32_t>(pSignalValue);
                break;
            case daq::SampleType::Int64:
                signalValue = extractConstantValue<int64_t>(pSignalValue);
                break;
            case daq::SampleType::UInt8:
                signalValue = extractConstantValue<uint8_t>(pSignalValue);
                break;
            case daq::SampleType::UInt16:
                signalValue = extractConstantValue<uint16_t>(pSignalValue);
                break;
            case daq::SampleType::UInt32:
                signalValue = extractConstantValue<uint32_t>(pSignalValue);
                break;
            case daq::SampleType::UInt64:
                signalValue = extractConstantValue<uint64_t>(pSignalValue);
                break;
            case daq::SampleType::Float32:
                signalValue = extractConstantValue<float>(pSignalValue);
                break;
            case daq::SampleType::Float64:
                signalValue = extractConstantValue<double>(pSignalValue);
                break;
            default:
                return;
        }

        cachedSignalValues.insert_or_assign(domainValue, signalValue);
    }
}

NumberPtr InputConstantDataSignal::getDomainRuleDelta()
{
    return inputDomainSignal->getSignalDescriptor().getRule().getParameters().get("delta");
}

uint32_t InputConstantDataSignal::calcPosition(const NumberPtr& startDomainValue, const NumberPtr& domainValue)
{
    NumberPtr domainRuleDelta = getDomainRuleDelta();

    if (startDomainValue.getCoreType() == CoreType::ctFloat)
        return (domainValue.getFloatValue() - startDomainValue.getFloatValue()) / domainRuleDelta.getFloatValue();
    else
        return (domainValue.getIntValue() - startDomainValue.getIntValue()) / domainRuleDelta.getIntValue();
}

DataPacketPtr InputConstantDataSignal::generateDataPacket(const NumberPtr& /*packetOffset*/,
                                                          const uint8_t* /*data*/,
                                                          size_t sampleCount,
                                                          const DataPacketPtr& domainPacket)
{
    if (sampleCount == 0)
        return nullptr;

    std::scoped_lock lock(descriptorsSync);

    if (domainPacket.getDataDescriptor() != inputDomainSignal->getSignalDescriptor())
    {
        STREAMING_PROTOCOL_LOG_E("Fail to generate constant data packet: domain descriptor mismatch");
        return nullptr;
    }

    NumberPtr packetStartDomainValue = domainPacket.getOffset();
    NumberPtr packetEndDomainValue = calcDomainValue(packetStartDomainValue, sampleCount - 1);

    // search for cached value to be used as packet start value
    // it should have smaller or equal domain value (map key) in comparison with packet domain value
    auto itStart = cachedSignalValues.end();
    for (auto it = cachedSignalValues.begin(); it != cachedSignalValues.end(); ++it)
    {
        if (it->first <= packetStartDomainValue)
            itStart = it;
    }

    // start value is not found
    if (itStart == cachedSignalValues.end())
    {
        STREAMING_PROTOCOL_LOG_E("Fail to generate constant data packet: packet start value is unknown");
        return nullptr;
    }

    // start value found
    SignalValueType packetStartValue = itStart->second;

    // search for other cached values which belong to generated packet domain values range
    std::vector<std::pair<uint32_t, SignalValueType>> packetOtherValues;
    {
        auto it = itStart;
        ++it;
        for (; it != cachedSignalValues.end() && it->first <= packetEndDomainValue; ++it)
        {
            auto pos = calcPosition(packetStartDomainValue, it->first);
            packetOtherValues.emplace_back(pos, it->second);
        }
    }

    // erase values related to previously generated packets
    if (itStart != cachedSignalValues.begin())
    {
        cachedSignalValues.erase(cachedSignalValues.begin(), itStart);
    }

    switch (currentDataDescriptor.getSampleType())
    {
        case daq::SampleType::Int8:
            return createTypedConstantPacket<int8_t>(packetStartValue, packetOtherValues, sampleCount, domainPacket, currentDataDescriptor);
        case daq::SampleType::Int16:
            return createTypedConstantPacket<int16_t>(packetStartValue, packetOtherValues, sampleCount, domainPacket, currentDataDescriptor);
        case daq::SampleType::Int32:
            return createTypedConstantPacket<int32_t>(packetStartValue, packetOtherValues, sampleCount, domainPacket, currentDataDescriptor);
        case daq::SampleType::Int64:
            return createTypedConstantPacket<int64_t>(packetStartValue, packetOtherValues, sampleCount, domainPacket, currentDataDescriptor);
        case daq::SampleType::UInt8:
            return createTypedConstantPacket<uint8_t>(packetStartValue, packetOtherValues, sampleCount, domainPacket, currentDataDescriptor);
        case daq::SampleType::UInt16:
            return createTypedConstantPacket<uint16_t>(packetStartValue, packetOtherValues, sampleCount, domainPacket, currentDataDescriptor);
        case daq::SampleType::UInt32:
            return createTypedConstantPacket<uint32_t>(packetStartValue, packetOtherValues, sampleCount, domainPacket, currentDataDescriptor);
        case daq::SampleType::UInt64:
            return createTypedConstantPacket<uint64_t>(packetStartValue, packetOtherValues, sampleCount, domainPacket, currentDataDescriptor);
        case daq::SampleType::Float32:
            return createTypedConstantPacket<float>(packetStartValue, packetOtherValues, sampleCount, domainPacket, currentDataDescriptor);
        case daq::SampleType::Float64:
            return createTypedConstantPacket<double>(packetStartValue, packetOtherValues, sampleCount, domainPacket, currentDataDescriptor);
        default:
            STREAMING_PROTOCOL_LOG_E("Fail to generate constant data packet: unsupported sample type");
            return nullptr;
    }
}

bool InputConstantDataSignal::isDomainSignal() const
{
    return false;
}

bool InputConstantDataSignal::isCountable() const
{
    return false;
}

template<typename DataType>
InputConstantDataSignal::SignalValueType InputConstantDataSignal::extractConstantValue(const uint8_t* pValue)
{
    return SignalValueType(*(reinterpret_cast<const DataType*>(pValue)));
}

template<typename DataType>
DataPacketPtr InputConstantDataSignal::createTypedConstantPacket(
    SignalValueType startValue,
    const std::vector<std::pair<uint32_t, SignalValueType>>& otherValues,
    size_t sampleCount,
    const DataPacketPtr& domainPacket,
    const DataDescriptorPtr& dataDescriptor)
{
    const auto startValueTyped = std::get<DataType>(startValue);
    std::vector<ConstantPosAndValue<DataType>> otherValuesTyped;
    for (const auto& otherValue : otherValues)
    {
        const auto otherValueTyped = std::get<DataType>(otherValue.second);
        uint32_t pos = otherValue.first;
        otherValuesTyped.push_back({pos, otherValueTyped});
    }

    return ConstantDataPacketWithDomain<DataType>(domainPacket, dataDescriptor, sampleCount, startValueTyped, otherValuesTyped);
}

InputNullSignal::InputNullSignal(const std::string& signalId, streaming_protocol::LogCallback logCb)
    : InputSignalBase(signalId, std::string(), SubscribedSignalInfo(), nullptr, logCb)
{
}

EventPacketPtr InputNullSignal::createDecriptorChangedPacket(bool valueChanged, bool domainChanged) const
{
    return DataDescriptorChangedEventPacket(valueChanged ? NullDataDescriptor() : nullptr,
                                            domainChanged ? NullDataDescriptor() : nullptr);
}

bool InputNullSignal::hasDescriptors() const
{
    return true;
}

DataPacketPtr InputNullSignal::generateDataPacket(const NumberPtr& /*packetOffset*/,
                                                  const uint8_t* /*data*/,
                                                  size_t /*sampleCount*/,
                                                  const DataPacketPtr& /*domainPacket*/)
{
    return nullptr;
}

bool InputNullSignal::isDomainSignal() const
{
    return false;
}

bool InputNullSignal::isCountable() const
{
    return false;
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
