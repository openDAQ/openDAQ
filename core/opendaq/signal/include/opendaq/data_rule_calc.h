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

#pragma once
#include <opendaq/data_rule_ptr.h>
#include <opendaq/signal_exceptions.h>
#include <opendaq/range_type.h>
#include <opendaq/sample_type_traits.h>

BEGIN_NAMESPACE_OPENDAQ

class DataRuleCalc
{
public:
    virtual ~DataRuleCalc() = default;

    virtual void* calculateRule(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize)
    {
        return nullptr;
    }
    
    virtual void calculateRule(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize, void** output)
    {
    }

    virtual void* calculateSample(const NumberPtr& packetOffset, SizeT sampleIndex, void* input, SizeT inputSize)
    {
        return nullptr;
    }

    virtual void calculateSample(const NumberPtr& packetOffset, SizeT sampleIndex, void* input, SizeT inputSize, void** output)
    {
    }
};

[[maybe_unused]]
static DataRuleCalc* createDataRuleCalcTyped(const DataRulePtr& outputRule, SampleType outputType);

template <typename T>
class DataRuleCalcTyped : public DataRuleCalc
{
public:
    void* calculateRule(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize) override;
    void calculateRule(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize, void** output) override;
    void* calculateSample(const NumberPtr& packetOffset, SizeT sampleIndex, void* input, SizeT inputSize) override;
    void calculateSample(const NumberPtr& packetOffset, SizeT sampleIndex, void* input, SizeT inputSize, void** output) override;

private:
    friend DataRuleCalc* createDataRuleCalcTyped(const DataRulePtr& outputRule, SampleType outputType);
    static std::vector<T> ParseRuleParameters(const DictPtr<IString, IBaseObject>& ruleParameters, DataRuleType type);

    explicit DataRuleCalcTyped(const DataRulePtr& rule);

    void* calculateLinearRule(const NumberPtr& packetOffset, SizeT sampleCount) const;
    void* calculateConstantRule(SizeT sampleCount, void* input, SizeT inputSize);

    void calculateLinearRule(const NumberPtr& packetOffset, SizeT sampleCount, void** output) const;
    void calculateConstantRule(SizeT sampleCount, void* input, SizeT inputSize, void** output);

    void* calculateLinearSample(const NumberPtr& packetOffset, const SizeT sampleIndex) const;
    void* calculateConstantSample(const SizeT sampleIndex, void* input, SizeT inputSize);

    void calculateLinearSample(const NumberPtr& packetOffset, const SizeT sampleIndex, void** output) const;
    void calculateConstantSample(const SizeT sampleIndex, void* input, SizeT inputSize, void** output);

    DataRuleType type;
    std::vector<T> parameters;
};

template <typename T>
DataRuleCalcTyped<T>::DataRuleCalcTyped(const DataRulePtr& rule)
{
    type = rule.getType();
    parameters = ParseRuleParameters(rule.getParameters(), type);
}

template <typename T>
std::vector<T> DataRuleCalcTyped<T>::ParseRuleParameters(const DictPtr<IString, IBaseObject>& ruleParameters, DataRuleType type)
{
    std::vector<T> parameters{};
    if (type == DataRuleType::Linear)
    {
        T delta = ruleParameters.get("delta");
        T start = ruleParameters.get("start");
        parameters.push_back(delta);
        parameters.push_back(start);
    }

    return parameters;
}

template <>
inline std::vector<RangeType64> DataRuleCalcTyped<RangeType64>::ParseRuleParameters(const DictPtr<IString, IBaseObject>& ruleParameters, DataRuleType type)
{
    std::vector<RangeType64> parameters{};
    if (type == DataRuleType::Linear)
    {
        RangeType64 delta = (RangeType64::Type) ruleParameters.get("delta");
        RangeType64 start = (RangeType64::Type) ruleParameters.get("start");
        parameters.push_back(delta);
        parameters.push_back(start);
    }
    return parameters;
}

template <>
inline std::vector<uint8_t> DataRuleCalcTyped<uint8_t>::ParseRuleParameters(const DictPtr<IString, IBaseObject>& ruleParameters, DataRuleType type)
{
    std::vector<uint8_t> parameters{};
    if (type == DataRuleType::Linear)
    {
        int16_t delta = ruleParameters.get("delta");
        int16_t start = ruleParameters.get("start");
        parameters.push_back(static_cast<uint8_t>(delta));
        parameters.push_back(static_cast<uint8_t>(start));
    }
    return parameters;
}

