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
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/stream_reader_ptr.h>
#include <opendaq/sample_type_traits.h>
#include <date/date.h>
#include <opendaq/signal_exceptions.h>
#include <opendaq/tail_reader_ptr.h>
#include <opendaq/block_reader_ptr.h>
#include <opendaq/multi_reader_ptr.h>
#include <opendaq/reader_utils.h>
#include <opendaq/reader_status.h>

#include <utility>

BEGIN_NAMESPACE_OPENDAQ

class TimeReaderBase
{
protected:
    template <typename ReadType>
    void readSamples(ReadType* input, std::chrono::system_clock::time_point* output, SizeT samples) const;

    void readData(void* inputBuffer, std::chrono::system_clock::time_point* output, SizeT count) const;

    void handleDescriptorChanged(const DataDescriptorPtr& descriptor);
    bool transform(void* inputBuff, std::chrono::system_clock::time_point* outputBuff, SizeT toRead, const DataDescriptorPtr& descriptor);

    DataDescriptorPtr domainDataDescriptor;

    RatioPtr resolution;
    SampleType domainSampleType{};
    std::chrono::system_clock::time_point parsedEpoch{};
};

template <typename WrappedReaderPtr>
struct ReaderStatusType
{
    using Type = daq::ReaderStatusPtr;
    using IType = daq::IReaderStatus;
};

template<>
struct ReaderStatusType<BlockReaderPtr>
{
    using Type = daq::BlockReaderStatusPtr;
    using IType = daq::IBlockReaderStatus;
};

template<>
struct ReaderStatusType<TailReaderPtr>
{
    using Type = daq::TailReaderStatusPtr;
    using IType = daq::ITailReaderStatus;
};

template <>
struct ReaderStatusType<MultiReaderPtr>
{
    using Type = daq::MultiReaderStatusPtr;
    using IType = daq::IMultiReaderStatus;
};

/*!
 * @brief A reader wrapper that can convert domain clock-ticks to time-points when the domain is time.
 */
template <typename WrappedReaderPtr>
class TimeReader final : public WrappedReaderPtr
                       , public TimeReaderBase
{
public:
    using IReaderStatusType = typename ReaderStatusType<WrappedReaderPtr>::IType;

    explicit TimeReader(const WrappedReaderPtr& reader)
        : WrappedReaderPtr(reader)
    {
        this->setDomainTransformFunction([this](Int inputBuffer, Int outputBuffer, SizeT toRead, DataDescriptorPtr descriptor)
            {
                return transform(
                    reinterpret_cast<void*>(inputBuffer),
                    reinterpret_cast<std::chrono::system_clock::time_point*>(outputBuffer),
                    toRead,
                    std::move(descriptor));
            });
    }

    explicit TimeReader(typename WrappedReaderPtr::DeclaredInterface* reader)
        : TimeReader(WrappedReaderPtr(reader))
    {
    }

    ~TimeReader()
    {
        if (this->assigned())
            this->setDomainTransformFunction(nullptr);
    }

    /*!
     * @brief Copies at maximum the next `count` unread samples and time-points to the `values` and `domain` buffers.
     * The amount actually read is returned through the `count` parameter.
     * @param[in] values The buffer that the segments will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` amount of segments.
     * @param[in] domain The buffer that the time-points will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` amount of time-point segments.
     * @param[in,out] count The maximum amount of segments to be read. If the `count` is less than
     * available the parameter value is set to the actual amount and only the available
     * segments are returned. The rest of the buffer is not modified or cleared.
     * @param timeoutMs The maximum amount of time in milliseconds to wait for the requested amount of samples before returning.
     */
    void readWithDomain(void* values, std::chrono::system_clock::time_point* domain, daq::SizeT* count, daq::SizeT timeoutMs = 0, IReaderStatusType** status = nullptr) const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        auto errCode = this->object->readWithDomain(values, domain, count, timeoutMs, status);
        daq::checkErrorInfo(errCode);
    }
};

/*!
 * @brief A reader wrapper that can convert domain clock-ticks to time-points when the domain is time.
 */
template <>
class TimeReader<TailReaderPtr> final : public TailReaderPtr
                                      , public TimeReaderBase
{
public:
    using IReaderStatusType = ITailReaderStatus;

    explicit TimeReader(TailReaderPtr reader)
        : TailReaderPtr(std::move(reader))
    {
        this->setDomainTransformFunction([this](Int inputBuffer, Int outputBuffer, SizeT toRead, DataDescriptorPtr descriptor)
        {
            return transform(
                reinterpret_cast<void*>(inputBuffer),
                reinterpret_cast<std::chrono::system_clock::time_point*>(outputBuffer),
                toRead,
                std::move(descriptor)
            );
        });
    }

    ~TimeReader() override
    {
        if (this->assigned())
            this->setDomainTransformFunction(nullptr);
    }

    /*!
     * @brief Copies at maximum the next `count` unread samples and time-points to the `values` and `stamps` buffers.
     * The amount actually read is returned through the `count` parameter.
     * @param[in] values The buffer that the data values will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` amount of samples.
     * @param[in] domain The buffer that the time-points will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` amount of time-points.
     * @param[in,out] count The maximum amount of samples to be read. If the `count` is less than
     * available the parameter value is set to the actual amount and only the available
     * samples are returned. The rest of the buffer is not modified or cleared.
     */
    void readWithDomain(void* values, std::chrono::system_clock::time_point* domain, daq::SizeT* count, IReaderStatusType** status = nullptr) const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        auto errCode = this->object->readWithDomain(values, domain, count, status);
        daq::checkErrorInfo(errCode);
    }
};

