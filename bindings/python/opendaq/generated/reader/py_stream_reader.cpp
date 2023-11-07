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

#include "py_core_types/py_converter.h"
#include "py_opendaq/py_opendaq.h"

#include <pybind11/chrono.h>
#include <pybind11/stl.h>

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

template <typename ValueType>
inline py::array_t<ValueType> read(const daq::StreamReaderPtr& reader, size_t count, size_t timeoutMs)
{
    std::vector<ValueType> values(count);
    reader->read(values.data(), &count, timeoutMs);
    values.resize(count);
    return toPyArray(std::move(values));
}

template <typename ValueType, typename DomainType, typename ReaderType>
inline std::tuple<SampleTypeVariant, DomainTypeVariant> read(const ReaderType& reader, size_t count, size_t timeoutMs)
{
    using DomainVectorType = typename std::conditional<std::is_same<DomainType, std::chrono::system_clock::time_point>::value,
                                                       std::vector<int64_t>,
                                                       std::vector<DomainType>>::type;

    std::vector<ValueType> values(count);
    DomainVectorType domain(count);

    if constexpr (std::is_same_v<DomainType, std::chrono::system_clock::time_point>)
    {
        static_assert(sizeof(std::chrono::system_clock::time_point::rep) == sizeof(int64_t));
        reader.readWithDomain(values.data(), reinterpret_cast<std::chrono::system_clock::time_point*>(domain.data()), &count, timeoutMs);
    }
    else
    {
        reader.readWithDomain(values.data(), domain.data(), &count, timeoutMs);
    }

    domain.resize(count);
    values.resize(count);

    auto valuesArray = toPyArray(std::move(values));
    auto domainArray = toPyArray(std::move(domain));
    if constexpr (std::is_same_v<DomainType, std::chrono::system_clock::time_point>)
    {
        domainArray.attr("dtype") = "datetime64[ns]";
    }

    return {valuesArray, domainArray};
}

template <typename ValueType, typename ReaderType>
inline std::tuple<SampleTypeVariant, DomainTypeVariant> readWithDomain(const ReaderType& reader, size_t count, size_t timeoutMs)
{
    if constexpr (std::is_same_v<ReaderType, daq::TimeReader<daq::StreamReaderPtr>>)
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

template <typename ReaderType>
inline std::tuple<SampleTypeVariant, DomainTypeVariant> readValuesWithDomain(const ReaderType& reader, size_t count, size_t timeoutMs)
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

inline SampleTypeVariant readValues(const daq::StreamReaderPtr& reader, size_t count, size_t timeoutMs)
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

PyDaqIntf<daq::IStreamReader, daq::ISampleReader> declareIStreamReader(pybind11::module_ m)
{
    return wrapInterface<daq::IStreamReader, daq::ISampleReader>(m, "IStreamReader", py::dynamic_attr());
}

py::class_<daq::TimeReader<daq::StreamReaderPtr>> declareTimeStreamReader(pybind11::module_ m)
{
    return py::class_<daq::TimeReader<daq::StreamReaderPtr>>(m, "TimeReader");
}

void defineIStreamReader(pybind11::module_ m, PyDaqIntf<daq::IStreamReader, daq::ISampleReader> cls)
{
    cls.doc() = "A signal data reader that abstracts away reading of signal packets by keeping an internal read-position and automatically "
                "advances it on subsequent reads.";

    m.def(
        "StreamReader",
        [](daq::ISignal* signal, daq::SampleType valueType, daq::SampleType domainType, daq::ReadTimeoutType timeoutType)
        {
            const auto signalPtr = daq::SignalPtr::Borrow(signal);
            if (valueType != daq::SampleType::Invalid || domainType != daq::SampleType::Invalid)
                return daq::StreamReader(signalPtr, valueType, domainType, daq::ReadMode::Scaled, timeoutType).detach();
            else
                return daq::StreamReader(signalPtr, daq::ReadMode::Scaled, timeoutType).detach();
        },
        py::arg("signal"),
        py::arg("value_type") = daq::SampleType::Invalid,
        py::arg("domain_type") = daq::SampleType::Invalid,
        py::arg("timeout_type") = daq::ReadTimeoutType::All,
        "");
    m.def("StreamReaderFromExisting", &daq::StreamReaderFromExisting_Create);

    cls.def(
        "read",
        [](daq::IStreamReader* object, size_t count, const size_t timeoutMs)
        { return readValues(daq::StreamReaderPtr::Borrow(object), count, timeoutMs); },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        "Copies at maximum the next `count` unread samples to the values buffer. The amount actually read is returned through the `count` "
        "parameter.");
    cls.def(
        "read_with_domain",
        [](daq::IStreamReader* object, size_t count, const size_t timeoutMs)
        { return readValuesWithDomain(daq::StreamReaderPtr::Borrow(object), count, timeoutMs); },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        "Copies at maximum the next `count` unread samples and clock-stamps to the `values` and `stamps` buffers. The amount actually read "
        "is returned through the `count` parameter.");
}

void defineTimeStreamReader(pybind11::module_ m, py::class_<daq::TimeReader<daq::StreamReaderPtr>> cls)
{
    cls.doc() = "A wrapper for stream signal data reader that provides the ability to read samples with timestamps.";

    cls.def(py::init(
        [](daq::IStreamReader* reader)
        {
            const auto objectPtr = daq::StreamReaderPtr::Borrow(reader);
            return std::make_unique<daq::TimeReader<daq::StreamReaderPtr>>(objectPtr);
        }));
    cls.def(
        "read_with_timestamps",
        [](daq::TimeReader<daq::StreamReaderPtr>* object, size_t count, const size_t timeoutMs)
        {
            return readValuesWithDomain(*object, count, timeoutMs);
        },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        "Copies at maximum the next `count` unread samples and clock-stamps to the `values` and `stamps` buffers. The amount actually read "
        "is returned through the `count` parameter.");
}
