#include <opendaq/typed_reader.h>
#include <opendaq/sample_type.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_errors.h>
#include <opendaq/signal_errors.h>
#include <opendaq/multi_typed_reader.h>

#include <utility>

BEGIN_NAMESPACE_OPENDAQ

template <typename T, typename = std::void_t<>>
struct GreaterEqual
{
    static T Multiply(T& value, const RatioPtr& multiplier)
    {
        return value * multiplier.getNumerator() / multiplier.getDenominator();
    }

    static T Adjust(T value, const RatioPtr& multiplier)
    {
        return Multiply(value, multiplier);
    }

    static T GetStart(T startValue, std::int64_t offset)
    {
        return startValue + offset;
    }

    static bool Check(const RatioPtr& multiplier, T readValue, T startValue)
    {
        return Multiply(readValue, multiplier) >= startValue;
    }
};

template <typename T>
struct GreaterEqual<T, typename std::enable_if_t<daq::IsTemplateOf<T, daq::RangeType>::value>>
{
    using RangeValue = typename T::Type;

    static RangeValue Multiply(RangeValue value, const RatioPtr& multiplier)
    {
        return value * multiplier.getNumerator() / multiplier.getDenominator();
    }

    static RangeValue Adjust(T value, const RatioPtr& multiplier)
    {
        return Multiply(value.start, multiplier);
    }

    static T GetStart(T startValue, std::int64_t offset)
    {
        RangeValue start = startValue.start + offset;
        RangeValue end = -1;
        if (startValue.end != -1)
        {
            end = startValue.end + offset;
        }

        return T(start, end);
    }

    static bool Check(const RatioPtr& multiplier, T readValue, T startValue)
    {
        return Multiply(readValue.start, multiplier) >= startValue.start;
    }
};

template <typename T>
struct GreaterEqual<T, typename std::enable_if_t<daq::IsTemplateOf<T, daq::Complex_Number>::value>>
{
    static T Multiply(T value, const RatioPtr& multiplier)
    {
        throw NotSupportedException();
    }

    static T Adjust(T value, const RatioPtr& multiplier)
    {
        return Multiply(value, multiplier);
    }

    static T GetStart(T startValue, std::int64_t offset)
    {
        throw NotSupportedException();
    }

    static constexpr bool Check(const RatioPtr& multiplier, T readValue, T startValue)
    {
        return Multiply(readValue, multiplier) >= startValue;
    }
};

template <typename TReadType>
static bool isSampleTypeConvertibleTo(SampleType sampleType)
{
    switch (sampleType)
    {
        case SampleType::Float32:
            return std::is_convertible_v<SampleTypeToType<SampleType::Float32>::Type, TReadType>;
        case SampleType::Float64:
            return std::is_convertible_v<SampleTypeToType<SampleType::Float64>::Type, TReadType>;
        case SampleType::UInt8:
            return std::is_convertible_v<SampleTypeToType<SampleType::UInt8>::Type, TReadType>;
        case SampleType::Int8:
            return std::is_convertible_v<SampleTypeToType<SampleType::Int8>::Type, TReadType>;
        case SampleType::Int16:
            return std::is_convertible_v<SampleTypeToType<SampleType::Int16>::Type, TReadType>;
        case SampleType::UInt16:
            return std::is_convertible_v<SampleTypeToType<SampleType::UInt16>::Type, TReadType>;
        case SampleType::Int32:
            return std::is_convertible_v<SampleTypeToType<SampleType::Int32>::Type, TReadType>;
        case SampleType::UInt32:
            return std::is_convertible_v<SampleTypeToType<SampleType::UInt32>::Type, TReadType>;
        case SampleType::Int64:
            return std::is_convertible_v<SampleTypeToType<SampleType::Int64>::Type, TReadType>;
        case SampleType::UInt64:
            return std::is_convertible_v<SampleTypeToType<SampleType::UInt64>::Type, TReadType>;
        case SampleType::RangeInt64:
            if constexpr (std::is_same_v<TReadType, ClockTick>)
            {
                return true;
            }
            return std::is_convertible_v<SampleTypeToType<SampleType::RangeInt64>::Type, TReadType>;
        case SampleType::ComplexFloat32:
            return std::is_convertible_v<SampleTypeToType<SampleType::ComplexFloat32>::Type, TReadType>;
        case SampleType::ComplexFloat64:
            return std::is_convertible_v<SampleTypeToType<SampleType::ComplexFloat64>::Type, TReadType>;
        case SampleType::Binary:
            return std::is_convertible_v<SampleTypeToType<SampleType::Binary>::Type, TReadType>;
        case SampleType::String:
            return std::is_convertible_v<SampleTypeToType<SampleType::String>::Type, TReadType>;
        case SampleType::Struct:
        case SampleType::Invalid:
        case SampleType::_count:
            break;
    }

    return false;
}

