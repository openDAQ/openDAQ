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

#include <arrow/type.h>
#include <arrow/type_fwd.h>
#include <parquet/types.h>

#include <basic_csv_recorder_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

template<typename T>
struct ArrowTypeResolver;

template<>
struct ArrowTypeResolver<void> {
    constexpr const static parquet::Type::type parquet_type = parquet::Type::UNDEFINED;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::NONE;
    static auto get() { return arrow::null(); }
};

template <>
struct ArrowTypeResolver<std::int8_t> {
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::INT_8;
    static auto get() { return arrow::int8(); }
};

template <>
struct ArrowTypeResolver<std::int16_t> {
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::INT_16;
    static auto get() { return arrow::int16(); }
};

template <>
struct ArrowTypeResolver<std::int32_t> {
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::INT_32;
    static auto get() { return arrow::int32(); }
};

template <>
struct ArrowTypeResolver<std::int64_t> {
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT64;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::INT_64;
    static auto get() { return arrow::int64(); }
};

template <>
struct ArrowTypeResolver<std::uint8_t> {
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::UINT_8;
    static auto get() { return arrow::uint8(); }
};

template <>
struct ArrowTypeResolver<std::uint16_t> {
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::UINT_16;
    static auto get() { return arrow::uint16(); }
};

template <>
struct ArrowTypeResolver<std::uint32_t> {
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT32;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::UINT_32;
    static auto get() { return arrow::uint32(); }
};

template <>
struct ArrowTypeResolver<std::uint64_t> {
    constexpr const static parquet::Type::type parquet_type = parquet::Type::INT64;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::UINT_64;
    static auto get() { return arrow::uint64(); }
};

template <>
struct ArrowTypeResolver<float> {
    constexpr const static parquet::Type::type parquet_type = parquet::Type::FLOAT;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::NONE;
    static auto get() { return arrow::float32(); }
};

template <>
struct ArrowTypeResolver<double> {
    constexpr const static parquet::Type::type parquet_type = parquet::Type::DOUBLE;
    constexpr const static parquet::ConvertedType::type parquet_converted_type = parquet::ConvertedType::NONE;
    static auto get() { return arrow::float64(); }
};

template<typename T>
auto get_arrow_type() {
    return ArrowTypeResolver<T>::get();
}

template<typename T>
constexpr auto get_parquet_type() {
    return ArrowTypeResolver<T>::parquet_type;
}

template<typename T>
constexpr auto get_parquet_converted_type() {
    return ArrowTypeResolver<T>::parquet_converted_type;
}

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE