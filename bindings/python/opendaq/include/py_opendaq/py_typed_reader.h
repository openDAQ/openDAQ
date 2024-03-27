/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

#include <cstddef>
#include <type_traits>

#include "coretypes/exceptions.h"
#include "opendaq/block_reader_ptr.h"
#include "opendaq/time_reader.h"
#include "py_core_types/py_converter.h"

#include <pybind11/chrono.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "py_opendaq/py_reader_traits.h"

using SampleTypeVariant = std::variant<py::array_t<daq::SampleTypeToType<daq::SampleType::Float32>::Type>,
                                       py::array_t<daq::SampleTypeToType<daq::SampleType::Float64>::Type>,
                                       py::array_t<daq::SampleTypeToType<daq::SampleType::UInt32>::Type>,
                                       py::array_t<daq::SampleTypeToType<daq::SampleType::Int32>::Type>,
                                       py::array_t<daq::SampleTypeToType<daq::SampleType::UInt64>::Type>,
                                       py::array_t<daq::SampleTypeToType<daq::SampleType::Int64>::Type>,
                                       py::array_t<daq::SampleTypeToType<daq::SampleType::UInt8>::Type>,
                                       py::array_t<daq::SampleTypeToType<daq::SampleType::Int8>::Type>,
                                       py::array_t<daq::SampleTypeToType<daq::SampleType::UInt16>::Type>,
                                       py::array_t<daq::SampleTypeToType<daq::SampleType::Int16>::Type>>;

using DomainTypeVariant = SampleTypeVariant;

struct PyTypedReader
{
    template <typename ReaderType>
    static inline SampleTypeVariant readValues(const ReaderType& reader, size_t count, size_t timeoutMs)
    {
        daq::SampleType valueType = daq::SampleType::Undefined;
        reader->getValueReadType(&valueType);
        switch (valueType)
        {
            case daq::SampleType::Float32:
                return read<daq::SampleTypeToType<daq::SampleType::Float32>::Type>(reader, count, timeoutMs);
            case daq::SampleType::Float64:
                return read<daq::SampleTypeToType<daq::SampleType::Float64>::Type>(reader, count, timeoutMs);
            case daq::SampleType::UInt32:
                return read<daq::SampleTypeToType<daq::SampleType::UInt32>::Type>(reader, count, timeoutMs);
            case daq::SampleType::Int32:
                return read<daq::SampleTypeToType<daq::SampleType::Int32>::Type>(reader, count, timeoutMs);
            case daq::SampleType::UInt64:
                return read<daq::SampleTypeToType<daq::SampleType::UInt64>::Type>(reader, count, timeoutMs);
            case daq::SampleType::Int64:
                return read<daq::SampleTypeToType<daq::SampleType::Int64>::Type>(reader, count, timeoutMs);
            case daq::SampleType::UInt8:
                return read<daq::SampleTypeToType<daq::SampleType::UInt8>::Type>(reader, count, timeoutMs);
            case daq::SampleType::Int8:
                return read<daq::SampleTypeToType<daq::SampleType::Int8>::Type>(reader, count, timeoutMs);
            case daq::SampleType::UInt16:
                return read<daq::SampleTypeToType<daq::SampleType::UInt16>::Type>(reader, count, timeoutMs);
            case daq::SampleType::Int16:
                return read<daq::SampleTypeToType<daq::SampleType::Int16>::Type>(reader, count, timeoutMs);
            case daq::SampleType::RangeInt64:
            case daq::SampleType::ComplexFloat64:
            case daq::SampleType::ComplexFloat32:
            case daq::SampleType::Undefined:
            case daq::SampleType::Binary:
            case daq::SampleType::String:
            default:
                throw std::runtime_error("Unsupported values sample type: " + convertSampleTypeToString(valueType));
        }
    }

