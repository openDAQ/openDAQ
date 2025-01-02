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
#include <opendaq/sample_type.h>
#include <coretypes/complex_number_type.h>
#include <opendaq/range_type.h>

BEGIN_NAMESPACE_OPENDAQ

using RawPtr = void*;

// Values need to have the same order/index as SampleType enum
#define OPENDAQ_DEFAULT_SAMPLE_TYPES OPENDAQ_SAMPLE_TYPES(RawPtr, float,  double, uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, RangeType64, ComplexFloat32, ComplexFloat64, BinarySample)
#define OPENDAQ_VALUE_SAMPLE_TYPES   OPENDAQ_SAMPLE_TYPES(        float,  double, uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, RangeType64, ComplexFloat32, ComplexFloat64)

#pragma pack(push, 1)

/////////////////////

struct BinarySample
{
    SizeT position;
    SizeT length;
};

/////////////////////

#pragma pack(pop)

template <typename T>
struct SampleTypeFromType
{
    static_assert(DependentFalse<T>::value, "Not a valid sample type!");
};

//////////////////////////
//
//   SampleTypeToType
//
/////////////////////////

template <SampleType Type>
struct SampleTypeToType;

template <>
struct SampleTypeToType<SampleType::Invalid>
{
    using Type = void;
    static constexpr daq::SampleType SampleType = daq::SampleType::Invalid;
};

