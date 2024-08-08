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

BEGIN_NAMESPACE_OPENDAQ

class ReferenceDomainOffsetAdder
{
public:
    virtual ~ReferenceDomainOffsetAdder() = default;
    virtual void* addReferenceDomainOffset(void* input) = 0;
    virtual void addReferenceDomainOffset(void** input) = 0;
};

template <typename T>
class ReferenceDomainOffsetAdderTyped : public ReferenceDomainOffsetAdder
{
public:
    void* addReferenceDomainOffset(void* input) override
    {
        void* output = std::malloc(sampleCount * sizeof(T));
        if (!output)
            throw NoMemoryException("Memory allocation failed.");

        T* typedOutput = static_cast<T*>(output);
        T* typedInput = static_cast<T*>(input);
        for (SizeT i = 0; i < sampleCount; ++i)
            typedOutput[i] = typedInput[i] + referenceDomainOffset;

        return output;
    }

    void addReferenceDomainOffset(void** input) override
    {
        T* typed = static_cast<T*>(*input);
        for (SizeT i = 0; i < sampleCount; ++i)
            typed[i] = typed[i] + referenceDomainOffset;
    }

    ReferenceDomainOffsetAdderTyped(const IntegerPtr& referenceDomainOffset, SizeT sampleCount)
    {
        this->referenceDomainOffset = static_cast<T>(referenceDomainOffset);
        this->sampleCount = sampleCount;
    }

private:
    T referenceDomainOffset;
    SizeT sampleCount;
};

static ReferenceDomainOffsetAdder* createReferenceDomainOffsetAdderTyped(SampleType outputType,
                                                                         const IntegerPtr& referenceDomainOffset,
                                                                         SizeT sampleCount)
{
    switch (outputType)
    {
        case SampleType::UInt8:
            return new ReferenceDomainOffsetAdderTyped<SampleTypeToType<SampleType::UInt8>::Type>(referenceDomainOffset, sampleCount);
        case SampleType::Int8:
            return new ReferenceDomainOffsetAdderTyped<SampleTypeToType<SampleType::Int8>::Type>(referenceDomainOffset, sampleCount);
        case SampleType::UInt16:
            return new ReferenceDomainOffsetAdderTyped<SampleTypeToType<SampleType::UInt16>::Type>(referenceDomainOffset, sampleCount);
        case SampleType::Int16:
            return new ReferenceDomainOffsetAdderTyped<SampleTypeToType<SampleType::Int16>::Type>(referenceDomainOffset, sampleCount);
        case SampleType::UInt32:
            return new ReferenceDomainOffsetAdderTyped<SampleTypeToType<SampleType::UInt32>::Type>(referenceDomainOffset, sampleCount);
        case SampleType::Int32:
            return new ReferenceDomainOffsetAdderTyped<SampleTypeToType<SampleType::Int32>::Type>(referenceDomainOffset, sampleCount);
        case SampleType::UInt64:
            return new ReferenceDomainOffsetAdderTyped<SampleTypeToType<SampleType::UInt64>::Type>(referenceDomainOffset, sampleCount);
        case SampleType::Int64:
            return new ReferenceDomainOffsetAdderTyped<SampleTypeToType<SampleType::Int64>::Type>(referenceDomainOffset, sampleCount);
        case SampleType::Float32:
        case SampleType::Float64:
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

    throw InvalidSampleTypeException{"Reference Domain Offset: Output type is not supported."};
}

END_NAMESPACE_OPENDAQ