template <typename ReadType>
std::unique_ptr<Comparable> TypedReader<ReadType>::readStart(void* inputBuffer, SizeT offset, const ReaderDomainInfo& domainInfo)
{
    if constexpr (std::is_same_v<void*, ReadType>)
    {
        // void reader should never be used to read domain info
        return {};
    }
    else
    {
        ReadType startDomain{};
        void* data = &startDomain;

        setTransformIgnore(true);
        readData(inputBuffer, offset, &data, 1);
        setTransformIgnore(false);

        return std::make_unique<ComparableValue<ReadType>>(startDomain, domainInfo);
    }
}

template <typename ReadType>
ErrCode TypedReader<ReadType>::readData(void* inputBuffer, SizeT offset, void** outputBuffer, SizeT count)
{
    switch (dataSampleType)
    {
        case SampleType::Float32:
            return readValues<SampleTypeToType<SampleType::Float32>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::Float64:
            return readValues<SampleTypeToType<SampleType::Float64>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::UInt8:
            return readValues<SampleTypeToType<SampleType::UInt8>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::Int8:
            return readValues<SampleTypeToType<SampleType::Int8>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::Int16:
            return readValues<SampleTypeToType<SampleType::Int16>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::UInt16:
            return readValues<SampleTypeToType<SampleType::UInt16>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::Int32:
            return readValues<SampleTypeToType<SampleType::Int32>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::UInt32:
            return readValues<SampleTypeToType<SampleType::UInt32>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::Int64:
            return readValues<SampleTypeToType<SampleType::Int64>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::UInt64:
            return readValues<SampleTypeToType<SampleType::UInt64>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::RangeInt64:
            return readValues<SampleTypeToType<SampleType::RangeInt64>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::ComplexFloat32:
            return readValues<SampleTypeToType<SampleType::ComplexFloat32>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::ComplexFloat64:
            return readValues<SampleTypeToType<SampleType::ComplexFloat64>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::Binary:
        case SampleType::String:
            return readValues<SampleTypeToType<SampleType::String>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::Struct:
            return readValues<SampleTypeToType<SampleType::Struct>::Type>(inputBuffer, offset, outputBuffer, count);
        case SampleType::Invalid:
            return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Unknown raw data-type, conversion not possible.", nullptr);
        case SampleType::_count:
            break;
    }

    return makeErrorInfo(OPENDAQ_ERR_INVALID_SAMPLE_TYPE, "Packet with invalid sample-type samples encountered", nullptr);
}

