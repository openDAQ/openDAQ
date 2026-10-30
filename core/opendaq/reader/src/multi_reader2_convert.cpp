#include <opendaq/multi_reader2_convert.h>
#include <opendaq/sample_type_traits.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

namespace
{

bool isNumeric(SampleType type)
{
    return type >= SampleType::Float32 && type <= SampleType::Int64;
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
D convert(S s)
{
    if constexpr (std::is_same_v<D, float> && std::is_same_v<S, double>)
    {
        // A finite value beyond the float range saturates; NaN and infinity carry over
        if (std::isfinite(s))
            return static_cast<float>(std::clamp<double>(s, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()));
        return static_cast<float>(s);
    }
    else if constexpr (std::is_same_v<D, S> || std::is_floating_point_v<D>)
        return static_cast<D>(s);
    else if constexpr (std::is_floating_point_v<S>)
    {
        if (std::isnan(s))
            return 0;
        const S r = std::round(s);
        if (r <= static_cast<S>(std::numeric_limits<D>::min()))
            return std::numeric_limits<D>::min();
        if (r >= static_cast<S>(std::numeric_limits<D>::max()))
            return std::numeric_limits<D>::max();
        return static_cast<D>(r);
    }
    else
    {
        if constexpr (std::is_signed_v<S>)
            if (s < 0)
            {
                if constexpr (std::is_unsigned_v<D>)
                    return 0;
                else if (static_cast<std::int64_t>(s) < static_cast<std::int64_t>(std::numeric_limits<D>::min()))
                    return std::numeric_limits<D>::min();
                else
                    return static_cast<D>(s);
            }
        if (static_cast<std::uint64_t>(s) > static_cast<std::uint64_t>(std::numeric_limits<D>::max()))
            return std::numeric_limits<D>::max();
        return static_cast<D>(s);
    }
}

template <typename D, typename S>
void convertRun(const void* src, void* dst, SizeT n)
{
    const S* s = static_cast<const S*>(src);
    D* d = static_cast<D*>(dst);
    for (SizeT i = 0; i < n; ++i)
        d[i] = convert<D>(s[i]);
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
