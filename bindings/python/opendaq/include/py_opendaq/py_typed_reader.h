/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <tuple>
#include <type_traits>

#include "coretypes/exceptions.h"
#include "opendaq/block_reader_ptr.h"
#include "opendaq/event_packet_ids.h"
#include "opendaq/multi_reader_ptr.h"
#include "opendaq/reader_config_ptr.h"
#include "opendaq/time_reader.h"
#include "py_core_types/py_converter.h"

#include <pybind11/chrono.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "py_opendaq/py_reader_traits.h"

template <typename ReaderType>
using ReaderStatusType = typename daq::ReaderStatusType<ReaderType>::IType*;

template <typename ReaderType>
using SampleTypeReaderStatusVariant = std::variant<py::array, std::tuple<py::array, ReaderStatusType<ReaderType>>>;

template <typename ReaderType>
using SampleTypeDomainTypeReaderStatusVariant =
    std::variant<std::tuple<py::array, py::array>, std::tuple<py::array, py::array, ReaderStatusType<ReaderType>>>;

template <typename ReaderType>
using SizeReaderStatusVariant = std::variant<daq::SizeT, std::tuple<daq::SizeT, ReaderStatusType<ReaderType>>>;

struct PyTypedReader
{
    static constexpr const char* VALUE_DATA_DESCRIPTOR_ATTRIBUTE = "__value_data_descriptor";
    static constexpr const char* DOMAIN_DATA_DESCRIPTOR_ATTRIBUTE = "__domain_data_descriptor";

    template <typename ReaderType>
    static inline SampleTypeReaderStatusVariant<ReaderType> readValues(const ReaderType& reader,
                                                                       size_t count,
                                                                       size_t timeoutMs,
                                                                       bool returnStatus)
    {
        daq::SampleType valueType = daq::SampleType::Undefined;
        reader->getValueReadType(&valueType);
        if (valueType == daq::SampleType::Undefined)
        {
            daq::DataDescriptorPtr vd = getDescriptor<ReaderType>(reader, VALUE_DATA_DESCRIPTOR_ATTRIBUTE);
            if (!vd.assigned())
            {
                auto status = readZeroValues(reader, timeoutMs);
                auto [dataDescriptor, domainDescriptor] = getDescriptorsFromStatus(status);

                if (!dataDescriptor.assigned() && !domainDescriptor.assigned())
                {
                    throw std::runtime_error("Undefined type reader has no descriptors");
                }

                setDescriptor(reader, VALUE_DATA_DESCRIPTOR_ATTRIBUTE, dataDescriptor);
                setDescriptor(reader, DOMAIN_DATA_DESCRIPTOR_ATTRIBUTE, domainDescriptor);
                return returnStatus ? SampleTypeReaderStatusVariant<ReaderType>(std::make_tuple(py::array{}, status.detach()))
                                    : SampleTypeReaderStatusVariant<ReaderType>(py::array{});
            }
            else
            {
                valueType = vd.getSampleType();
            }
        }

        switch (valueType)
        {
            case daq::SampleType::Float32:
                return read<daq::SampleTypeToType<daq::SampleType::Float32>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::Float64:
                return read<daq::SampleTypeToType<daq::SampleType::Float64>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::UInt32:
                return read<daq::SampleTypeToType<daq::SampleType::UInt32>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::Int32:
                return read<daq::SampleTypeToType<daq::SampleType::Int32>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::UInt64:
                return read<daq::SampleTypeToType<daq::SampleType::UInt64>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::Int64:
                return read<daq::SampleTypeToType<daq::SampleType::Int64>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::UInt8:
                return read<daq::SampleTypeToType<daq::SampleType::UInt8>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::Int8:
                return read<daq::SampleTypeToType<daq::SampleType::Int8>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::UInt16:
                return read<daq::SampleTypeToType<daq::SampleType::UInt16>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::Int16:
                return read<daq::SampleTypeToType<daq::SampleType::Int16>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::RangeInt64:
            case daq::SampleType::ComplexFloat64:
            case daq::SampleType::ComplexFloat32:
            case daq::SampleType::Undefined:
            case daq::SampleType::Binary:
            case daq::SampleType::String:
            case daq::SampleType::Struct:
            default:
                throw std::runtime_error("Unsupported values sample type: " + convertSampleTypeToString(valueType));
        }
    }