template <typename ReadType>
SizeT TypedReader<ReadType>::getOffsetTo(const ReaderDomainInfo& domainInfo,
                                         const Comparable& start,
                                         void* inputBuffer,
                                         SizeT size)
{
    switch (dataSampleType)
    {
        case SampleType::Float32:
            return getOffsetToData<SampleTypeToType<SampleType::Float32>::Type>(domainInfo, start, inputBuffer, size);
        case SampleType::Float64:
            return getOffsetToData<SampleTypeToType<SampleType::Float64>::Type>(domainInfo, start, inputBuffer, size);
        case SampleType::UInt8:
            return getOffsetToData<SampleTypeToType<SampleType::UInt8>::Type>(domainInfo, start, inputBuffer, size);
        case SampleType::Int8:
            return getOffsetToData<SampleTypeToType<SampleType::Int8>::Type>(domainInfo, start, inputBuffer, size);
        case SampleType::Int16:
            return getOffsetToData<SampleTypeToType<SampleType::Int16>::Type>(domainInfo, start, inputBuffer, size);
        case SampleType::UInt16:
            return getOffsetToData<SampleTypeToType<SampleType::UInt16>::Type>(domainInfo, start, inputBuffer, size);
        case SampleType::Int32:
            return getOffsetToData<SampleTypeToType<SampleType::Int32>::Type>(domainInfo, start, inputBuffer, size);
        case SampleType::UInt32:
            return getOffsetToData<SampleTypeToType<SampleType::UInt32>::Type>(domainInfo, start, inputBuffer, size);
        case SampleType::Int64:
            return getOffsetToData<SampleTypeToType<SampleType::Int64>::Type>(domainInfo, start, inputBuffer, size);
        case SampleType::UInt64:
            return getOffsetToData<SampleTypeToType<SampleType::UInt64>::Type>(domainInfo, start, inputBuffer, size);
        case SampleType::RangeInt64:
            return getOffsetToData<SampleTypeToType<SampleType::RangeInt64>::Type>(domainInfo, start, inputBuffer, size);
        case SampleType::ComplexFloat32:
        case SampleType::ComplexFloat64:
        case SampleType::Binary:
        case SampleType::String:
        case SampleType::Struct:
            return makeErrorInfo(
                OPENDAQ_ERR_NOT_SUPPORTED,
                fmt::format("Using the SampleType {} as a domain is not supported", dataSampleType),
                nullptr
            );
        case SampleType::Invalid:
            return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Unknown raw data-type, conversion not possible.", nullptr);
        case SampleType::_count:
            break;
    }

    return makeErrorInfo(OPENDAQ_ERR_INVALID_SAMPLE_TYPE, "Packet with invalid sample-type samples encountered", nullptr);
}

template <typename TReadType>
template <typename TDataType>
SizeT TypedReader<TReadType>::getOffsetToData(const ReaderDomainInfo& domainInfo,
                                              const Comparable& start,
                                              void* inputBuffer,
                                              SizeT size) const
{
    if (!inputBuffer)
        throw ArgumentNullException{};
        
    using namespace reader;

    if constexpr (std::is_convertible_v<TDataType, TReadType>)
    {
        auto* dataStart = static_cast<TDataType*>(inputBuffer);

        auto* startV = dynamic_cast<const ComparableValue<TReadType>*>(&start);
        // Should always be non-negative
        auto startValue = GreaterEqual<TReadType>::GetStart(startV->getValue(), -domainInfo.offset);

        // std::stringstream ss11;
        // ss11 << toSysTime(startValue, domainInfo.epoch, domainInfo.readResolution);
        // std::string s11 = ss11.str();
        //
        // std::stringstream eps;
        // eps << domainInfo.epoch;
        // std::string epoch = eps.str();
        //
        //
        // [[maybe_unused]]
        // int a = 5;

        for (std::size_t i = 0; i < size * valuesPerSample; ++i)
        {
            // debug
            // [[maybe_unused]] auto packetValue = dataStart[i];
            // [[maybe_unused]] auto readValue = static_cast<TReadType>(packetValue);
            // [[maybe_unused]] auto adjusted = GreaterEqual<TReadType>::Adjust(readValue, domainInfo.multiplier);
            //
            // std::stringstream ss1;
            // ss1 << toSysTime(adjusted, domainInfo.epoch, domainInfo.readResolution);
            // std::string s1 = ss1.str();

            if (GreaterEqual<TReadType>::Check(domainInfo.multiplier, static_cast<TReadType>(dataStart[i]), startValue))
            {
                return i / valuesPerSample;
            }
        }
        // debug
        // [[maybe_unused]] auto packetValue = dataStart[size - 1];
        // [[maybe_unused]] auto readValue = static_cast<TReadType>(packetValue);
        // [[maybe_unused]] auto adjusted = GreaterEqual<TReadType>::Adjust(readValue, domainInfo.multiplier);

        return static_cast<SizeT>(-1);
    }
    else
    {
        return makeErrorInfo(
            OPENDAQ_ERR_NOT_SUPPORTED,
            "Implicit conversion from packet data-type to the read data-type is not supported.",
            nullptr
        );
    }
}