    template <typename ReaderType>
    static inline std::tuple<SampleTypeVariant, DomainTypeVariant> readValuesWithDomain(const ReaderType& reader,
                                                                                        size_t count,
                                                                                        size_t timeoutMs)
    {
        daq::SampleType valueType = daq::SampleType::Undefined;
        reader->getValueReadType(&valueType);
        switch (valueType)
        {
            case daq::SampleType::Float32:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Float32>::Type>(reader, count, timeoutMs);
            case daq::SampleType::Float64:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Float64>::Type>(reader, count, timeoutMs);
            case daq::SampleType::UInt32:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::UInt32>::Type>(reader, count, timeoutMs);
            case daq::SampleType::Int32:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Int32>::Type>(reader, count, timeoutMs);
            case daq::SampleType::UInt64:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::UInt64>::Type>(reader, count, timeoutMs);
            case daq::SampleType::Int64:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Int64>::Type>(reader, count, timeoutMs);
            case daq::SampleType::UInt8:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::UInt8>::Type>(reader, count, timeoutMs);
            case daq::SampleType::Int8:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Int8>::Type>(reader, count, timeoutMs);
            case daq::SampleType::UInt16:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::UInt16>::Type>(reader, count, timeoutMs);
            case daq::SampleType::Int16:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Int16>::Type>(reader, count, timeoutMs);
            case daq::SampleType::RangeInt64:
            case daq::SampleType::ComplexFloat64:
            case daq::SampleType::ComplexFloat32:
            case daq::SampleType::Undefined:
            case daq::SampleType::Binary:
            case daq::SampleType::String:
            default:
                throw std::runtime_error("Unsupported values sample type: " + convertSampleTypeToString(valueType));
        }
    }

    static inline void checkTypes(daq::SampleType valueType, daq::SampleType domainType)
    {
        checkSampleType(valueType);
        checkSampleType(domainType);
    }

private:
    template <typename ValueType, typename ReaderType>
    static inline std::tuple<SampleTypeVariant, DomainTypeVariant> readWithDomain(const ReaderType& reader, size_t count, size_t timeoutMs)
    {
        if constexpr (std::is_base_of<daq::TimeReaderBase, ReaderType>::value)
        {
            return read<ValueType, std::chrono::system_clock::time_point>(reader, count, timeoutMs);
        }
        else
        {
            daq::SampleType domainType = daq::SampleType::Undefined;
            reader->getDomainReadType(&domainType);
            switch (domainType)
            {
                case daq::SampleType::Float32:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Float32>::Type>(reader, count, timeoutMs);
                case daq::SampleType::Float64:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Float64>::Type>(reader, count, timeoutMs);
                case daq::SampleType::UInt32:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt32>::Type>(reader, count, timeoutMs);
                case daq::SampleType::Int32:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int32>::Type>(reader, count, timeoutMs);
                case daq::SampleType::UInt64:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt64>::Type>(reader, count, timeoutMs);
                case daq::SampleType::Int64:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int64>::Type>(reader, count, timeoutMs);
                case daq::SampleType::UInt8:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt8>::Type>(reader, count, timeoutMs);
                case daq::SampleType::Int8:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int8>::Type>(reader, count, timeoutMs);
                case daq::SampleType::UInt16:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt16>::Type>(reader, count, timeoutMs);
                case daq::SampleType::Int16:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int16>::Type>(reader, count, timeoutMs);
                case daq::SampleType::RangeInt64:
                case daq::SampleType::ComplexFloat64:
                case daq::SampleType::ComplexFloat32:
                case daq::SampleType::Undefined:
                case daq::SampleType::Binary:
                case daq::SampleType::String:
                default:
                    throw std::runtime_error("Unsupported domain sample type: " + convertSampleTypeToString(domainType));
            }
        }
    }

