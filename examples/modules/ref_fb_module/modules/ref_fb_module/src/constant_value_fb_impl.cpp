/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <locale>
#include <sstream>
#include <string>

#include <opendaq/component_type_private.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/sample_type_traits.h>
#include <ref_fb_module/constant_value_fb_impl.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace ConstantValue
{

namespace
{

struct ParsedElement
{
    bool isInteger;
    Int integerValue;
    Float floatValue;
};

// The values the signal carries, in the sample type the way they are written implies.
struct ParsedValues
{
    SampleType sampleType{SampleType::Int32};
    std::vector<Int> integers;
    std::vector<Float> floats;
};

ParsedElement parseElement(const std::string& element)
{
    if (element.find_first_of("xX") != std::string::npos)
        DAQ_THROW_EXCEPTION(InvalidParameterException, R"(Value "{}" is not a decimal number.)", element);

    const char* begin = element.data();
    const char* end = begin + element.size();
    if (begin != end && *begin == '+')
        ++begin;

    Int integerValue{};
    if (const auto result = std::from_chars(begin, end, integerValue); result.ec == std::errc() && result.ptr == end)
        return {true, integerValue, static_cast<Float>(integerValue)};

    // The classic locale keeps the decimal point from following whatever locale the host application set.
    std::istringstream stream(element);
    stream.imbue(std::locale::classic());

    Float floatValue;
    stream >> floatValue;
    if (stream.fail() || !(stream >> std::ws).eof() || !std::isfinite(floatValue))
        DAQ_THROW_EXCEPTION(InvalidParameterException, R"(Value "{}" is not a finite number.)", element);

    return {false, 0, floatValue};
}

// One value per element of a sample, separated by semicolons. The number of values is the vector size,
// and the way they are written decides the sample type.
ParsedValues parseValues(const std::string& text)
{
    std::vector<ParsedElement> elements;

    size_t position = 0;
    while (true)
    {
        const size_t separator = text.find(';', position);
        const std::string element =
            text.substr(position, separator == std::string::npos ? std::string::npos : separator - position);

        const size_t first = element.find_first_not_of(" \t");
        if (first == std::string::npos)
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Value must not have empty elements.");

        elements.push_back(parseElement(element.substr(first, element.find_last_not_of(" \t") - first + 1)));

        if (separator == std::string::npos)
            break;
        position = separator + 1;
    }

    ParsedValues values;

    // A single value written as a real number makes the whole signal a real one.
    const bool allIntegers = std::all_of(elements.begin(), elements.end(), [](const auto& element) { return element.isInteger; });
    if (!allIntegers)
    {
        values.sampleType = SampleType::Float64;
        for (const auto& element : elements)
            values.floats.push_back(element.floatValue);
        return values;
    }

    const bool fitsInt32 = std::all_of(elements.begin(),
                                       elements.end(),
                                       [](const auto& element)
                                       {
                                           return element.integerValue >= std::numeric_limits<int32_t>::lowest() &&
                                                  element.integerValue <= std::numeric_limits<int32_t>::max();
                                       });

    values.sampleType = fitsInt32 ? SampleType::Int32 : SampleType::Int64;
    for (const auto& element : elements)
        values.integers.push_back(element.integerValue);

    return values;
}
}

ConstantValueFbImpl::ConstantValueFbImpl(const ModuleInfoPtr& moduleInfo,
                                         const ContextPtr& ctx,
                                         const ComponentPtr& parent,
                                         const StringPtr& localId)
    : FunctionBlock(CreateType(moduleInfo), ctx, parent, localId)
    , sampleType(SampleType::Int32)
    , elementCount(1)
    , domainConnected(false)
    , rawValueSize(0)
    , valueEmitted(false)
    , emittedSampleType(SampleType::Invalid)
    , emittedRawValueSize(0)
{
    initComponentStatus();

    createSignals();
    createInputPorts();
    initProperties();
}

void ConstantValueFbImpl::createSignals()
{
    // Without a domain signal a constant rule signal hands its current value to input ports on connect;
    // the domain signal is attached only while one drives the output, and is reached through it rather
    // than on its own.
    outputSignal = createAndAddSignal(String("output"));
    outputDomainSignal = createAndAddSignal(String("output_domain"), nullptr, false);
}

void ConstantValueFbImpl::createInputPorts()
{
    inputPort = createAndAddInputPort("domain", PacketReadyNotification::SameThread);
}

void ConstantValueFbImpl::initProperties()
{
    objPtr.addProperty(StringProperty("Value", "0"));
    objPtr.getOnPropertyValueWrite("Value") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { configure(); };

    configure();
}

DataDescriptorPtr ConstantValueFbImpl::buildDescriptor() const
{
    auto builder = DataDescriptorBuilder().setName("Value").setSampleType(sampleType).setRule(ConstantDataRule());
    if (elementCount > 1)
        builder.setDimensions(List<IDimension>(Dimension(LinearDimensionRule(1, 0, elementCount), nullptr, "index")));

    return builder.build();
}

void ConstantValueFbImpl::configure()
{
    // Parsing first leaves the signal untouched when the value cannot be read.
    const StringPtr value = objPtr.getPropertyValue("Value");
    const auto values = parseValues(value.toStdString());
    const size_t newElementCount = values.sampleType == SampleType::Float64 ? values.floats.size() : values.integers.size();

    const bool descriptorChanged = !dataDescriptor.assigned() || values.sampleType != sampleType || newElementCount != elementCount;
    sampleType = values.sampleType;
    elementCount = newElementCount;

    if (descriptorChanged)
    {
        dataDescriptor = buildDescriptor();
        outputSignal.setDescriptor(dataDescriptor);
    }

    switch (sampleType)
    {
        case SampleType::Int32:
            storeValue<SampleTypeToType<SampleType::Int32>::Type>(values.integers);
            break;
        case SampleType::Int64:
            storeValue<SampleTypeToType<SampleType::Int64>::Type>(values.integers);
            break;
        case SampleType::Float64:
            storeValue<SampleTypeToType<SampleType::Float64>::Type>(values.floats);
            break;
        default:
            break;
    }

    // While a domain drives the output, the new value takes effect with the next domain packet.
    if (!domainConnected)
        emitValue();
}

template <typename T, typename Source>
void ConstantValueFbImpl::storeValue(const std::vector<Source>& values)
{
    // The sample type follows the values, so nothing is lost in the conversion.
    std::vector<T> converted;
    converted.reserve(values.size());
    for (const auto value : values)
        converted.push_back(static_cast<T>(value));

    rawValueSize = converted.size() * sizeof(T);
    rawValue = std::make_unique<uint8_t[]>(rawValueSize);
    std::memcpy(rawValue.get(), converted.data(), rawValueSize);
}

void ConstantValueFbImpl::emitValue()
{
    // Two spellings of the same number are not a change of the constant.
    if (valueEmitted && sampleType == emittedSampleType && rawValueSize == emittedRawValueSize &&
        std::memcmp(emittedRawValue.get(), rawValue.get(), rawValueSize) == 0)
        return;

    outputSignal.sendPacket(ConstantDataPacketWithRawValue(dataDescriptor, rawValue.get()));

    emittedRawValue = std::make_unique<uint8_t[]>(rawValueSize);
    std::memcpy(emittedRawValue.get(), rawValue.get(), rawValueSize);
    emittedRawValueSize = rawValueSize;
    emittedSampleType = sampleType;
    valueEmitted = true;
}

void ConstantValueFbImpl::onConnected(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();

    // The output follows a domain from here on, so it is no longer a signal whose value can be replayed.
    domainConnected = true;
    outputSignal.setDomainSignal(outputDomainSignal);
}

void ConstantValueFbImpl::onDisconnected(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();

    domainConnected = false;
    domainDescriptor = nullptr;

    // Clearing the domain descriptor first tells consumers the domain is gone while they still reference it.
    outputDomainSignal.setDescriptor(nullptr);
    outputSignal.setDomainSignal(nullptr);

    // The signal carries a value again, and with it the value a port connecting later is handed.
    valueEmitted = false;
    emitValue();
}

void ConstantValueFbImpl::onPacketReceived(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();

    const auto connection = inputPort.getConnection();
    if (!connection.assigned())
        return;

    PacketPtr packet = connection.dequeue();
    while (packet.assigned())
    {
        switch (packet.getType())
        {
            case PacketType::Event:
                processEventPacket(packet);
                break;
            case PacketType::Data:
                processDomainPacket(packet);
                break;
            default:
                break;
        }

        packet = connection.dequeue();
    }
}

void ConstantValueFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() != event_packet_id::DATA_DESCRIPTOR_CHANGED)
        return;

    const auto [valueChanged, domainChanged, newValueDescriptor, newDomainDescriptor] = parseDataDescriptorEventPacket(packet);

    // A connected value signal describes its domain separately; a domain signal is one itself.
    const DataDescriptorPtr descriptor = newDomainDescriptor.assigned() ? newDomainDescriptor : newValueDescriptor;
    if (!descriptor.assigned())
        return;

    domainDescriptor = descriptor;
    outputDomainSignal.setDescriptor(domainDescriptor);
}

void ConstantValueFbImpl::processDomainPacket(const DataPacketPtr& packet)
{
    if (!domainDescriptor.assigned() || rawValueSize == 0)
        return;

    const auto domainPacket = packet.getDomainPacket().assigned() ? packet.getDomainPacket() : packet;
    const auto sampleCount = domainPacket.getSampleCount();
    if (sampleCount == 0)
        return;

    // The raw form, because the value may be a vector, which the typed factories take one sample at a time.
    const DataPacketPtr valuePacket(
        ConstantDataPacketWithDomain_Create(domainPacket, dataDescriptor, sampleCount, rawValue.get(), nullptr, 0));

    outputDomainSignal.sendPacket(domainPacket);
    outputSignal.sendPacket(valuePacket);
}

FunctionBlockTypePtr ConstantValueFbImpl::CreateType(const ModuleInfoPtr& moduleInfo)
{
    auto fbType = FunctionBlockType("RefFBModuleConstantValue", "Constant value", "Outputs a configurable constant value");
    checkErrorInfo(fbType.asPtr<IComponentTypePrivate>(true)->setModuleInfo(moduleInfo));
    return fbType;
}
}

END_NAMESPACE_REF_FB_MODULE