template <typename TReadType>
template <typename TDataType>
ErrCode TypedReader<TReadType>::readValues(void* inputBuffer, SizeT offset, void** outputBuffer, SizeT toRead) const
{
    if (!inputBuffer || !outputBuffer)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    if constexpr (std::is_same_v<TReadType, void*>)
    {
        if (!ignoreTransform && transformFunction.assigned())
            return makeErrorInfo(OPENDAQ_ERR_NOT_SUPPORTED, "Transform function for void reader not supported.", nullptr);

        const auto dataStart = static_cast<void*>(static_cast<uint8_t*>(inputBuffer) + offset * rawSampleSize);
        const auto toReadInBytes = rawSampleSize * toRead;
        const auto dataOut = *outputBuffer;
        std::memcpy(dataOut, dataStart, toReadInBytes);
        *outputBuffer = static_cast<void*>(static_cast<uint8_t*>(dataOut) + toReadInBytes);
        return OPENDAQ_SUCCESS;
    }
    else if constexpr (std::is_convertible_v<TDataType, TReadType>)
    {
        auto dataStart = static_cast<TDataType*>(inputBuffer) + (offset * valuesPerSample);
        auto dataOut = static_cast<TReadType*>(*outputBuffer);

        if (!ignoreTransform && transformFunction.assigned())
        {
            transformFunction.call((Int) dataStart, (Int) dataOut, toRead, dataDescriptor);

            *outputBuffer = dataOut + (valuesPerSample * toRead);
            return OPENDAQ_SUCCESS;
        }

        // If the type of samples is the same, then just copy
        if (std::is_same_v<TReadType, TDataType>)
        {
            // Returns the pointer to the value after the last copied one
            *outputBuffer = std::copy_n(dataStart, valuesPerSample * toRead, dataOut);
        }
        else
        {
            for (std::size_t i = 0; i < toRead * valuesPerSample; ++i)
            {
                dataOut[i] = (TReadType) dataStart[i];
            }

            // Set the pointer to the value after the last copied one
            *outputBuffer = &dataOut[toRead];
        }

        return OPENDAQ_SUCCESS;
    }
    else
    {
        return makeErrorInfo(
            OPENDAQ_ERR_NOT_SUPPORTED,
            "Implicit conversion from packet data-type to the read data-type is not supported.",
            nullptr
        );
    }
}

template <>
template <>
ErrCode TypedReader<ClockTick>::readValues<ClockRange>(void* inputBuffer, SizeT offset, void** outputBuffer, SizeT toRead) const
{
    auto dataStart = static_cast<ClockRange*>(inputBuffer) + (offset * valuesPerSample);
    auto dataOut = static_cast<ClockTick*>(*outputBuffer);

    if (!ignoreTransform && transformFunction.assigned())
    {
        transformFunction.call((Int) dataStart, (Int) dataOut, toRead, dataDescriptor);

        *outputBuffer = dataOut + (valuesPerSample * toRead);
        return OPENDAQ_SUCCESS;
    }

    for (std::size_t i = 0; i < toRead * valuesPerSample; ++i)
    {
        dataOut[i] = dataStart[i].start;
    }

    // Set the pointer to the value after the last copied one
    *outputBuffer = &dataOut[toRead];
    return OPENDAQ_SUCCESS;
}

template <typename ReadType>
bool TypedReader<ReadType>::handleDescriptorChanged(DataDescriptorPtr& descriptor, ReadMode readMode)
{
    if (!descriptor.assigned())
    {
        descriptor = dataDescriptor;
    }

    bool valid = false;

    if (descriptor.assigned())
    {
        const auto postScaling = descriptor.getPostScaling();
        if (!postScaling.assigned() || readMode == ReadMode::Scaled)
        {
            dataSampleType = descriptor.getSampleType();
        }
        else
        {
            dataSampleType = postScaling.getInputSampleType();
        }

        if constexpr (std::is_same_v<ReadType, void*>)
        {
            valid = true;
        }
        else
        {
            valid = isSampleTypeConvertibleTo<ReadType>(dataSampleType);
        }

        rawSampleSize = descriptor.getRawSampleSize();
        auto dimensions = descriptor.getDimensions();
        if (dimensions.assigned() && dimensions.getCount() == 1)
        {
            valuesPerSample = dimensions[0].getSize();
        }

        dataDescriptor = descriptor;
    }

    return valid;
}

template <typename ReadType>
SampleType TypedReader<ReadType>::getReadType() const noexcept
{
    return SampleTypeFromType<ReadType>::SampleType;
}

////
// Reader
////

Reader::Reader(FunctionPtr transform)
    : ignoreTransform(false)
    , transformFunction(std::move(transform))
{
}