template <typename T>
void* DataRuleCalcTyped<T>::calculateRule(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize)
{
    switch (type)
    {

        case DataRuleType::Linear:
            return calculateLinearRule(packetOffset, sampleCount);
        case DataRuleType::Constant:
            return calculateConstantRule(sampleCount, input, inputSize);
        case DataRuleType::Other:
        case DataRuleType::Explicit:
            break;
    }

    throw UnknownRuleTypeException{};
}

template <typename T>
void DataRuleCalcTyped<T>::calculateRule(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize, void** output)
{
    switch (type)
    {
        case DataRuleType::Linear:
            calculateLinearRule(packetOffset, sampleCount, output);
            return;
        case DataRuleType::Constant:
            calculateConstantRule(sampleCount, input, inputSize, output);
            return;
        case DataRuleType::Other:
        case DataRuleType::Explicit:
            break;
    }

    throw UnknownRuleTypeException{};
}

template <typename T>
inline void* DataRuleCalcTyped<T>::calculateSample(const NumberPtr& packetOffset, SizeT sampleIndex, void* input, SizeT inputSize)
{
    switch (type)
    {
        case DataRuleType::Linear:
            return calculateLinearSample(packetOffset, sampleIndex);
        case DataRuleType::Constant:
            return calculateConstantSample(sampleIndex, input, inputSize);
        case DataRuleType::Other:
        case DataRuleType::Explicit:
            break;
    }

    throw UnknownRuleTypeException{};
}

template <typename T>
inline void DataRuleCalcTyped<T>::calculateSample(
    const NumberPtr& packetOffset, SizeT sampleIndex, void* input, SizeT inputSize, void** output)
{
    switch (type)
    {
        case DataRuleType::Linear:
            calculateLinearSample(packetOffset, sampleIndex, output);
            return;
        case DataRuleType::Constant:
            calculateConstantSample(sampleIndex, input, inputSize, output);
            return;
        case DataRuleType::Other:
        case DataRuleType::Explicit:
            break;
    }

    throw UnknownRuleTypeException{};
}

template <typename T>
void* DataRuleCalcTyped<T>::calculateLinearRule(const NumberPtr& packetOffset, SizeT sampleCount) const
{
    auto output = std::malloc(sampleCount * sizeof(T));
    if (!output)
        throw NoMemoryException("Memory allocation failed.");

    this->calculateLinearRule(packetOffset, sampleCount, &output);
    return output;
}

template <typename T>
void* DataRuleCalcTyped<T>::calculateConstantRule(SizeT sampleCount, void* input, SizeT inputSize)
{
    auto output = std::malloc(sampleCount * sizeof(T));
    if (!output)
        throw NoMemoryException("Memory allocation failed.");

    this->calculateConstantRule(sampleCount, input, inputSize, &output);
    return output;
}

template <>
inline void DataRuleCalcTyped<ClockRange>::calculateLinearRule(const NumberPtr& packetOffset, SizeT sampleCount, void** output) const
{
    auto outputTyped = static_cast<ClockRange*>(*output);

    for (SizeT i = 0; i < sampleCount; ++i)
    {
        outputTyped[i] = ClockRange(packetOffset);
        outputTyped[i].start += i * parameters[0].start + parameters[1].start;
    }
}

template <typename T>
void DataRuleCalcTyped<T>::calculateLinearRule(const NumberPtr& packetOffset, SizeT sampleCount, void** output) const
{
    T* outputTyped = static_cast<T*>(*output);
    const T scale = parameters[0];
    const T offset = static_cast<T>(packetOffset) + parameters[1];
    for (SizeT i = 0; i < sampleCount; ++i)
        outputTyped[i] = scale * static_cast<T>(i) + offset;
}

template <typename T>
void DataRuleCalcTyped<T>::calculateConstantRule(SizeT sampleCount, void* input, SizeT inputSize, void** output)
{
    if (inputSize < sizeof(T))
        throw InvalidParameterException("Constant rule data packet must have at least one value");

    constexpr size_t entrySize = sizeof(T) + sizeof(uint32_t);
    const size_t entryCount = (inputSize - sizeof(T)) / entrySize;

    T* outputTyped = static_cast<T*>(*output);
    T constant = *static_cast<T*>(input);

    SizeT currentEntry = 0;
    auto* entryPtr = (reinterpret_cast<uint8_t*>(input) + sizeof(T));

    SizeT sampleIndex = 0;
    while (sampleIndex < sampleCount)
    {
        SizeT upToSamples;
        T nextConstantValue{};
        if (currentEntry++ == entryCount)
            upToSamples = sampleCount;
        else
        {
            upToSamples = *(reinterpret_cast<uint32_t*>(entryPtr));
            entryPtr += sizeof(uint32_t);
            nextConstantValue = *(reinterpret_cast<T*>(entryPtr));
            entryPtr += sizeof(T);
        }

        for (; sampleIndex < upToSamples; sampleIndex++)
            outputTyped[sampleIndex] = constant;

        constant = nextConstantValue;
    }
}

