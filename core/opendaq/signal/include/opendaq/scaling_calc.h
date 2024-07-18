/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/scaling_ptr.h>
#include <opendaq/signal_exceptions.h>
#include <opendaq/sample_type_traits.h>

BEGIN_NAMESPACE_OPENDAQ

class ScalingCalc
{
public:
    virtual ~ScalingCalc() = default;

    virtual void* scaleData(void* data, SizeT sampleCount)
    {
        return nullptr;
    }
    
    virtual void scaleData(void* data, SizeT sampleCount, void** output)
    {
    }
};

[[maybe_unused]]
static ScalingCalc* createScalingCalcTyped(const ScalingPtr& scaling);

template <typename T, typename U>
class ScalingCalcTyped : public ScalingCalc
{
public:
    void* scaleData(void* data, SizeT sampleCount) override;
    void scaleData(void* data, SizeT sampleCount, void** output) override;

private:
    friend ScalingCalc* createScalingCalcTyped(const ScalingPtr& scaling);
    ScalingCalcTyped(const ScalingPtr& scaling);

    void* scaleLinear(void* data, SizeT sampleCount);
    void scaleLinear(void* data, SizeT sampleCount, void** output);

    ScalingType type;
    std::vector<U> params;
};

template <typename T, typename U>
ScalingCalcTyped<T, U>::ScalingCalcTyped(const ScalingPtr& scaling)
{
    type = scaling.getType();
    if (type == ScalingType::Linear)
    {
        U scale = scaling.getParameters().get("scale");
        U offset = scaling.getParameters().get("offset");
        params.push_back(scale);
        params.push_back(offset);
    }
}

template <typename T, typename U>
void* ScalingCalcTyped<T, U>::scaleData(void* data, SizeT sampleCount)
{
    if (type == ScalingType::Linear)
        return scaleLinear(data, sampleCount);

    throw(UnknownRuleTypeException{});
}

template <typename T, typename U>
void ScalingCalcTyped<T, U>::scaleData(void* data, SizeT sampleCount, void** output)
{
    if (type == ScalingType::Linear)
    {
        scaleLinear(data, sampleCount, output);
        return;
    }

    throw(UnknownRuleTypeException{});
}

template <typename T, typename U>
void* ScalingCalcTyped<T, U>::scaleLinear(void* data, SizeT sampleCount)
{
    auto scaledData = std::malloc(sampleCount * sizeof(U));
    if (!scaledData)
        throw NoMemoryException("Memory allocation failed.");

    this->scaleLinear(data, sampleCount, &scaledData);
    return scaledData;
}

template <typename T, typename U>
void ScalingCalcTyped<T, U>::scaleLinear(void* data, SizeT sampleCount, void** output)
{
    T* rawData = static_cast<T*>(data);
    U* scaledData = static_cast<U*>(*output);
    const U scale = params[0];
    const U offset = params[1];
    for (SizeT i = 0; i < sampleCount; ++i)
        scaledData[i] = scale * static_cast<U>(rawData[i]) + offset;
}

static ScalingCalc* createScalingCalcTyped(const ScalingPtr& scaling)
{
    const auto inputType = scaling.getInputSampleType();
    const auto outputType = scaling.getOutputSampleType();

    if (inputType == SampleType::Float32 && outputType == ScaledSampleType::Float32)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Float32>::Type, SampleTypeToType<SampleType::Float32>::Type>(scaling);
    if (inputType == SampleType::Float64 && outputType == ScaledSampleType::Float32)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Float64>::Type, SampleTypeToType<SampleType::Float32>::Type>(scaling);

    if (inputType == SampleType::UInt8 && outputType == ScaledSampleType::Float32)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::UInt8>::Type, SampleTypeToType<SampleType::Float32>::Type>(scaling);
    if (inputType == SampleType::Int8 && outputType == ScaledSampleType::Float32)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Int8>::Type, SampleTypeToType<SampleType::Float32>::Type>(scaling);

    if (inputType == SampleType::UInt16 && outputType == ScaledSampleType::Float32)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::UInt16>::Type, SampleTypeToType<SampleType::Float32>::Type>(scaling);
    if (inputType == SampleType::Int16 && outputType == ScaledSampleType::Float32)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Int16>::Type, SampleTypeToType<SampleType::Float32>::Type>(scaling);

    if (inputType == SampleType::UInt32 && outputType == ScaledSampleType::Float32)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::UInt32>::Type, SampleTypeToType<SampleType::Float32>::Type>(scaling);
    if (inputType == SampleType::Int32 && outputType == ScaledSampleType::Float32)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Int32>::Type, SampleTypeToType<SampleType::Float32>::Type>(scaling);

    if (inputType == SampleType::UInt64 && outputType == ScaledSampleType::Float32)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::UInt64>::Type, SampleTypeToType<SampleType::Float32>::Type>(scaling);
    if (inputType == SampleType::Int64 && outputType == ScaledSampleType::Float32)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Int64>::Type, SampleTypeToType<SampleType::Float32>::Type>(scaling);

    if (inputType == SampleType::Float32 && outputType == ScaledSampleType::Float64)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Float32>::Type, SampleTypeToType<SampleType::Float64>::Type>(scaling);
    if (inputType == SampleType::Float64 && outputType == ScaledSampleType::Float64)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Float64>::Type, SampleTypeToType<SampleType::Float64>::Type>(scaling);

    if (inputType == SampleType::UInt8 && outputType == ScaledSampleType::Float64)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::UInt8>::Type, SampleTypeToType<SampleType::Float64>::Type>(scaling);
    if (inputType == SampleType::Int8 && outputType == ScaledSampleType::Float64)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Int8>::Type, SampleTypeToType<SampleType::Float64>::Type>(scaling);

    if (inputType == SampleType::UInt16 && outputType == ScaledSampleType::Float64)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::UInt16>::Type, SampleTypeToType<SampleType::Float64>::Type>(scaling);
    if (inputType == SampleType::Int16 && outputType == ScaledSampleType::Float64)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Int16>::Type, SampleTypeToType<SampleType::Float64>::Type>(scaling);

    if (inputType == SampleType::UInt32 && outputType == ScaledSampleType::Float64)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::UInt32>::Type, SampleTypeToType<SampleType::Float64>::Type>(scaling);
    if (inputType == SampleType::Int32 && outputType == ScaledSampleType::Float64)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Int32>::Type, SampleTypeToType<SampleType::Float64>::Type>(scaling);

    if (inputType == SampleType::UInt64 && outputType == ScaledSampleType::Float64)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::UInt64>::Type, SampleTypeToType<SampleType::Float64>::Type>(scaling);
    if (inputType == SampleType::Int64 && outputType == ScaledSampleType::Float64)
        return new ScalingCalcTyped<SampleTypeToType<SampleType::Int64>::Type, SampleTypeToType<SampleType::Float64>::Type>(scaling);

    throw InvalidSampleTypeException{"The scaling input or output type is not supported."};
}
END_NAMESPACE_OPENDAQ