template<typename WrappedReaderPtr>
struct ReaderStatusType<TimeReader<WrappedReaderPtr>>
{
    using Type = typename ReaderStatusType<WrappedReaderPtr>::Type;
    using IType = typename ReaderStatusType<WrappedReaderPtr>::IType;
};

template <typename ReadType>
void TimeReaderBase::readSamples(ReadType* input, std::chrono::system_clock::time_point* output, SizeT samples) const
{
    using namespace std::chrono;
    using Seconds = duration<double>;

    for (SizeT i = 0; i < samples; ++i)
    {
        auto offset = Seconds((resolution.getNumerator() * input[i]) / static_cast<double>(resolution.getDenominator()));
        output[i] = round<system_clock::duration>(parsedEpoch + offset);
    }
}

template <>
inline void TimeReaderBase::readSamples<ClockRange>(ClockRange* input, std::chrono::system_clock::time_point* output, SizeT samples) const
{
    using namespace std::chrono;
    using Seconds = duration<double>;

    for (SizeT i = 0; i < samples; ++i)
    {
        output[i] = round<system_clock::duration>(parsedEpoch + Seconds(resolution * input[i].start));
    }
}

inline void TimeReaderBase::handleDescriptorChanged(const DataDescriptorPtr& descriptor)
{
    auto unit = descriptor.getUnit();
    auto domainQuantity = unit.getQuantity();
    if (domainQuantity != "time")
    {
        DAQ_THROW_EXCEPTION(InvalidSampleTypeException, R"(Domain quantity is not "time" but "{}".)", domainQuantity);
    }

    auto timeUnit = unit.getSymbol();
    if (timeUnit != "s")
    {
        DAQ_THROW_EXCEPTION(NotSupportedException, R"(Only seconds are supported as a time-unit but encountered '{}'.)", timeUnit);
    }

    std::istringstream epochString(reader::fixupIso8601(descriptor.getOrigin()));
    epochString >> date::parse("%FT%T%z", parsedEpoch);

    domainDataDescriptor = descriptor;
    resolution = descriptor.getTickResolution();
    domainSampleType = descriptor.getSampleType();
}

inline bool TimeReaderBase::transform(void* inputBuff,
                                      std::chrono::system_clock::time_point* outputBuff,
                                      SizeT toRead,
                                      const DataDescriptorPtr& descriptor)
{
    if (descriptor != domainDataDescriptor)
    {
        handleDescriptorChanged(descriptor);
    }

    readData(inputBuff, outputBuff, toRead);
    return true;
}

inline void TimeReaderBase::readData(void* inputBuffer, std::chrono::system_clock::time_point* output, SizeT count) const
{
    switch (domainSampleType)
    {
        case SampleType::Float32:
            return readSamples(static_cast<SampleTypeToType<SampleType::Float32>::Type*>(inputBuffer), output, count);
        case SampleType::Float64:
            return readSamples(static_cast<SampleTypeToType<SampleType::Float64>::Type*>(inputBuffer), output, count);
        case SampleType::UInt8:
            return readSamples(static_cast<SampleTypeToType<SampleType::UInt8>::Type*>(inputBuffer), output, count);
        case SampleType::Int8:
            return readSamples(static_cast<SampleTypeToType<SampleType::Int8>::Type*>(inputBuffer), output, count);
        case SampleType::Int16:
            return readSamples(static_cast<SampleTypeToType<SampleType::Int16>::Type*>(inputBuffer), output, count);
        case SampleType::UInt16:
            return readSamples(static_cast<SampleTypeToType<SampleType::UInt16>::Type*>(inputBuffer), output, count);
        case SampleType::Int32:
            return readSamples(static_cast<SampleTypeToType<SampleType::Int32>::Type*>(inputBuffer), output, count);
        case SampleType::UInt32:
            return readSamples(static_cast<SampleTypeToType<SampleType::UInt32>::Type*>(inputBuffer), output, count);
        case SampleType::Int64:
            return readSamples(static_cast<SampleTypeToType<SampleType::Int64>::Type*>(inputBuffer), output, count);
        case SampleType::UInt64:
            return readSamples(static_cast<SampleTypeToType<SampleType::UInt64>::Type*>(inputBuffer), output, count);
        case SampleType::RangeInt64:
            return readSamples(static_cast<SampleTypeToType<SampleType::RangeInt64>::Type*>(inputBuffer), output, count);
        case SampleType::ComplexFloat32:
        case SampleType::ComplexFloat64:
            DAQ_THROW_EXCEPTION(NotSupportedException, "Complex values as time domain are not supported.");
        case SampleType::Binary:
        case SampleType::Struct:
        case SampleType::String:
            DAQ_THROW_EXCEPTION(NotSupportedException, "Struct, string or binary values as time domain are not supported.");
        case SampleType::Invalid:
            DAQ_THROW_EXCEPTION(InvalidStateException, "Unknown raw data-type, conversion not possible.");
        case SampleType::Null:
            DAQ_THROW_EXCEPTION(InvalidSampleTypeException, "Packet with Null sample-type samples encountered");
        case SampleType::_count:
            break;
    }

    DAQ_THROW_EXCEPTION(InvalidSampleTypeException, "Packet with invalid sample-type samples encountered");
}

END_NAMESPACE_OPENDAQ
