#include <opendaq/multi_reader2_convert.h>
#include <opendaq/reader_utils.h>
#include <opendaq/sample_type_traits.h>

#include <cstring>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

namespace
{

bool isNumeric(SampleType type)
{
    switch (type)
    {
        case SampleType::Float32:
        case SampleType::Float64:
        case SampleType::Int8:
        case SampleType::UInt8:
        case SampleType::Int16:
        case SampleType::UInt16:
        case SampleType::Int32:
        case SampleType::UInt32:
        case SampleType::Int64:
        case SampleType::UInt64:
            return true;
        default:
            return false;
    }
}

SizeT elementCount(const DataDescriptorPtr& value)
{
    SizeT count = 1;
    const auto dimensions = value.getDimensions();
    if (dimensions.assigned())
        for (const auto& dimension : dimensions)
            count *= dimension.getSize();
    return count;
}

template <typename D, typename S>
void convertRun(const void* src, void* dst, SizeT n)
{
    const S* s = static_cast<const S*>(src);
    D* d = static_cast<D*>(dst);
    for (SizeT i = 0; i < n; ++i)
        d[i] = reader::convertSample<D>(s[i]);
}

template <typename D>
void convertFrom(SampleType srcType, const void* src, void* dst, SizeT n)
{
    switch (srcType)
    {
        case SampleType::Float32:
            return convertRun<D, float>(src, dst, n);
        case SampleType::Float64:
            return convertRun<D, double>(src, dst, n);
        case SampleType::UInt8:
            return convertRun<D, std::uint8_t>(src, dst, n);
        case SampleType::Int8:
            return convertRun<D, std::int8_t>(src, dst, n);
        case SampleType::UInt16:
            return convertRun<D, std::uint16_t>(src, dst, n);
        case SampleType::Int16:
            return convertRun<D, std::int16_t>(src, dst, n);
        case SampleType::UInt32:
            return convertRun<D, std::uint32_t>(src, dst, n);
        case SampleType::Int32:
            return convertRun<D, std::int32_t>(src, dst, n);
        case SampleType::UInt64:
            return convertRun<D, std::uint64_t>(src, dst, n);
        case SampleType::Int64:
            return convertRun<D, std::int64_t>(src, dst, n);
        default:
            return;
    }
}

void convertElements(SampleType srcType, const void* src, SampleType dstType, void* dst, SizeT n)
{
    switch (dstType)
    {
        case SampleType::Float32:
            return convertFrom<float>(srcType, src, dst, n);
        case SampleType::Float64:
            return convertFrom<double>(srcType, src, dst, n);
        case SampleType::UInt8:
            return convertFrom<std::uint8_t>(srcType, src, dst, n);
        case SampleType::Int8:
            return convertFrom<std::int8_t>(srcType, src, dst, n);
        case SampleType::UInt16:
            return convertFrom<std::uint16_t>(srcType, src, dst, n);
        case SampleType::Int16:
            return convertFrom<std::int16_t>(srcType, src, dst, n);
        case SampleType::UInt32:
            return convertFrom<std::uint32_t>(srcType, src, dst, n);
        case SampleType::Int32:
            return convertFrom<std::int32_t>(srcType, src, dst, n);
        case SampleType::UInt64:
            return convertFrom<std::uint64_t>(srcType, src, dst, n);
        case SampleType::Int64:
            return convertFrom<std::int64_t>(srcType, src, dst, n);
        default:
            return;
    }
}

}

bool isReadType(SampleType readType)
{
    return readType == SampleType::Undefined || isNumeric(readType);
}

SizeT readSampleSize(const DataDescriptorPtr& value, SampleType readType)
{
    if (!value.assigned())
        return 0;
    const SampleType type = value.getSampleType();
    if (readType == SampleType::Undefined)
    {
        if (type == SampleType::Struct)
            return value.getRawSampleSize();
        return type == SampleType::Binary || type == SampleType::String || type == SampleType::Null
                   ? 0
                   : elementCount(value) * getSampleSize(type);
    }
    return isNumeric(type) ? elementCount(value) * getSampleSize(readType) : 0;
}

void copySamples(const DataPacketPtr& packet, SizeT first, SizeT count, void* dst, SampleType readType)
{
    const auto value = packet.getDataDescriptor();
    const SampleType type = value.getSampleType();
    if (type == SampleType::Struct)
    {
        const SizeT size = value.getRawSampleSize();
        std::memcpy(dst, static_cast<const std::uint8_t*>(packet.getRawData()) + first * size, count * size);
        return;
    }
    const SizeT elements = elementCount(value);
    const auto* src = static_cast<const std::uint8_t*>(packet.getData()) + first * elements * getSampleSize(type);
    if (readType == SampleType::Undefined || readType == type)
        std::memcpy(dst, src, count * elements * getSampleSize(type));
    else
        convertElements(type, src, readType, dst, count * elements);
}

}

END_NAMESPACE_OPENDAQ