    template <typename ReaderType>
    static inline SampleTypeDomainTypeReaderStatusVariant<ReaderType> readValuesWithDomain(const ReaderType& reader,
                                                                                           size_t count,
                                                                                           size_t timeoutMs,
                                                                                           bool returnStatus)
    {
        daq::SampleType valueType = daq::SampleType::Undefined;
        reader->getValueReadType(&valueType);
        if (valueType == daq::SampleType::Undefined)
        {
            printf("Undefined value type\n");
            daq::DataDescriptorPtr vd = getDescriptor<ReaderType>(reader, VALUE_DATA_DESCRIPTOR_ATTRIBUTE);
            if (!vd.assigned())
            {
                printf("Getting descriptors from event\n");
                auto status = readZeroValues(reader, timeoutMs);
                auto [dataDescriptor, domainDescriptor] = getDescriptorsFromStatus(status);

                // if (!dataDescriptor.assigned() && !domainDescriptor.assigned())
                // {
                //     throw std::runtime_error("Undefined type reader has no descriptors");
                // }

                setDescriptor(reader, VALUE_DATA_DESCRIPTOR_ATTRIBUTE, dataDescriptor);
                setDescriptor(reader, DOMAIN_DATA_DESCRIPTOR_ATTRIBUTE, domainDescriptor);
                return returnStatus
                           ? SampleTypeDomainTypeReaderStatusVariant<ReaderType>(std::make_tuple(py::array{}, py::array{}, status.detach()))
                           : SampleTypeDomainTypeReaderStatusVariant<ReaderType>(std::make_tuple(py::array{}, py::array{}));
            }
            else
            {
                printf("Getting descriptors from attribute\n");
                valueType = vd.getSampleType();
            }
        }
        switch (valueType)
        {
            case daq::SampleType::Float32:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Float32>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::Float64:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Float64>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::UInt32:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::UInt32>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::Int32:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Int32>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::UInt64:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::UInt64>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::Int64:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Int64>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::UInt8:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::UInt8>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::Int8:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Int8>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::UInt16:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::UInt16>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::Int16:
                return readWithDomain<daq::SampleTypeToType<daq::SampleType::Int16>::Type>(reader, count, timeoutMs, returnStatus);
            case daq::SampleType::RangeInt64:
            case daq::SampleType::ComplexFloat64:
            case daq::SampleType::ComplexFloat32:
            case daq::SampleType::Undefined:
            case daq::SampleType::Binary:
            case daq::SampleType::String:
            case daq::SampleType::Struct:
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
    static inline SampleTypeDomainTypeReaderStatusVariant<ReaderType> readWithDomain(const ReaderType& reader,
                                                                                     size_t count,
                                                                                     size_t timeoutMs,
                                                                                     bool returnStatus)
    {
        if constexpr (std::is_base_of_v<daq::TimeReaderBase, ReaderType>)
        {
            return read<ValueType, std::chrono::system_clock::time_point>(reader, count, timeoutMs, returnStatus);
        }
        else
        {
            daq::SampleType domainType = daq::SampleType::Undefined;
            reader->getDomainReadType(&domainType);
            if (domainType == daq::SampleType::Undefined)
            {
                printf("Undefined domain type\n");
                daq::DataDescriptorPtr dd = getDescriptor<ReaderType>(reader, DOMAIN_DATA_DESCRIPTOR_ATTRIBUTE);
                if (!dd.assigned())
                {
                    printf("Getting descriptors from event\n");
                    auto status = readZeroValues(reader, timeoutMs);
                    auto [dataDescriptor, domainDescriptor] = getDescriptorsFromStatus(status);

                    // if (!dataDescriptor.assigned() && !domainDescriptor.assigned())
                    // {
                    //     throw std::runtime_error("Undefined type reader has no descriptors");
                    // }

                    setDescriptor(reader, VALUE_DATA_DESCRIPTOR_ATTRIBUTE, dataDescriptor);
                    setDescriptor(reader, DOMAIN_DATA_DESCRIPTOR_ATTRIBUTE, domainDescriptor);
                    return returnStatus ? SampleTypeDomainTypeReaderStatusVariant<ReaderType>(
                                              std::make_tuple(py::array{}, py::array{}, status.detach()))
                                        : SampleTypeDomainTypeReaderStatusVariant<ReaderType>(std::make_tuple(py::array{}, py::array{}));
                }
                else
                {
                    printf("Getting descriptors from attribute\n");
                    domainType = dd.getSampleType();
                }
            }

            switch (domainType)
            {
                case daq::SampleType::Float32:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Float32>::Type>(reader, count, timeoutMs, returnStatus);
                case daq::SampleType::Float64:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Float64>::Type>(reader, count, timeoutMs, returnStatus);
                case daq::SampleType::UInt32:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt32>::Type>(reader, count, timeoutMs, returnStatus);
                case daq::SampleType::Int32:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int32>::Type>(reader, count, timeoutMs, returnStatus);
                case daq::SampleType::UInt64:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt64>::Type>(reader, count, timeoutMs, returnStatus);
                case daq::SampleType::Int64:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int64>::Type>(reader, count, timeoutMs, returnStatus);
                case daq::SampleType::UInt8:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt8>::Type>(reader, count, timeoutMs, returnStatus);
                case daq::SampleType::Int8:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int8>::Type>(reader, count, timeoutMs, returnStatus);
                case daq::SampleType::UInt16:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt16>::Type>(reader, count, timeoutMs, returnStatus);
                case daq::SampleType::Int16:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int16>::Type>(reader, count, timeoutMs, returnStatus);
                case daq::SampleType::RangeInt64:
                case daq::SampleType::ComplexFloat64:
                case daq::SampleType::ComplexFloat32:
                case daq::SampleType::Undefined:
                case daq::SampleType::Binary:
                case daq::SampleType::String:
                case daq::SampleType::Struct:
                default:
                    throw std::runtime_error("Unsupported domain sample type: " + convertSampleTypeToString(domainType));
            }
        }
    }

    template <typename ValueType, typename ReaderType>
    static inline SampleTypeReaderStatusVariant<ReaderType> read(const ReaderType& reader,
                                                                 size_t count,
                                                                 [[maybe_unused]] size_t timeoutMs,
                                                                 bool returnStatus)
    {
        if (count == 0)
        {
            auto status = readZeroValues(reader, timeoutMs);
            return returnStatus ? SampleTypeReaderStatusVariant<ReaderType>{std::make_tuple(py::array{}, status.detach())}
                                : SampleTypeReaderStatusVariant<ReaderType>{};
        }

        size_t blockSize = 1, initialCount = count;
        constexpr const bool isMultiReader = std::is_base_of_v<daq::MultiReaderPtr, ReaderType>;

        if constexpr (std::is_same_v<ReaderType, daq::BlockReaderPtr>)
        {
            reader->getBlockSize(&blockSize);
        }
        if constexpr (isMultiReader)
        {
            daq::ReaderConfigPtr readerConfig = reader.template asPtr<daq::IReaderConfig>();
            blockSize = readerConfig.getInputPorts().getCount();
        }

        using StatusType = typename daq::ReaderStatusType<ReaderType>::Type;
        StatusType status;
        std::vector<ValueType> values(count * blockSize);
        if constexpr (ReaderHasReadWithTimeout<ReaderType, ValueType>::value)
        {
            if constexpr (isMultiReader)
            {
                std::vector<void*> ptrs(blockSize);
                for (size_t i = 0; i < blockSize; i++)
                {
                    ptrs[i] = values.data() + i * count;
                }
                reader->read(ptrs.data(), &count, timeoutMs, &status);
            }
            else
            {
                reader->read(values.data(), &count, timeoutMs, &status);
            }
        }
        else
        {
            reader->read(values.data(), &count, &status);
        }

        py::array::ShapeContainer shape;
        if (blockSize > 1)
        {
            if (!isMultiReader)
            {
                shape = {count, blockSize};
            }
            else
            {
                shape = {blockSize, count};
            }
        }
        else
            shape = {count};

        py::array::StridesContainer strides;
        if (blockSize > 1 && isMultiReader)
            strides = {sizeof(ValueType) * initialCount, sizeof(ValueType)};

        return returnStatus ? SampleTypeReaderStatusVariant<ReaderType>{std::make_tuple(toPyArray(std::move(values), shape, strides),
                                                                                        status.detach())}
                            : SampleTypeReaderStatusVariant<ReaderType>{toPyArray(std::move(values), shape, strides)};
    }

    template <typename ValueType, typename DomainType, typename ReaderType>
    static inline SampleTypeDomainTypeReaderStatusVariant<ReaderType> read(const ReaderType& reader,
                                                                           size_t count,
                                                                           [[maybe_unused]] size_t timeoutMs,
                                                                           bool returnStatus)
    {
        static_assert(sizeof(std::chrono::system_clock::time_point::rep) == sizeof(int64_t));
        using DomainVectorType = typename std::conditional<std::is_same<DomainType, std::chrono::system_clock::time_point>::value,
                                                           std::vector<int64_t>,
                                                           std::vector<DomainType>>::type;

        if (count == 0)
        {
            auto status = readZeroValues(reader, timeoutMs);
            return returnStatus
                       ? SampleTypeDomainTypeReaderStatusVariant<ReaderType>{std::make_tuple(py::array{}, py::array{}, status.detach())}
                       : SampleTypeDomainTypeReaderStatusVariant<ReaderType>{std::make_tuple(py::array{}, py::array{})};
        }

        size_t blockSize = 1, initialCount = count;
        constexpr const bool isMultiReader = std::is_base_of_v<daq::MultiReaderPtr, ReaderType>;
        if constexpr (std::is_base_of_v<daq::BlockReaderPtr, ReaderType>)
        {
            reader->getBlockSize(&blockSize);
        }
        if constexpr (isMultiReader)
        {
            daq::ReaderConfigPtr readerConfig = reader.template asPtr<daq::IReaderConfig>();
            blockSize = readerConfig.getInputPorts().getCount();
        }

        using StatusType = typename daq::ReaderStatusType<ReaderType>::Type;
        StatusType status;
        std::vector<ValueType> values(count * blockSize);
        DomainVectorType domain(count * blockSize);
        if constexpr (ReaderHasReadWithTimeout<ReaderType, ValueType>::value)
        {
            if constexpr (isMultiReader)
            {
                std::vector<void*> valuesPtrs(blockSize), domainPtrs(blockSize);
                for (size_t i = 0; i < blockSize; i++)
                {
                    valuesPtrs[i] = values.data() + i * count;
                    domainPtrs[i] = domain.data() + i * count;
                }
                reader->readWithDomain(valuesPtrs.data(), domainPtrs.data(), &count, timeoutMs, &status);
            }
            else
            {
                reader->readWithDomain(values.data(), domain.data(), &count, timeoutMs, &status);
            }
        }
        else
        {
            reader->readWithDomain(values.data(), domain.data(), &count, &status);
        }

        py::array::ShapeContainer shape;
        if (blockSize > 1)
        {
            if (!isMultiReader)
            {
                shape = {count, blockSize};
            }
            else
            {
                shape = {blockSize, count};
            }
        }
        else
            shape = {count};

        py::array::StridesContainer strides;
        if (blockSize > 1 && isMultiReader)
        {
            strides = {sizeof(ValueType) * initialCount, sizeof(ValueType)};
        }

        std::string domainDtype;
        if constexpr (std::is_same_v<DomainType, std::chrono::system_clock::time_point>)
        {
            domainDtype = "datetime64[ns]";
            std::transform(domain.begin(),
                           domain.end(),
                           domain.begin(),
                           [](int64_t timestamp)
                           {
                               const auto t = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(timestamp));
                               return std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
                           });
        }

        auto valuesArray = toPyArray(std::move(values), shape, strides);
        auto domainArray = toPyArray(std::move(domain), shape, strides, domainDtype);

        // WA for datetime64
        if (!domainDtype.empty())
            domainArray.attr("dtype") = domainDtype;

        return returnStatus
                   ? SampleTypeDomainTypeReaderStatusVariant<ReaderType>{std::make_tuple(
                         std::move(valuesArray), std::move(domainArray), status.detach())}
                   : SampleTypeDomainTypeReaderStatusVariant<ReaderType>{std::make_tuple(std::move(valuesArray), std::move(domainArray))};
    }

    template <typename ReaderType>
    static inline typename daq::ReaderStatusType<ReaderType>::Type readZeroValues(const ReaderType& reader, size_t timeoutMs)
    {
        using StatusType = typename daq::ReaderStatusType<ReaderType>::Type;
        StatusType status;
        size_t tmpCount = 0;
        if constexpr (ReaderHasReadWithTimeout<ReaderType, void>::value)
        {
            reader->read(nullptr, &tmpCount, timeoutMs, &status);
        }
        else
        {
            reader->read(nullptr, &tmpCount, &status);
        }
        return status;
    }

    static inline void checkSampleType(daq::SampleType type)
    {
        switch (type)
        {
            case daq::SampleType::Undefined:
            case daq::SampleType::Float32:
            case daq::SampleType::Float64:
            case daq::SampleType::UInt8:
            case daq::SampleType::Int8:
            case daq::SampleType::UInt16:
            case daq::SampleType::Int16:
            case daq::SampleType::UInt32:
            case daq::SampleType::Int32:
            case daq::SampleType::UInt64:
            case daq::SampleType::Int64:
            case daq::SampleType::Struct:
                // case daq::SampleType::Invalid:
                // case daq::SampleType::Binary: //banned
                break;
            case daq::SampleType::RangeInt64:
            case daq::SampleType::ComplexFloat32:  // complex64
            case daq::SampleType::ComplexFloat64:  // complex128
            case daq::SampleType::String:
            default:
                throw daq::InvalidParameterException("Unsupported sample type: " + convertSampleTypeToString(type));
        }
    }

    template <typename ReaderType>
    static daq::DataDescriptorPtr getDescriptor(const ReaderType& reader, const char* attribute_name)
    {
        py::object pyObject = py::cast(InterfaceWrapper<typename ReaderType::DeclaredInterface>(reader.addRefAndReturn()));
        try
        {
            auto descriptor = pyObject.attr(attribute_name).template cast<daq::IDataDescriptor*>();
            return daq::DataDescriptorPtr::Borrow(descriptor);
        }
        catch (const py::error_already_set& error)
        {
        }
        return nullptr;
    }

    template <typename ReaderType>
    static void setDescriptor(const ReaderType& reader, const char* attribute_name, daq::DataDescriptorPtr descriptor)
    {
        auto pyObject = py::cast(InterfaceWrapper<typename ReaderType::DeclaredInterface>(reader.addRefAndReturn()));
        pyObject.attr(attribute_name) = descriptor.detach();
    }

    static inline std::tuple<daq::DataDescriptorPtr, daq::DataDescriptorPtr> getDescriptorsFromStatus(const daq::ReaderStatusPtr& status)
    {
        daq::DataDescriptorPtr valueDescriptor;
        daq::DataDescriptorPtr domainDescriptor;

        if (status.assigned() && status.getReadStatus() == daq::ReadStatus::Event)
        {
            auto packet = status.getEventPacket();
            if (packet.assigned() && packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                auto params = packet.getParameters();
                if (params.assigned())
                {
                    valueDescriptor = params.get("DataDescriptor");
                    domainDescriptor = params.get("DomainDataDescriptor");
                }
            }
        }
        return {valueDescriptor, domainDescriptor};
    }
};
