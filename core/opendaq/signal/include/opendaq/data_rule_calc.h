/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <cstring>

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

    virtual void calculateLastSample(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize, void** output)
    {
    }
};

[[maybe_unused]]
static DataRuleCalc* createDataRuleCalcTyped(const DataRulePtr& outputRule, SampleType outputType, SizeT elementCount = 1);

template <typename T>
class DataRuleCalcTyped : public DataRuleCalc
{
public:
    void* calculateRule(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize) override;
    void calculateRule(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize, void** output) override;
    void* calculateSample(const NumberPtr& packetOffset, SizeT sampleIndex, void* input, SizeT inputSize) override;
    void calculateSample(const NumberPtr& packetOffset, SizeT sampleIndex, void* input, SizeT inputSize, void** output) override;
    void calculateLastSample(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize, void** output) override;

private:
    friend DataRuleCalc* createDataRuleCalcTyped(const DataRulePtr& outputRule, SampleType outputType, SizeT elementCount);
    static std::vector<T> ParseRuleParameters(const DictPtr<IString, IBaseObject>& ruleParameters, DataRuleType type);

    explicit DataRuleCalcTyped(const DataRulePtr& rule, SizeT elementCount = 1);

    void* calculateLinearRule(const NumberPtr& packetOffset, SizeT sampleCount) const;
    void* calculateConstantRule(SizeT sampleCount, void* input, SizeT inputSize);

    void calculateLinearRule(const NumberPtr& packetOffset, SizeT sampleCount, void** output) const;
    void calculateConstantRule(SizeT sampleCount, void* input, SizeT inputSize, void** output);

    void* calculateLinearSample(const NumberPtr& packetOffset, const SizeT sampleIndex) const;
    void* calculateConstantSample(const SizeT sampleIndex, void* input, SizeT inputSize);

    void calculateLinearSample(const NumberPtr& packetOffset, const SizeT sampleIndex, void** output) const;
    void calculateConstantSample(const SizeT sampleIndex, void* input, SizeT inputSize, void** output);

    void calculateLastLinearSample(const NumberPtr& packetOffset, const SizeT sampleCount, void** output) const;
    void calculateLastConstantSample(const SizeT sampleCount, void* input, SizeT inputSize, void** output);

    DataRuleType type;
    std::vector<T> parameters;
    // Number of values in one sample; above one the constant rule works on whole vectors.
    SizeT elementCount;
};

template <typename T>
DataRuleCalcTyped<T>::DataRuleCalcTyped(const DataRulePtr& rule, SizeT elementCount)
    : elementCount(elementCount == 0 ? 1 : elementCount)
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

    DAQ_THROW_EXCEPTION(UnknownRuleTypeException);
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

    DAQ_THROW_EXCEPTION(UnknownRuleTypeException);
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

    DAQ_THROW_EXCEPTION(UnknownRuleTypeException);
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

    DAQ_THROW_EXCEPTION(UnknownRuleTypeException);
}

template <typename T>
inline void DataRuleCalcTyped<T>::calculateLastSample(
    const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize, void** output)
{
    switch (type)
    {
        case DataRuleType::Linear:
            calculateLastLinearSample(packetOffset, sampleCount, output);
            return;
        case DataRuleType::Constant:
            calculateLastConstantSample(sampleCount, input, inputSize, output);
            return;
        case DataRuleType::Other:
        case DataRuleType::Explicit:
            break;
    }

    DAQ_THROW_EXCEPTION(UnknownRuleTypeException);
}

template <typename T>
void* DataRuleCalcTyped<T>::calculateLinearRule(const NumberPtr& packetOffset, SizeT sampleCount) const
{
    auto output = std::malloc(sampleCount * sizeof(T));
    if (!output)
        DAQ_THROW_EXCEPTION(NoMemoryException, "Memory allocation failed.");

    this->calculateLinearRule(packetOffset, sampleCount, &output);
    return output;
}

template <typename T>
void* DataRuleCalcTyped<T>::calculateConstantRule(SizeT sampleCount, void* input, SizeT inputSize)
{
    auto output = std::malloc(sampleCount * elementCount * sizeof(T));
    if (!output)
        DAQ_THROW_EXCEPTION(NoMemoryException, "Memory allocation failed.");

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
    const size_t valueSize = elementCount * sizeof(T);
    if (inputSize < valueSize)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Constant rule data packet must have at least one value");

    const size_t entrySize = valueSize + sizeof(uint32_t);
    const size_t entryCount = (inputSize - valueSize) / entrySize;

    auto* outputBytes = reinterpret_cast<uint8_t*>(*output);
    auto* constant = reinterpret_cast<uint8_t*>(input);

    SizeT currentEntry = 0;
    auto* entryPtr = (reinterpret_cast<uint8_t*>(input) + valueSize);

    SizeT sampleIndex = 0;
    while (sampleIndex < sampleCount)
    {
        SizeT upToSamples;
        uint8_t* nextConstant = nullptr;
        if (currentEntry++ == entryCount)
            upToSamples = sampleCount;
        else
        {
            upToSamples = *(reinterpret_cast<uint32_t*>(entryPtr));
            entryPtr += sizeof(uint32_t);
            nextConstant = entryPtr;
            entryPtr += valueSize;
        }

        for (; sampleIndex < upToSamples; sampleIndex++)
            std::memcpy(outputBytes + sampleIndex * valueSize, constant, valueSize);

        constant = nextConstant;
    }
}