bool Reader::isUndefined() const noexcept
{
    return false;
}

FunctionPtr Reader::getTransformFunction() const
{
    return transformFunction;
}

void Reader::setTransformFunction(FunctionPtr transform)
{
    transformFunction = std::move(transform);
}

void Reader::setTransformIgnore(bool ignore)
{
    ignoreTransform = ignore;
}

std::unique_ptr<Reader> createReaderForType(SampleType readType, const FunctionPtr& transformFunction)
{
    switch (readType)
    {
        case SampleType::Float32:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::Float32>::Type>>(transformFunction);
        case SampleType::Float64:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::Float64>::Type>>(transformFunction);
        case SampleType::UInt8:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::UInt8>::Type>>(transformFunction);
        case SampleType::Int8:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::Int8>::Type>>(transformFunction);
        case SampleType::UInt16:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::UInt16>::Type>>(transformFunction);
        case SampleType::Int16:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::Int16>::Type>>(transformFunction);
        case SampleType::UInt32:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::UInt32>::Type>>(transformFunction);
        case SampleType::Int32:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::Int32>::Type>>(transformFunction);
        case SampleType::UInt64:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::UInt64>::Type>>(transformFunction);
        case SampleType::Int64:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::Int64>::Type>>(transformFunction);
        case SampleType::RangeInt64:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::RangeInt64>::Type>>(transformFunction);
        case SampleType::ComplexFloat32:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::ComplexFloat32>::Type>>(transformFunction);
        case SampleType::ComplexFloat64:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::ComplexFloat64>::Type>>(transformFunction);
        case SampleType::Struct:
            return std::make_unique<TypedReader<SampleTypeToType<SampleType::Struct>::Type>>(transformFunction);
        case SampleType::Undefined:
            return std::make_unique<UndefinedReader>(transformFunction);
        case SampleType::Binary:
        case SampleType::String:
        case SampleType::_count:
            break;
    }
    throw NotSupportedException("The requested sample-type is unsupported or invalid.");
}

std::string_view format_as(SampleType sampleType)
{
    switch (sampleType)
    {
        case SampleType::Float32:
            return "Float32";
        case SampleType::Float64:
            return "Float64";
        case SampleType::UInt8:
            return "UInt8";
        case SampleType::Int8:
            return "Int8";
        case SampleType::UInt16:
            return "UInt16";
        case SampleType::Int16:
            return "Int16";
        case SampleType::UInt32:
            return "UInt32";
        case SampleType::Int32:
            return "Int32";
        case SampleType::UInt64:
            return "UInt64";
        case SampleType::Int64:
            return "Int64";
        case SampleType::RangeInt64:
            return "RangeInt64";
        case SampleType::ComplexFloat32:
            return "ComplexFloat32";
        case SampleType::ComplexFloat64:
            return "ComplexFloat64";
        case SampleType::Struct:
            return "Struct";
        case SampleType::Undefined:
            return "Undefined";
        case SampleType::Binary:
            return "Binary";
        case SampleType::String:
            return "String";
        case SampleType::_count:
            return "Count";
    }
    return "Unknown";
}

template class TypedReader<SampleTypeToType<SampleType::Float32>::Type>;
template class TypedReader<SampleTypeToType<SampleType::Float64>::Type>;
template class TypedReader<SampleTypeToType<SampleType::UInt8>::Type>;
template class TypedReader<SampleTypeToType<SampleType::Int8>::Type>;
template class TypedReader<SampleTypeToType<SampleType::UInt16>::Type>;
template class TypedReader<SampleTypeToType<SampleType::Int16>::Type>;
template class TypedReader<SampleTypeToType<SampleType::UInt32>::Type>;
template class TypedReader<SampleTypeToType<SampleType::Int32>::Type>;
template class TypedReader<SampleTypeToType<SampleType::Int64>::Type>;
template class TypedReader<SampleTypeToType<SampleType::RangeInt64>::Type>;
template class TypedReader<SampleTypeToType<SampleType::ComplexFloat32>::Type>;
template class TypedReader<SampleTypeToType<SampleType::ComplexFloat64>::Type>;
template class TypedReader<SampleTypeToType<SampleType::Struct>::Type>;

END_NAMESPACE_OPENDAQ
