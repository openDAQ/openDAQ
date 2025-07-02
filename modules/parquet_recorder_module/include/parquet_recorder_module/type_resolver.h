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

#include <cstdint>
#include <opendaq/sample_type.h>

#include <arrow/type.h>
#include <arrow/type_fwd.h>
#include <parquet/types.h>

#include <parquet_recorder_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE

template <typename T>
struct ArrowTypeResolver;

template <>
struct ArrowTypeResolver<void>
{
    using BuilderType = void;
    constexpr const static parquet::Type::type parquet_type = parquet::Type::UNDEFINED;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::NONE;
    static auto get()
    {
        return arrow::null();
    }
};

template <>
struct ArrowTypeResolver<std::int8_t>
{
    using BuilderType = arrow::Int8Builder;
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::INT_8;
    static auto get()
    {
        return arrow::int8();
    }
};

template <>
struct ArrowTypeResolver<std::int16_t>
{
    using BuilderType = arrow::Int16Builder;
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::INT_16;
    static auto get()
    {
        return arrow::int16();
    }
};

template <>
struct ArrowTypeResolver<std::int32_t>
{
    using BuilderType = arrow::Int32Builder;
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::INT_32;
    static auto get()
    {
        return arrow::int32();
    }
};

template <>
struct ArrowTypeResolver<std::int64_t>
{
    using BuilderType = arrow::Int64Builder;
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT64;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::INT_64;
    static auto get()
    {
        return arrow::int64();
    }
};

template <>
struct ArrowTypeResolver<std::uint8_t>
{
    using BuilderType = arrow::UInt8Builder;
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::UINT_8;
    static auto get()
    {
        return arrow::uint8();
    }
};

template <>
struct ArrowTypeResolver<std::uint16_t>
{
    using BuilderType = arrow::UInt16Builder;
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::UINT_16;
    static auto get()
    {
        return arrow::uint16();
    }
};

template <>
struct ArrowTypeResolver<std::uint32_t>
{
    using BuilderType = arrow::UInt32Builder;
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::UINT_32;
    static auto get()
    {
        return arrow::uint32();
    }
};

template <>
struct ArrowTypeResolver<std::uint64_t>
{
    using BuilderType = arrow::UInt64Builder;
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT64;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::UINT_64;
    static auto get()
    {
        return arrow::uint64();
    }
};

template <>
struct ArrowTypeResolver<float>
{
    using BuilderType = arrow::FloatBuilder;
    constexpr const static parquet::Type::type parquet_type = parquet::Type::FLOAT;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::NONE;
    static auto get()
    {
        return arrow::float32();
    }
};

template <>
struct ArrowTypeResolver<double>
{
    using BuilderType = arrow::DoubleBuilder;
    constexpr const static parquet::Type::type parquet_type = parquet::Type::DOUBLE;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::NONE;
    static auto get()
    {
        return arrow::float64();
    }
};

template <typename T>
auto get_arrow_type()
{
    return ArrowTypeResolver<T>::get();
}

template <typename T>
constexpr auto get_parquet_type()
{
    return ArrowTypeResolver<T>::parquet_type;
}

template <typename T>
constexpr auto get_parquet_converted_type()
{
    return ArrowTypeResolver<T>::parquet_converted_type;
}

template <SampleType Type>
struct DaqSampleToArrowBuilderType;

template <>
struct DaqSampleToArrowBuilderType<SampleType::Invalid>
{
    using Type = void;
    auto getArrowType() const
    {
        return arrow::null();
    }
};

template <>
struct DaqSampleToArrowBuilderType<SampleType::UInt8>
{
    using Type = arrow::UInt8Builder;
    auto getArrowType() const
    {
        return arrow::uint8();
    }
};

template <>
struct DaqSampleToArrowBuilderType<SampleType::Int8>
{
    using Type = arrow::Int8Builder;

    auto getArrowType() const
    {
        return arrow::int8();
    }
};

template <>
struct DaqSampleToArrowBuilderType<SampleType::UInt16>
{
    using Type = arrow::UInt16Builder;

    auto getArrowType() const
    {
        return arrow::uint16();
    }
};

template <>
struct DaqSampleToArrowBuilderType<SampleType::Int16>
{
    using Type = arrow::Int16Builder;

    auto getArrowType() const
    {
        return arrow::int16();
    }
};

template <>
struct DaqSampleToArrowBuilderType<SampleType::UInt32>
{
    using Type = arrow::UInt32Builder;

    auto getArrowType() const
    {
        return arrow::uint32();
    }
};

template <>
struct DaqSampleToArrowBuilderType<SampleType::Int32>
{
    using Type = arrow::Int32Builder;

    auto getArrowType() const
    {
        return arrow::int32();
    }
};

template <>
struct DaqSampleToArrowBuilderType<SampleType::UInt64>
{
    using Type = arrow::UInt64Builder;

    auto getArrowType() const
    {
        return arrow::uint64();
    }
};

template <>
struct DaqSampleToArrowBuilderType<SampleType::Int64>
{
    using Type = arrow::Int64Builder;

    auto getArrowType() const
    {
        return arrow::int64();
    }
};

template <>
struct DaqSampleToArrowBuilderType<SampleType::Float32>
{
    using Type = arrow::FloatBuilder;

    auto getArrowType() const
    {
        return arrow::float32();
    }
};

template <>
struct DaqSampleToArrowBuilderType<SampleType::Float64>
{
    using Type = arrow::DoubleBuilder;

    auto getArrowType() const
    {
        return arrow::float64();
    }
};

inline auto arrow_type_from_sample_type(SampleType type)
{
    switch (type)
    {
        case SampleType::UInt8:
            return arrow::uint8();
        case SampleType::Int8:
            return arrow::int8();
        case SampleType::UInt16:
            return arrow::uint16();
        case SampleType::Int16:
            return arrow::int16();
        case SampleType::UInt32:
            return arrow::uint32();
        case SampleType::Int32:
            return arrow::int32();
        case SampleType::UInt64:
            return arrow::uint64();
        case SampleType::Int64:
            return arrow::int64();
        case SampleType::Float32:
            return arrow::float32();
        case SampleType::Float64:
            return arrow::float64();
        case SampleType::Invalid:
        default:
            return arrow::null();
    }
}

END_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE