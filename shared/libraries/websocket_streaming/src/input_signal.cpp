#include <websocket_streaming/input_signal.h>
#include <streaming_protocol/SubscribedSignal.hpp>
#include <websocket_streaming/signal_descriptor_converter.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/sample_type_traits.h>

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
{
}

void InputSignalBase::processSamples(uint64_t /*packetOffset*/, const uint8_t* /*data*/, size_t /*sampleCount*/)
{
}

EventPacketPtr InputSignalBase::createDecriptorChangedPacket(bool valueChanged, bool domainChanged) const
{
    std::scoped_lock lock(descriptorsSync);

    if (isDomainSignal())
    {
        const auto valueDesc = valueChanged ? currentDataDescriptor : nullptr;
        return DataDescriptorChangedEventPacket(valueDesc, nullptr);
    }
    else
    {
        const auto valueDesc = valueChanged ? currentDataDescriptor : nullptr;
        const auto domainDesc = domainChanged ? inputDomainSignal->getSignalDescriptor() : nullptr;
        return DataDescriptorChangedEventPacket(valueDesc, domainDesc);
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

InputDomainSignal::InputDomainSignal(const std::string& signalId,
                                     const std::string& tabledId,
                                     const SubscribedSignalInfo& signalInfo,
                                     streaming_protocol::LogCallback logCb)
    : InputSignalBase(signalId, tabledId, signalInfo, nullptr, logCb)
{
}

DataPacketPtr InputDomainSignal::generateDataPacket(uint64_t packetOffset,
                                                    const uint8_t* /*data*/,
                                                    size_t sampleCount,
                                                    const DataPacketPtr& /*domainPacket*/)
{
    std::scoped_lock lock(descriptorsSync);

    if (!lastDomainPacket.assigned() || lastDomainPacket.getOffset().getIntValue() != (Int) packetOffset)
    {
        lastDomainPacket = DataPacket(currentDataDescriptor, sampleCount, (Int) packetOffset);
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

DataPacketPtr InputExplicitDataSignal::generateDataPacket(uint64_t /*packetOffset*/,
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

void InputConstantDataSignal::processSamples(uint64_t timestamp, const uint8_t* data, size_t sampleCount)
{
    auto sampleType = currentDataDescriptor.getSampleType();
    const auto sampleSize = getSampleSize(sampleType);
    const auto bufferSize = sampleCount * (sampleSize + sizeof(uint64_t));
    Int delta = inputDomainSignal->getSignalDescriptor().getRule().getParameters().get("delta");

    for (size_t offset = 0; offset < bufferSize; offset += sizeof(uint64_t) + sampleSize)
    {
        const uint64_t* pIndex = reinterpret_cast<const uint64_t*>(data + offset);
        const uint8_t* pConstantValue = data + offset + sizeof(uint64_t);
        NumberPtr timeStamp = (Int)(*pIndex) * delta + timestamp;

        ConstantValueType constantValue;
        switch (sampleType)
        {
            case daq::SampleType::Int8:
                constantValue = extractConstantValue<int8_t>(pConstantValue);
                break;
            case daq::SampleType::Int16:
                constantValue = extractConstantValue<int16_t>(pConstantValue);
                break;
            case daq::SampleType::Int32:
                constantValue = extractConstantValue<int32_t>(pConstantValue);
                break;
            case daq::SampleType::Int64:
                constantValue = extractConstantValue<int64_t>(pConstantValue);
                break;
            case daq::SampleType::UInt8:
                constantValue = extractConstantValue<uint8_t>(pConstantValue);
                break;
            case daq::SampleType::UInt16:
                constantValue = extractConstantValue<uint16_t>(pConstantValue);
                break;
            case daq::SampleType::UInt32:
                constantValue = extractConstantValue<uint32_t>(pConstantValue);
                break;
            case daq::SampleType::UInt64:
                constantValue = extractConstantValue<uint64_t>(pConstantValue);
                break;
            case daq::SampleType::Float32:
                constantValue = extractConstantValue<float>(pConstantValue);
                break;
            case daq::SampleType::Float64:
                constantValue = extractConstantValue<double>(pConstantValue);
                break;
            default:
                return;
        }

        cachedConstantValues.insert_or_assign(timeStamp, constantValue);
    }
}

DataPacketPtr InputConstantDataSignal::generateDataPacket(uint64_t /*packetOffset*/,
                                                          const uint8_t* /*data*/,
                                                          size_t sampleCount,
                                                          const DataPacketPtr& domainPacket)
{
    std::scoped_lock lock(descriptorsSync);

    auto sampleType = currentDataDescriptor.getSampleType();

    Int delta = domainPacket.getDataDescriptor().getRule().getParameters().get("delta");
    Int startOffset = domainPacket.getOffset();
    Int endOffset = startOffset + delta * sampleCount;

    DataPacketPtr dataPacket;
    ConstantValueType startValue;

    if (auto it = cachedConstantValues.find(startOffset); it != cachedConstantValues.end())
        startValue = it->second;
    else if (lastConstantValue.has_value())
        startValue = lastConstantValue.value();
    else
        return nullptr;

    // TODO erase cached values that was already used to generate packets
    std::vector<std::pair<uint32_t, ConstantValueType>> otherValues;
    if (!cachedConstantValues.empty())
    {
        auto itStart = cachedConstantValues.upper_bound(startOffset);
        auto itEnd = cachedConstantValues.lower_bound(endOffset);

        for (auto it = itStart; it != itEnd; ++it)
        {
            uint32_t pos = (it->first.getIntValue() - startOffset) / delta;
            otherValues.emplace_back(pos, it->second);
            lastConstantValue = it->second;
        }
    }

    switch (sampleType)
    {
        case daq::SampleType::Int8:
            return createTypedConstantPacket<int8_t>(startValue, otherValues, sampleCount, domainPacket, currentDataDescriptor);
            break;
        case daq::SampleType::Int16:
            return createTypedConstantPacket<int16_t>(startValue, otherValues, sampleCount, domainPacket, currentDataDescriptor);
            break;
        case daq::SampleType::Int32:
            return createTypedConstantPacket<int32_t>(startValue, otherValues, sampleCount, domainPacket, currentDataDescriptor);
            break;
        case daq::SampleType::Int64:
            return createTypedConstantPacket<int64_t>(startValue, otherValues, sampleCount, domainPacket, currentDataDescriptor);
            break;
        case daq::SampleType::UInt8:
            return createTypedConstantPacket<uint8_t>(startValue, otherValues, sampleCount, domainPacket, currentDataDescriptor);
            break;
        case daq::SampleType::UInt16:
            return createTypedConstantPacket<uint16_t>(startValue, otherValues, sampleCount, domainPacket, currentDataDescriptor);
            break;
        case daq::SampleType::UInt32:
            return createTypedConstantPacket<uint32_t>(startValue, otherValues, sampleCount, domainPacket, currentDataDescriptor);
            break;
        case daq::SampleType::UInt64:
            return createTypedConstantPacket<uint64_t>(startValue, otherValues, sampleCount, domainPacket, currentDataDescriptor);
            break;
        case daq::SampleType::Float32:
            return createTypedConstantPacket<float>(startValue, otherValues, sampleCount, domainPacket, currentDataDescriptor);
            break;
        case daq::SampleType::Float64:
            return createTypedConstantPacket<double>(startValue, otherValues, sampleCount, domainPacket, currentDataDescriptor);
            break;
        default:
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
InputConstantDataSignal::ConstantValueType InputConstantDataSignal::extractConstantValue(const uint8_t* pValue)
{
    return ConstantValueType(*(reinterpret_cast<const DataType*>(pValue)));
}

template<typename DataType>
DataPacketPtr InputConstantDataSignal::createTypedConstantPacket(
    ConstantValueType startValue,
    const std::vector<std::pair<uint32_t, ConstantValueType>>& otherValues,
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

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
