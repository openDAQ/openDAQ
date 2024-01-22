/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/reader_domain_info.h>
#include <opendaq/sample_reader.h>

BEGIN_NAMESPACE_OPENDAQ

class Comparable;

class Reader
{
public:
    explicit Reader(FunctionPtr transform, SampleType descriptorReadType = SampleType::Undefined);
    virtual ~Reader() = default;

    virtual ErrCode readData(void* inputBuffer, SizeT offset, void** outputBuffer, SizeT count) = 0;
    virtual std::unique_ptr<Comparable> readStart(void* inputBuffer, SizeT offset, const ReaderDomainInfo& domainInfo) = 0;
    
    virtual SizeT getOffsetTo(const ReaderDomainInfo& domainInfo, const Comparable& start, void* inputBuffer, SizeT size) = 0;
    virtual bool handleDescriptorChanged(DataDescriptorPtr& descriptor, ReadMode mode) = 0;

    [[nodiscard]] virtual bool isUndefined() const noexcept;
    [[nodiscard]] virtual SampleType getReadType() const noexcept = 0;

    FunctionPtr getTransformFunction() const;
    void setTransformFunction(FunctionPtr transform);

    void setTransformIgnore(bool ignore);

protected:
    bool ignoreTransform;
    FunctionPtr transformFunction;
    DataDescriptorPtr dataDescriptor;
    SampleType dataSampleType;
};

class UndefinedReader final : public Reader
{
public:
    using Reader::Reader;
    explicit UndefinedReader(FunctionPtr transform):
        Reader(transform, SampleType::Undefined)
    {
    }

    ErrCode readData(void* inputBuffer, SizeT offset, void** outputBuffer, SizeT count) override
    {
        return OPENDAQ_ERR_INVALIDSTATE;
    }

    virtual std::unique_ptr<Comparable> readStart(void* inputBuffer, SizeT offset, const ReaderDomainInfo& domainInfo) override
    {
        throw InvalidStateException();
    }

    SizeT getOffsetTo(const ReaderDomainInfo& domainInfo, const Comparable& start, void* inputBuffer, SizeT size) override
    {
        throw InvalidStateException();
    }

    virtual bool handleDescriptorChanged(DataDescriptorPtr& descriptor, ReadMode mode) override
    {
        return false;
    }

    [[nodiscard]]
    bool isUndefined() const noexcept override
    {
        return true;
    }

    [[nodiscard]]
    SampleType getReadType() const noexcept override
    {
        return SampleType::Invalid;
    }
};

template <typename ReadType>
class TypedReader : public Reader
{
public:
    using Reader::Reader;

    virtual ErrCode readData(void* inputBuffer, SizeT offset, void** outputBuffer, SizeT count) override;
    virtual std::unique_ptr<Comparable> readStart(void* inputBuffer, SizeT offset, const ReaderDomainInfo& domainInfo) override;

    virtual SizeT getOffsetTo(const ReaderDomainInfo& domainInfo, const Comparable& start, void* inputBuffer, SizeT size) override;

    virtual bool handleDescriptorChanged(DataDescriptorPtr& descriptor, ReadMode mode) override;

    virtual SampleType getReadType() const noexcept override;
private:
    template <typename TDataType>
    ErrCode readValues(void* inputBuffer, SizeT offset, void** outputBuffer, SizeT toRead) const;

    template <typename TDataType>
    SizeT getOffsetToData(const ReaderDomainInfo& domainInfo, const Comparable& start, void* inputBuffer, SizeT size) const;

    SizeT valuesPerSample{1};
};

std::unique_ptr<Reader> createReaderForType(SampleType readType, const FunctionPtr& transformFunction, SampleType descriptorReadType = SampleType::Undefined);

extern template class TypedReader<SampleTypeToType<SampleType::Float32>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::Float64>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::UInt8>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::Int8>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::UInt16>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::Int16>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::UInt32>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::Int32>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::Int64>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::RangeInt64>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::ComplexFloat32>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::ComplexFloat64>::Type>;

extern template class TypedReader<SampleTypeToType<SampleType::Float32>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::Float64>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::UInt8>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::Int8>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::UInt16>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::Int16>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::UInt32>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::Int32>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::Int64>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::RangeInt64>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::ComplexFloat32>::Type>;
extern template class TypedReader<SampleTypeToType<SampleType::ComplexFloat64>::Type>;

END_NAMESPACE_OPENDAQ