template <typename T>
inline void* DataRuleCalcTyped<T>::calculateLinearSample(const NumberPtr& packetOffset, const SizeT sampleIndex) const
{
    auto output = std::malloc(sizeof(T));
    if (!output)
        DAQ_THROW_EXCEPTION(NoMemoryException, "Memory allocation failed.");

    this->calculateLinearSample(packetOffset, sampleIndex, &output);
    return output;
}

template <typename T>
inline void* DataRuleCalcTyped<T>::calculateConstantSample(const SizeT sampleIndex, void* input, SizeT inputSize)
{
    auto output = std::malloc(elementCount * sizeof(T));
    if (!output)
        DAQ_THROW_EXCEPTION(NoMemoryException, "Memory allocation failed.");

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

    *outputTyped = scale * static_cast<T>(sampleIndex) + offset;
}

template <typename T>
inline void DataRuleCalcTyped<T>::calculateConstantSample(const SizeT sampleIndex, void* input, SizeT inputSize, void** output)
{
    const size_t valueSize = elementCount * sizeof(T);
    if (inputSize < valueSize)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Constant rule data packet must have at least one value");

    const size_t entrySize = valueSize + sizeof(uint32_t);
    const size_t entryCount = (inputSize - valueSize) / entrySize;

    auto* constant = reinterpret_cast<uint8_t*>(input);

    SizeT currentEntry = 0;
    auto* entryPtr = (reinterpret_cast<uint8_t*>(input) + valueSize);

    SizeT currentSampleIndex = 0;
    while (currentSampleIndex <= sampleIndex)
    {
        SizeT upToSamples;
        uint8_t* nextConstant = nullptr;
        if (currentEntry++ == entryCount)
        {
            upToSamples = sampleIndex;
        }
        else
        {
            upToSamples = *(reinterpret_cast<uint32_t*>(entryPtr));
            entryPtr += sizeof(uint32_t);
            nextConstant = entryPtr;
            entryPtr += valueSize;
        }

        if (upToSamples >= sampleIndex)
            break;

        currentSampleIndex = upToSamples;
        constant = nextConstant;
    }

    std::memcpy(*output, constant, valueSize);
}

template <typename T>
inline void DataRuleCalcTyped<T>::calculateLastLinearSample(const NumberPtr& packetOffset, const SizeT sampleCount, void** output) const
{
    this->calculateLinearSample(packetOffset, sampleCount - 1, output);
}

template <typename T>
inline void DataRuleCalcTyped<T>::calculateLastConstantSample(const SizeT sampleCount, void* input, SizeT inputSize, void** output)
{
    const size_t valueSize = elementCount * sizeof(T);
    if (inputSize < valueSize)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Constant rule data packet must have at least one value");

    const size_t entrySize = valueSize + sizeof(uint32_t);
    const size_t entryCount = (inputSize - valueSize) / entrySize;

    auto* basePtr = reinterpret_cast<uint8_t*>(input);

    if (entryCount == 0)
    {
        std::memcpy(*output, basePtr, valueSize);
        return;
    }

    auto* entriesStart = basePtr + valueSize;
    auto* entryPtr = entriesStart + (entryCount - 1) * entrySize;

    // Walk backwards until we find the largest upToSamples < sampleCount
    for (SizeT i = entryCount; i > 0; --i, entryPtr -= entrySize)
    {
        uint32_t upToSamples = *reinterpret_cast<uint32_t*>(entryPtr);

        if (sampleCount - 1 > upToSamples)
        {
            std::memcpy(*output, entryPtr + sizeof(uint32_t), valueSize);
            return;
        }
    }

    // If all upToSamples are >= sampleCount - 1, use the initial constant
    std::memcpy(*output, basePtr, valueSize);
}

static DataRuleCalc* createDataRuleCalcTyped(const DataRulePtr& outputRule, SampleType outputType, SizeT elementCount)
{
    switch (outputType)
    {
        case SampleType::Float32:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Float32>::Type>(outputRule, elementCount);
        case SampleType::Float64:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Float64>::Type>(outputRule, elementCount);
        case SampleType::UInt8:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::UInt8>::Type>(outputRule, elementCount);
        case SampleType::Int8:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Int8>::Type>(outputRule, elementCount);
        case SampleType::UInt16:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::UInt16>::Type>(outputRule, elementCount);
        case SampleType::Int16:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Int16>::Type>(outputRule, elementCount);
        case SampleType::UInt32:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::UInt32>::Type>(outputRule, elementCount);
        case SampleType::Int32:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Int32>::Type>(outputRule, elementCount);
        case SampleType::UInt64:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::UInt64>::Type>(outputRule, elementCount);
        case SampleType::Int64:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::Int64>::Type>(outputRule, elementCount);
        case SampleType::RangeInt64:
            return new DataRuleCalcTyped<SampleTypeToType<SampleType::RangeInt64>::Type>(outputRule, elementCount);
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

    DAQ_THROW_EXCEPTION(InvalidSampleTypeException, "The output rule output type is not supported.");
}

END_NAMESPACE_OPENDAQ