template <>
struct SampleTypeToType<SampleType::UInt8>
{
    using Type = uint8_t;
    static constexpr daq::SampleType SampleType = daq::SampleType::UInt8;
    static constexpr daq::ScaledSampleType DefaultScaledType = daq::ScaledSampleType::Float32;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::Int8>
{
    using Type = int8_t;
    static constexpr daq::SampleType SampleType = daq::SampleType::Int8;
    static constexpr daq::ScaledSampleType DefaultScaledType = daq::ScaledSampleType::Float32;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::UInt16>
{
    using Type = uint16_t;
    static constexpr daq::SampleType SampleType = daq::SampleType::UInt16;
    static constexpr daq::ScaledSampleType DefaultScaledType = daq::ScaledSampleType::Float32;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::Int16>
{
    using Type = int16_t;
    static constexpr daq::SampleType SampleType = daq::SampleType::Int16;
    static constexpr daq::ScaledSampleType DefaultScaledType = daq::ScaledSampleType::Float32;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::UInt32>
{
    using Type = uint32_t;
    static constexpr daq::SampleType SampleType = daq::SampleType::UInt32;
    static constexpr daq::ScaledSampleType DefaultScaledType = daq::ScaledSampleType::Float64;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::Int32>
{
    using Type = int32_t;
    static constexpr daq::SampleType SampleType = daq::SampleType::Int32;
    static constexpr daq::ScaledSampleType DefaultScaledType = daq::ScaledSampleType::Float64;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::UInt64>
{
    using Type = uint64_t;
    static constexpr daq::SampleType SampleType = daq::SampleType::UInt64;
    static constexpr daq::ScaledSampleType DefaultScaledType = daq::ScaledSampleType::Float64;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::Int64>
{
    using Type = int64_t;
    static constexpr daq::SampleType SampleType = daq::SampleType::Int64;
    static constexpr daq::ScaledSampleType DefaultScaledType = daq::ScaledSampleType::Float64;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::Float32>
{
    using Type = float;
    static constexpr daq::SampleType SampleType = daq::SampleType::Float32;
    static constexpr daq::ScaledSampleType DefaultScaledType = daq::ScaledSampleType::Float32;

    static constexpr bool IsScaledType = true;
};

template <>
struct SampleTypeToType<SampleType::Float64>
{
    using Type = double;
    static constexpr daq::SampleType SampleType = daq::SampleType::Float64;
    static constexpr daq::ScaledSampleType DefaultScaledType = daq::ScaledSampleType::Float64;

    static constexpr bool IsScaledType = true;
};

template <>
struct SampleTypeToType<SampleType::ComplexFloat32>
{
    using Type = Complex_Number<float>;
    static constexpr daq::SampleType SampleType = daq::SampleType::ComplexFloat32;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::ComplexFloat64>
{
    using Type = Complex_Number<double>;
    static constexpr daq::SampleType SampleType = daq::SampleType::ComplexFloat64;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::String>
{
    using Type = BinarySample;
    static constexpr daq::SampleType SampleType = daq::SampleType::String;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::Binary>
{
    using Type = BinarySample;
    static constexpr daq::SampleType SampleType = daq::SampleType::Binary;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::Struct>
{
    using Type = void*;
    static constexpr daq::SampleType SampleType = daq::SampleType::Struct;

    static constexpr bool IsScaledType = false;
};

template <>
struct SampleTypeToType<SampleType::RangeInt64>
{
    using Type = RangeType<int64_t>;
    static constexpr daq::SampleType SampleType = daq::SampleType::RangeInt64;

    static constexpr bool IsScaledType = false;
};

//////////////////////////
//
//  SampleTypeFromType
//
/////////////////////////

template <>
struct SampleTypeFromType<void> : SampleTypeToType<SampleType::Invalid>
{
};

template <>
struct SampleTypeFromType<void*> : SampleTypeToType<SampleType::Struct>
{
};

template <>
struct SampleTypeFromType<uint8_t> : SampleTypeToType<SampleType::UInt8>
{
};

template <>
struct SampleTypeFromType<int8_t> : SampleTypeToType<SampleType::Int8>
{
};

template <>
struct SampleTypeFromType<uint16_t> : SampleTypeToType<SampleType::UInt16>
{
};

template <>
struct SampleTypeFromType<int16_t> : SampleTypeToType<SampleType::Int16>
{
};

template <>
struct SampleTypeFromType<uint32_t> : SampleTypeToType<SampleType::UInt32>
{
};

template <>
struct SampleTypeFromType<int32_t> : SampleTypeToType<SampleType::Int32>
{
};

template <>
struct SampleTypeFromType<uint64_t> : SampleTypeToType<SampleType::UInt64>
{
};

template <>
struct SampleTypeFromType<int64_t> : SampleTypeToType<SampleType::Int64>
{
};

template <>
struct SampleTypeFromType<float> : SampleTypeToType<SampleType::Float32>
{
};

template <>
struct SampleTypeFromType<double> : SampleTypeToType<SampleType::Float64>
{
};

template <>
struct SampleTypeFromType<std::string> : SampleTypeToType<SampleType::String>
{
};

template <>
struct SampleTypeFromType<std::wstring> : SampleTypeToType<SampleType::String>
{
};

template <>
struct SampleTypeFromType<Complex_Number<float>> : SampleTypeToType<SampleType::ComplexFloat32>
{
};

template <>
struct SampleTypeFromType<Complex_Number<double>> : SampleTypeToType<SampleType::ComplexFloat64>
{
};

template <>
struct SampleTypeFromType<BinarySample> : SampleTypeToType<SampleType::Binary>
{
};

template <>
struct SampleTypeFromType<RangeType<int64_t>> : SampleTypeToType<SampleType::RangeInt64>
{
};

inline bool isScaledSampleType(SampleType sampleType)
{
    switch (sampleType)
    {
        case SampleType::Float32:
            return SampleTypeToType<SampleType::Float32>::IsScaledType;
        case SampleType::Float64:
            return SampleTypeToType<SampleType::Float64>::IsScaledType;
        case SampleType::UInt8:
            return SampleTypeToType<SampleType::UInt8>::IsScaledType;
        case SampleType::Int8:
            return SampleTypeToType<SampleType::Int8>::IsScaledType;
        case SampleType::UInt16:
            return SampleTypeToType<SampleType::UInt16>::IsScaledType;
        case SampleType::Int16:
            return SampleTypeToType<SampleType::Int16>::IsScaledType;
        case SampleType::UInt32:
            return SampleTypeToType<SampleType::UInt32>::IsScaledType;
        case SampleType::Int32:
            return SampleTypeToType<SampleType::Int32>::IsScaledType;
        case SampleType::UInt64:
            return SampleTypeToType<SampleType::UInt64>::IsScaledType;
        case SampleType::Int64:
            return SampleTypeToType<SampleType::Int64>::IsScaledType;
        case SampleType::ComplexFloat32:
            return SampleTypeToType<SampleType::ComplexFloat32>::IsScaledType;
        case SampleType::ComplexFloat64:
            return SampleTypeToType<SampleType::ComplexFloat64>::IsScaledType;
        case SampleType::String:
            return SampleTypeToType<SampleType::String>::IsScaledType;
        case SampleType::Binary:
            return SampleTypeToType<SampleType::Binary>::IsScaledType;
        case SampleType::Struct:
            return SampleTypeToType<SampleType::Struct>::IsScaledType;
        case SampleType::RangeInt64:
            return SampleTypeToType<SampleType::RangeInt64>::IsScaledType;
        case SampleType::Invalid:
        case SampleType::Null:
        case SampleType::_count:
            return false;
    }

    return false;
}

inline std::size_t getSampleSize(SampleType sampleType)
{
    switch (sampleType)
    {
        case SampleType::Float32:
            return sizeof(SampleTypeToType<SampleType::Float32>::Type);
        case SampleType::Float64:
            return sizeof(SampleTypeToType<SampleType::Float64>::Type);
        case SampleType::UInt8:
            return sizeof(SampleTypeToType<SampleType::UInt8>::Type);
        case SampleType::Int8:
            return sizeof(SampleTypeToType<SampleType::Int8>::Type);
        case SampleType::UInt16:
            return sizeof(SampleTypeToType<SampleType::UInt16>::Type);
        case SampleType::Int16:
            return sizeof(SampleTypeToType<SampleType::Int16>::Type);
        case SampleType::UInt32:
            return sizeof(SampleTypeToType<SampleType::UInt32>::Type);
        case SampleType::Int32:
            return sizeof(SampleTypeToType<SampleType::Int32>::Type);
        case SampleType::UInt64:
            return sizeof(SampleTypeToType<SampleType::UInt64>::Type);
        case SampleType::Int64:
            return sizeof(SampleTypeToType<SampleType::Int64>::Type);
        case SampleType::RangeInt64:
            return sizeof(SampleTypeToType<SampleType::RangeInt64>::Type);
        case SampleType::ComplexFloat32:
            return sizeof(SampleTypeToType<SampleType::ComplexFloat32>::Type);
        case SampleType::ComplexFloat64:
            return sizeof(SampleTypeToType<SampleType::ComplexFloat64>::Type);
        case SampleType::String:
        case SampleType::Binary:
        case SampleType::Struct:
        case SampleType::Invalid:
        case SampleType::Null:
        case SampleType::_count:
            break;
    }

    return 0;
}

inline ScaledSampleType getDefaultScaledType(SampleType type)
{
    switch (type)
    {
        case SampleType::UInt8:
            return SampleTypeToType<SampleType::UInt8>::DefaultScaledType;
        case SampleType::Int8:
            return SampleTypeToType<SampleType::Int8>::DefaultScaledType;
        case SampleType::UInt16:
            return SampleTypeToType<SampleType::UInt16>::DefaultScaledType;
        case SampleType::Int16:
            return SampleTypeToType<SampleType::Int16>::DefaultScaledType;
        case SampleType::UInt32:
            return SampleTypeToType<SampleType::UInt32>::DefaultScaledType;
        case SampleType::Int32:
            return SampleTypeToType<SampleType::Int32>::DefaultScaledType;
        case SampleType::UInt64:
            return SampleTypeToType<SampleType::UInt64>::DefaultScaledType;
        case SampleType::Int64:
            return SampleTypeToType<SampleType::Int64>::DefaultScaledType;
        case SampleType::Float64:
            return SampleTypeToType<SampleType::Float64>::DefaultScaledType;
        case SampleType::Float32:
            return SampleTypeToType<SampleType::Float32>::DefaultScaledType;
        case SampleType::ComplexFloat32:
        case SampleType::ComplexFloat64:
        case SampleType::String:
        case SampleType::Binary:
        case SampleType::RangeInt64:
        case SampleType::Struct:
        case SampleType::Invalid:
        case SampleType::Null:
        case SampleType::_count:
            break;
    }

    return INVALID_SCALED_SAMPLE_TYPE;
}

inline SampleType convertScaledToSampleType(ScaledSampleType type)
{
    switch (type)
    {
        case ScaledSampleType::Float32:
            return SampleType::Float32;
        case ScaledSampleType::Float64:
            return SampleType::Float64;
        case ScaledSampleType::Invalid:
            break;
    }

    return INVALID_SAMPLE_TYPE;
}

inline std::string convertSampleTypeToString(SampleType type)
{
    switch (type)
    {
        case SampleType::Float64:
            return "Float64";
        case SampleType::Float32:
            return "Float32";
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
        case SampleType::String:
            return "String";
        case SampleType::Binary:
            return "Binary";
        case SampleType::Struct:
            return "Struct";
        case SampleType::Null:
            return "Null";
        case SampleType::Invalid:
        case SampleType::_count:
            break;
    }

    return "Invalid";
}

inline std::string convertScaledSampleTypeToString(ScaledSampleType type)
{
    switch (type)
    {
        case ScaledSampleType::Float32:
            return "Float32";
        case ScaledSampleType::Float64:
            return "Float64";
        case ScaledSampleType::Invalid:
            return "Invalid";
    }

    return "Invalid";
}

using ClockRange = typename SampleTypeToType<SampleType::RangeInt64>::Type;
using ClockTick = ClockRange::Type;

END_NAMESPACE_OPENDAQ