    template <typename ValueType, typename ReaderType>
    static inline py::array_t<ValueType> read(const ReaderType& reader, size_t count, [[maybe_unused]] size_t timeoutMs)
    {
        size_t blockSize = 1;
        if constexpr (std::is_same_v<ReaderType, daq::BlockReaderPtr>)
        {
            reader->getBlockSize(&blockSize);
        }

        std::vector<ValueType> values(count * blockSize);
        if constexpr (ReaderHasReadWithTimeout<ReaderType, ValueType>::value)
        {
            reader->read(values.data(), &count, timeoutMs, nullptr);
        }
        else
        {
            reader->read(values.data(), &count, nullptr);
        }

        values.resize(count * blockSize);

        py::array::ShapeContainer shape;
        if (blockSize > 1)
            shape = {count, blockSize};
        else
            shape = {count};
        return toPyArray(std::move(values), shape);
    }

    template <typename ValueType, typename DomainType, typename ReaderType>
    static inline std::tuple<SampleTypeVariant, DomainTypeVariant> read(const ReaderType& reader, size_t count, [[maybe_unused]] size_t timeoutMs)
    {
        using DomainVectorType = typename std::conditional<std::is_same<DomainType, std::chrono::system_clock::time_point>::value,
                                                           std::vector<int64_t>,
                                                           std::vector<DomainType>>::type;

        size_t blockSize = 1;
        if constexpr (std::is_base_of_v<daq::BlockReaderPtr, ReaderType>)
        {
            daq::BlockReaderPtr blockReader = static_cast<daq::BlockReaderPtr>(reader);
            blockReader->getBlockSize(&blockSize);
        }

        std::vector<ValueType> values(count * blockSize);
        DomainVectorType domain(count * blockSize);

        if constexpr (std::is_same_v<DomainType, std::chrono::system_clock::time_point>)
        {
            static_assert(sizeof(std::chrono::system_clock::time_point::rep) == sizeof(int64_t));
            if constexpr (ReaderHasReadWithTimeout<ReaderType, ValueType>::value)
            {
                reader->readWithDomain(
                    values.data(), reinterpret_cast<std::chrono::system_clock::time_point*>(domain.data()), &count, timeoutMs, nullptr);
            }
            else
            {
                reader->readWithDomain(values.data(), reinterpret_cast<std::chrono::system_clock::time_point*>(domain.data()), &count, nullptr);
            }
        }
        else
        {
            if constexpr (ReaderHasReadWithTimeout<ReaderType, ValueType>::value)
            {
                reader->readWithDomain(values.data(), domain.data(), &count, timeoutMs, nullptr);
            }
            else
            {
                reader->readWithDomain(values.data(), domain.data(), &count, nullptr);
            }
        }

        domain.resize(count * blockSize);
        values.resize(count * blockSize);

        py::array::ShapeContainer shape;
        shape->emplace_back(count);
        if (blockSize > 1)
            shape->emplace_back(blockSize);

        std::string domainDtype;
        if constexpr (std::is_same_v<DomainType, std::chrono::system_clock::time_point>)
            domainDtype = "datetime64[ns]";

        auto valuesArray = toPyArray(std::move(values), shape);
        auto domainArray = toPyArray(std::move(domain), shape, domainDtype);

        // WA for datetime64
        if (!domainDtype.empty())
            domainArray.attr("dtype") = domainDtype;

        return {std::move(valuesArray), std::move(domainArray)};
    }

    static inline void checkSampleType(daq::SampleType type)
    {
        switch (type)
        {
            case daq::SampleType::Float32:
            case daq::SampleType::Float64:
            case daq::SampleType::UInt32:
            case daq::SampleType::Int32:
            case daq::SampleType::UInt64:
            case daq::SampleType::Int64:
            case daq::SampleType::UInt8:
            case daq::SampleType::Int8:
            case daq::SampleType::UInt16:
            case daq::SampleType::Int16:
            case daq::SampleType::Invalid:  // for default value
                break;
            default:
                throw daq::InvalidParameterException("Unsupported sample type: " + convertSampleTypeToString(type));
        }
    }
};
