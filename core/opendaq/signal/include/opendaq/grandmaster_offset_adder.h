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
#include <opendaq/sample_type_traits.h>
#include <opendaq/signal_exceptions.h>
#include "coretypes/integer_factory.h"

BEGIN_NAMESPACE_OPENDAQ

class GrandmasterOffsetAdder
{
public:
    virtual ~GrandmasterOffsetAdder() = default;
    virtual void addGrandmasterOffset(void** input) = 0;
};

template <typename T>
class GrandmasterOffsetAdderTyped : public GrandmasterOffsetAdder
{
public:
    void addGrandmasterOffset(void** input) override
    {
        T* typed = static_cast<T*>(*input);
        for (SizeT i = 0; i < sampleCount; ++i)
            typed[i] = typed[i] + grandmasterOffset;
    }

    GrandmasterOffsetAdderTyped(const IntegerPtr& grandmasterOffset, SizeT sampleCount)
    {
        this->grandmasterOffset = static_cast<T>(grandmasterOffset);
        this->sampleCount = sampleCount;
    }

private:
    T grandmasterOffset;
    SizeT sampleCount;
};

static GrandmasterOffsetAdder* createGrandmasterOffsetTyped(SampleType outputType, const IntegerPtr& grandmasterOffset, SizeT sampleCount)
{
    switch (outputType)
    {
        case SampleType::Float32:
            return new GrandmasterOffsetAdderTyped<SampleTypeToType<SampleType::Float32>::Type>(grandmasterOffset, sampleCount);
        case SampleType::Float64:
            return new GrandmasterOffsetAdderTyped<SampleTypeToType<SampleType::Float64>::Type>(grandmasterOffset, sampleCount);
        case SampleType::UInt8:
            return new GrandmasterOffsetAdderTyped<SampleTypeToType<SampleType::UInt8>::Type>(grandmasterOffset, sampleCount);
        case SampleType::Int8:
            return new GrandmasterOffsetAdderTyped<SampleTypeToType<SampleType::Int8>::Type>(grandmasterOffset, sampleCount);
        case SampleType::UInt16:
            return new GrandmasterOffsetAdderTyped<SampleTypeToType<SampleType::UInt16>::Type>(grandmasterOffset, sampleCount);
        case SampleType::Int16:
            return new GrandmasterOffsetAdderTyped<SampleTypeToType<SampleType::Int16>::Type>(grandmasterOffset, sampleCount);
        case SampleType::UInt32:
            return new GrandmasterOffsetAdderTyped<SampleTypeToType<SampleType::UInt32>::Type>(grandmasterOffset, sampleCount);
        case SampleType::Int32:
            return new GrandmasterOffsetAdderTyped<SampleTypeToType<SampleType::Int32>::Type>(grandmasterOffset, sampleCount);
        case SampleType::UInt64:
            return new GrandmasterOffsetAdderTyped<SampleTypeToType<SampleType::UInt64>::Type>(grandmasterOffset, sampleCount);
        case SampleType::Int64:
            return new GrandmasterOffsetAdderTyped<SampleTypeToType<SampleType::Int64>::Type>(grandmasterOffset, sampleCount);
        case SampleType::RangeInt64:
        case SampleType::Binary:
        case SampleType::ComplexFloat32:
        case SampleType::ComplexFloat64:
        case SampleType::Invalid:
        case SampleType::String:
        case SampleType::Struct:
        case SampleType::_count:
            break;
    }

    throw InvalidSampleTypeException{"Grandmaster Offset: Output type is not supported."};
}

END_NAMESPACE_OPENDAQ