template <typename T>
inline void* DataRuleCalcTyped<T>::calculateLinearSample(const NumberPtr& packetOffset, const SizeT sampleIndex) const
{
    auto output = std::malloc(sizeof(T));
    if (!output)
        throw NoMemoryException("Memory allocation failed.");

    this->calculateLinearSample(packetOffset, sampleIndex, &output);
    return output;
}

template <typename T>
inline void* DataRuleCalcTyped<T>::calculateConstantSample(const SizeT sampleIndex, void* input, SizeT inputSize)
{
    auto output = std::malloc(sizeof(T));
    if (!output)
        throw NoMemoryException("Memory allocation failed.");

    this->calculateConstantSample(sampleIndex, input, inputSize, &output);
    return output;
}

template<>
inline void DataRuleCalcTyped<ClockRange>::calculateLinearSample(const NumberPtr& packetOffset, const SizeT sampleIndex, void** output) const
{
    auto* outputTyped = static_cast<ClockRange*>(*output);
    *outputTyped = ClockRange(packetOffset);

    outputTyped->start = parameters[0].start * sampleIndex + (parameters[1].start + packetOffset.getIntValue());
}

template <typename T>
inline void DataRuleCalcTyped<T>::calculateLinearSample(const NumberPtr& packetOffset, const SizeT sampleIndex, void** output) const
{
    T* outputTyped = static_cast<T*>(*output);
    const T scale = parameters[0];
    const T offset = static_cast<T>(packetOffset) + parameters[1];

    *outputTyped = scale * sampleIndex + offset;
}

template <typename T>
inline void DataRuleCalcTyped<T>::calculateConstantSample(const SizeT sampleIndex, void* input, SizeT inputSize, void** output)
{
    if (inputSize < sizeof(T))
        throw InvalidParameterException("Constant rule data packet must have at least one value");

    constexpr size_t entrySize = sizeof(T) + sizeof(uint32_t);
    const size_t entryCount = (inputSize - sizeof(T)) / entrySize;

    T* outputTyped = static_cast<T*>(*output);
    T constant = *static_cast<T*>(input);

    SizeT currentEntry = 0;
    auto* entryPtr = (reinterpret_cast<uint8_t*>(input) + sizeof(T));

    SizeT currentSampleIndex = 0;
    while (currentSampleIndex <= sampleIndex)
    {
        SizeT upToSamples;
        T nextConstantValue{};
        if (currentEntry++ == entryCount)
        {
            upToSamples = sampleIndex;
        }
        else
        {
            upToSamples = *(reinterpret_cast<uint32_t*>(entryPtr));
            entryPtr += sizeof(uint32_t);
            nextConstantValue = *(reinterpret_cast<T*>(entryPtr));
            entryPtr += sizeof(T);
        }

        if (currentSampleIndex + upToSamples >= sampleIndex)
            break;

        currentSampleIndex += upToSamples;
        constant = nextConstantValue;
    }

    *outputTyped = constant;
}

static DataRuleCalc* createDataRuleCalcTyped(const DataRulePtr& outputRule, SampleType outputType)
{
    switch (outputType)
    {
        case SampleType::Float32:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Float32>::Type>(outputRule);
        case SampleType::Float64:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Float64>::Type>(outputRule);
        case SampleType::UInt8:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::UInt8>::Type>(outputRule);
        case SampleType::Int8:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Int8>::Type>(outputRule);
        case SampleType::UInt16:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::UInt16>::Type>(outputRule);
        case SampleType::Int16:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Int16>::Type>(outputRule);
        case SampleType::UInt32:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::UInt32>::Type>(outputRule);
        case SampleType::Int32:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Int32>::Type>(outputRule);
        case SampleType::UInt64:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::UInt64>::Type>(outputRule);
        case SampleType::Int64:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Int64>::Type>(outputRule);
        case SampleType::RangeInt64:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::RangeInt64>::Type>(outputRule);
        case SampleType::Binary:
        case SampleType::ComplexFloat32:
        case SampleType::ComplexFloat64:
        case SampleType::Invalid:
        case SampleType::String:
        case SampleType::Struct:
        case SampleType::Null:
        case SampleType::_count:
            break;
    }

    throw InvalidSampleTypeException{"The output rule output type is not supported."};
}

END_NAMESPACE_OPENDAQ
