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

template <typename Func>
auto InputConstantDataSignal::callWithSampleType(daq::SampleType sampleType, Func&& func)
{
    switch (sampleType)
    {
        case daq::SampleType::Int8:
            return func(daq::SampleTypeToType<daq::SampleType::Int8>::Type{});
        case daq::SampleType::Int16:
            return func(daq::SampleTypeToType<daq::SampleType::Int16>::Type{});
        case daq::SampleType::Int32:
            return func(daq::SampleTypeToType<daq::SampleType::Int32>::Type{});
        case daq::SampleType::Int64:
            return func(daq::SampleTypeToType<daq::SampleType::Int64>::Type{});
        case daq::SampleType::UInt8:
            return func(daq::SampleTypeToType<daq::SampleType::UInt8>::Type{});
        case daq::SampleType::UInt16:
            return func(daq::SampleTypeToType<daq::SampleType::UInt16>::Type{});
        case daq::SampleType::UInt32:
            return func(daq::SampleTypeToType<daq::SampleType::UInt32>::Type{});
        case daq::SampleType::UInt64:
            return func(daq::SampleTypeToType<daq::SampleType::UInt64>::Type{});
        case daq::SampleType::Float32:
            return func(daq::SampleTypeToType<daq::SampleType::Float32>::Type{});
        case daq::SampleType::Float64:
            return func(daq::SampleTypeToType<daq::SampleType::Float64>::Type{});
        default:
            throw std::invalid_argument("Unsupported sample type");
    }
}

InputConstantDataSignal::InputConstantDataSignal(const std::string& signalId,
                                                 const std::string& tabledId,
                                                 const SubscribedSignalInfo& signalInfo,
                                                 const InputSignalBasePtr& domainSignal,
                                                 streaming_protocol::LogCallback logCb,
                                                 const nlohmann::json& metaInfoStartValue)
    : InputSignalBase(signalId, tabledId, signalInfo, domainSignal, logCb)
    , suppressDefaultStartValueWarnings(false)
{
    updateStartValue(metaInfoStartValue);
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
    std::scoped_lock lock(descriptorsSync);

    auto sampleType = currentDataDescriptor.getSampleType();
    const auto sampleSize = getSampleSize(sampleType);
    const auto bufferSize = sampleCount * (sampleSize + sizeof(uint64_t));

    for (size_t addrOffset = 0; addrOffset < bufferSize; addrOffset += sizeof(uint64_t) + sampleSize)
    {
        const uint64_t* pIndex = reinterpret_cast<const uint64_t*>(data + addrOffset);
        const uint8_t* pSignalValue = data + addrOffset + sizeof(uint64_t);
        auto domainValue = calcDomainValue(absoluteStartDomainValue, *pIndex);

        try
        {
            auto extractConstantValue = [pSignalValue](const auto& typeTag)
            {
                using DataType = typename std::decay_t<decltype(typeTag)>;
                return SignalValueType(*(reinterpret_cast<const DataType*>(pSignalValue)));
            };
            SignalValueType signalValue = callWithSampleType(sampleType, extractConstantValue);
            cachedSignalValues.insert_or_assign(domainValue, signalValue);
        }
        catch (...)
        {
            return;
        }
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

InputConstantDataSignal::CachedSignalValues::iterator InputConstantDataSignal::insertDefaultValue(const NumberPtr& domainValue)
{
    if (!suppressDefaultStartValueWarnings)
    {
        if (defaultStartValue.has_value())
        {
            STREAMING_PROTOCOL_LOG_W("Constant rule signal id \"{}\" (table \"{}\"): "
                                     "packet start value isn't yet received, will use default start value from meta-info",
                                     this->signalId,
                                     this->tableId);
        }
        else
        {
            STREAMING_PROTOCOL_LOG_W("Constant rule signal id \"{}\" (table \"{}\"): "
                                     "packet start value isn't yet received nor valid one provided in meta-info, will use default start value 0",
                                     this->signalId,
                                     this->tableId);
        }
        suppressDefaultStartValueWarnings = true; // log warning message just ones
    }

    auto createZeroValue = [](const auto& typeTag)
    {
        using DataType = typename std::decay_t<decltype(typeTag)>;
        return SignalValueType(static_cast<DataType>(0));
    };
    SignalValueType zeroValue = callWithSampleType(currentDataDescriptor.getSampleType(), createZeroValue);
    const auto result = cachedSignalValues.insert_or_assign(domainValue, defaultStartValue.value_or(zeroValue));
    return result.first;
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

    bool removeDefaultValueFromCache = false;
    // appropriate start value is not found as it wasn't received as signal data
    // temporary insert default value into cache and use it to generate packet
    if (itStart == cachedSignalValues.end())
    {
        try
        {
            itStart = insertDefaultValue(packetStartDomainValue);
            removeDefaultValueFromCache = true;
        }
        catch (const std::exception& e)
        {
            STREAMING_PROTOCOL_LOG_E("Fail to generate constant data packet: {}", e.what());
            return nullptr;
        }
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
    // erase temporary inserted default value
    if (removeDefaultValueFromCache)
    {
        cachedSignalValues.erase(itStart);
    }

    try
    {
        auto createPacket = [&](const auto& typeTag)
        {
            using DataType = typename std::decay_t<decltype(typeTag)>;
            return createTypedConstantPacket<DataType>(packetStartValue, packetOtherValues, sampleCount, domainPacket, currentDataDescriptor);
        };
        return callWithSampleType(currentDataDescriptor.getSampleType(), createPacket);
    }
    catch (const std::exception& e)
    {
        STREAMING_PROTOCOL_LOG_E("Fail to generate constant data packet: {}", e.what());
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

void InputConstantDataSignal::updateStartValue(const nlohmann::json& metaInfoStartValue)
{
    std::scoped_lock lock(descriptorsSync);

    try
    {
        auto getValueFromJson = [&metaInfoStartValue](const auto& typeTag)
        {
            using DataType = typename std::decay_t<decltype(typeTag)>;
            return SignalValueType(convertToNumeric<DataType>(metaInfoStartValue));
        };
        defaultStartValue = callWithSampleType(currentDataDescriptor.getSampleType(), getValueFromJson);
        suppressDefaultStartValueWarnings = false;
    }
    catch (const std::exception& e)
    {
        STREAMING_PROTOCOL_LOG_I("Cannot get default start value from signal meta-info: {}", e.what());
    }
}

template<typename DataType>
DataType InputConstantDataSignal::convertToNumeric(const nlohmann::json& jsonNumeric)
{
    if (jsonNumeric.is_null())
        throw std::invalid_argument("No value provided");

    if (!jsonNumeric.is_number())
        throw std::invalid_argument("JSON value is not number");

    if constexpr (std::is_floating_point<DataType>::value)
    {
        double numeric = jsonNumeric.get<double>();
        if (numeric < std::numeric_limits<DataType>::min() || numeric > std::numeric_limits<DataType>::max())
            throw std::out_of_range("Value out of range");
        return static_cast<DataType>(numeric);
    }
    else if constexpr (std::is_signed<DataType>::value)
    {
        int64_t numeric = jsonNumeric.get<int64_t>();
        if (numeric < std::numeric_limits<DataType>::min() || numeric > std::numeric_limits<DataType>::max())
            throw std::out_of_range("Value out of range");
        return static_cast<DataType>(numeric);
    }
    else if constexpr (std::is_unsigned<DataType>::value)
    {
        uint64_t numeric = jsonNumeric.get<uint64_t>();
        if (numeric < std::numeric_limits<DataType>::min() || numeric > std::numeric_limits<DataType>::max())
            throw std::out_of_range("Value out of range");
        return static_cast<DataType>(numeric);
    }
    throw std::invalid_argument("Conversion failed - invalid sample type");
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
