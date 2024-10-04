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
#include "opendaq/data_descriptor_ptr.h"
#include "opendaq/event_packet_ids.h"
#include "opendaq/event_packet_ptr.h"
#include "opendaq/multi_reader_ptr.h"
#include "opendaq/multi_reader_status.h"
#include "opendaq/reader_config_ptr.h"
#include "opendaq/time_reader.h"
#include "py_core_types/py_converter.h"

#include <pybind11/chrono.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pybind11/gil.h>

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

struct StructPlaceholder
{
};

template <typename ValueType>
struct SampleTypeToBufferType
{
    using Type = ValueType;
};

template <>
struct SampleTypeToBufferType<StructPlaceholder>
{
    using Type = uint8_t;
};

template <>
struct SampleTypeToBufferType<std::chrono::system_clock::time_point>
{
    using Type = uint64_t;
};

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

        daq::DataDescriptorPtr dataDescriptor;
        if (valueType == daq::SampleType::Undefined || valueType == daq::SampleType::Struct)
        {
            dataDescriptor = getDescriptor<ReaderType>(reader, VALUE_DATA_DESCRIPTOR_ATTRIBUTE);
            if (!dataDescriptor.assigned())
            {
                auto status = readZeroValues(reader, timeoutMs);
                assignDescriptorsFromStatus(reader, status);
                py::gil_scoped_acquire acquire;
                return returnStatus ? SampleTypeReaderStatusVariant<ReaderType>(std::make_tuple(py::array{}, status.detach()))
                                    : SampleTypeReaderStatusVariant<ReaderType>(py::array{});
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
            case daq::SampleType::Struct:
                return read<StructPlaceholder>(reader, count, timeoutMs, returnStatus, dataDescriptor);
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
    static inline SampleTypeDomainTypeReaderStatusVariant<ReaderType> readValuesWithDomain(const ReaderType& reader,
                                                                                           size_t count,
                                                                                           size_t timeoutMs,
                                                                                           bool returnStatus)
    {
        daq::SampleType valueType = daq::SampleType::Undefined;
        reader->getValueReadType(&valueType);

        daq::DataDescriptorPtr dataDescriptor;
        if (valueType == daq::SampleType::Undefined || valueType == daq::SampleType::Struct)
        {
            dataDescriptor = getDescriptor<ReaderType>(reader, VALUE_DATA_DESCRIPTOR_ATTRIBUTE);
            if (!dataDescriptor.assigned())
            {
                auto status = readZeroValues(reader, timeoutMs);
                assignDescriptorsFromStatus(reader, status);
                py::gil_scoped_acquire acquire;
                return returnStatus
                           ? SampleTypeDomainTypeReaderStatusVariant<ReaderType>(std::make_tuple(py::array{}, py::array{}, status.detach()))
                           : SampleTypeDomainTypeReaderStatusVariant<ReaderType>(std::make_tuple(py::array{}, py::array{}));
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
            case daq::SampleType::Struct:
                return readWithDomain<StructPlaceholder>(reader, count, timeoutMs, returnStatus, dataDescriptor);
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
    static inline SampleTypeDomainTypeReaderStatusVariant<ReaderType> readWithDomain(
        const ReaderType& reader,
        size_t count,
        size_t timeoutMs,
        bool returnStatus,
        [[maybe_unused]] const daq::DataDescriptorPtr& dataDescriptor = {})
    {
        if constexpr (std::is_base_of_v<daq::TimeReaderBase, ReaderType>)
        {
            return read<ValueType, std::chrono::system_clock::time_point>(reader, count, timeoutMs, returnStatus, dataDescriptor);
        }
        else
        {
            daq::SampleType domainType = daq::SampleType::Undefined;
            reader->getDomainReadType(&domainType);

            daq::DataDescriptorPtr domainDescriptor;
            if (domainType == daq::SampleType::Undefined)
            {
                domainDescriptor = getDescriptor<ReaderType>(reader, DOMAIN_DATA_DESCRIPTOR_ATTRIBUTE);
                if (!domainDescriptor.assigned())
                {
                    auto status = readZeroValues(reader, timeoutMs);
                    assignDescriptorsFromStatus(reader, status);
                    py::gil_scoped_acquire acquire;
                    return returnStatus ? SampleTypeDomainTypeReaderStatusVariant<ReaderType>(
                                              std::make_tuple(py::array{}, py::array{}, status.detach()))
                                        : SampleTypeDomainTypeReaderStatusVariant<ReaderType>(std::make_tuple(py::array{}, py::array{}));
                }
            }

            switch (domainType)
            {
                case daq::SampleType::Float32:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Float32>::Type>(
                        reader, count, timeoutMs, returnStatus, dataDescriptor);
                case daq::SampleType::Float64:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Float64>::Type>(
                        reader, count, timeoutMs, returnStatus, dataDescriptor);
                case daq::SampleType::UInt32:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt32>::Type>(
                        reader, count, timeoutMs, returnStatus, dataDescriptor);
                case daq::SampleType::Int32:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int32>::Type>(
                        reader, count, timeoutMs, returnStatus, dataDescriptor);
                case daq::SampleType::UInt64:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt64>::Type>(
                        reader, count, timeoutMs, returnStatus, dataDescriptor);
                case daq::SampleType::Int64:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int64>::Type>(
                        reader, count, timeoutMs, returnStatus, dataDescriptor);
                case daq::SampleType::UInt8:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt8>::Type>(
                        reader, count, timeoutMs, returnStatus, dataDescriptor);
                case daq::SampleType::Int8:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int8>::Type>(
                        reader, count, timeoutMs, returnStatus, dataDescriptor);
                case daq::SampleType::UInt16:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::UInt16>::Type>(
                        reader, count, timeoutMs, returnStatus, dataDescriptor);
                case daq::SampleType::Int16:
                    return read<ValueType, daq::SampleTypeToType<daq::SampleType::Int16>::Type>(
                        reader, count, timeoutMs, returnStatus, dataDescriptor);
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
                                                                 bool returnStatus = false,
                                                                 [[maybe_unused]] const daq::DataDescriptorPtr& dataDescriptor = {})
    {
        if (count == 0)
        {
            auto status = readZeroValues(reader, timeoutMs);
            // update descriptors if changed
            assignDescriptorsFromStatus(reader, status);
            py::gil_scoped_acquire acquire;
            return returnStatus ? SampleTypeReaderStatusVariant<ReaderType>{std::make_tuple(py::array{}, status.detach())}
                                : SampleTypeReaderStatusVariant<ReaderType>{};
        }

        size_t blockSize = 1, sampleSize = 1;
        const size_t initialCount = count;
        constexpr const bool isMultiReader = std::is_base_of_v<daq::MultiReaderPtr, ReaderType>;
        constexpr const bool isSampleTypeStruct = std::is_same_v<ValueType, StructPlaceholder>;

        if constexpr (isSampleTypeStruct)
        {
            if (!dataDescriptor.assigned())
                throw std::runtime_error("Data descriptor should be assigned when sample type is Struct");
            sampleSize = dataDescriptor.getSampleSize();
        }
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
        using SampleType = typename SampleTypeToBufferType<ValueType>::Type;

        StatusType status;
        std::vector<SampleType> values(count * blockSize * sampleSize);
        if constexpr (ReaderHasReadWithTimeout<ReaderType>::value)
        {
            if constexpr (isMultiReader)
            {
                std::vector<void*> ptrs(blockSize);
                for (size_t i = 0; i < blockSize; i++)
                {
                    ptrs[i] = values.data() + i * count * sampleSize;
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

        // update descriptors if changed
        assignDescriptorsFromStatus(reader, status);

        py::gil_scoped_acquire acquire;
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
            const size_t valueSize = isSampleTypeStruct ? sampleSize : sizeof(SampleType);
            strides = {valueSize * initialCount, valueSize};
        }

        py::dtype dtype{};
        if constexpr (isSampleTypeStruct)
        {
            dtype = py::dtype::from_args(parseDataDescriptor(dataDescriptor));
        }

        return returnStatus ? SampleTypeReaderStatusVariant<ReaderType>{std::make_tuple(toPyArray(std::move(values), shape, strides, dtype),
                                                                                        status.detach())}
                            : SampleTypeReaderStatusVariant<ReaderType>{toPyArray(std::move(values), shape, strides, dtype)};
    }

    template <typename ValueType, typename DomainType, typename ReaderType>
    static inline SampleTypeDomainTypeReaderStatusVariant<ReaderType> read(
        const ReaderType& reader,
        size_t count,
        [[maybe_unused]] size_t timeoutMs,
        bool returnStatus,
        [[maybe_unused]] const daq::DataDescriptorPtr& dataDescriptor = {})
    {
        static_assert(sizeof(std::chrono::system_clock::time_point::rep) == sizeof(int64_t));

        if (count == 0)
        {
            auto status = readZeroValues(reader, timeoutMs);
            // update descriptors if changed
            assignDescriptorsFromStatus(reader, status);
            py::gil_scoped_acquire acquire;
            return returnStatus
                       ? SampleTypeDomainTypeReaderStatusVariant<ReaderType>{std::make_tuple(py::array{}, py::array{}, status.detach())}
                       : SampleTypeDomainTypeReaderStatusVariant<ReaderType>{std::make_tuple(py::array{}, py::array{})};
        }

        size_t blockSize = 1, sampleSize = 1;
        const size_t initialCount = count;
        constexpr const bool isMultiReader = std::is_base_of_v<daq::MultiReaderPtr, ReaderType>;
        constexpr const bool isValueSampleTypeStruct = std::is_same_v<ValueType, StructPlaceholder>;

        if constexpr (isValueSampleTypeStruct)
        {
            if (!dataDescriptor.assigned())
                throw std::runtime_error("Data descriptor should be assigned when sample type is Struct");
            sampleSize = dataDescriptor.getSampleSize();
        }
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
        using ValueSampleType = typename SampleTypeToBufferType<ValueType>::Type;
        using DomainSampleType = typename SampleTypeToBufferType<DomainType>::Type;
        StatusType status;
        std::vector<ValueSampleType> values(count * blockSize * sampleSize);
        std::vector<DomainSampleType> domain(count * blockSize);
        if constexpr (ReaderHasReadWithTimeout<ReaderType>::value)
        {
            if constexpr (isMultiReader)
            {
                std::vector<void*> valuesPtrs(blockSize), domainPtrs(blockSize);
                for (size_t i = 0; i < blockSize; i++)
                {
                    valuesPtrs[i] = values.data() + i * count * sampleSize;
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

        // update descriptors if changed
        assignDescriptorsFromStatus(reader, status);

        py::gil_scoped_acquire acquire;
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

        py::array::StridesContainer valuesStrides;
        py::array::StridesContainer domainStrides;
        if (blockSize > 1 && isMultiReader)
        {
            const size_t valueSize = isValueSampleTypeStruct ? sampleSize : sizeof(ValueSampleType);
            const size_t domainSize = sizeof(DomainSampleType);
            valuesStrides = {valueSize * initialCount, valueSize};
            domainStrides = {domainSize * initialCount, domainSize};
        }

        py::dtype dtype{};
        if constexpr (isValueSampleTypeStruct)
        {
            dtype = py::dtype::from_args(parseDataDescriptor(dataDescriptor));
        }

        py::dtype domainDtype;
        if constexpr (std::is_same_v<DomainType, std::chrono::system_clock::time_point>)
        {
            domainDtype = py::dtype("datetime64[ns]");
            std::transform(domain.begin(),
                           domain.end(),
                           domain.begin(),
                           [](int64_t timestamp)
                           {
                               const auto t = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(timestamp));
                               return std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
                           });
        }

        auto valuesArray = toPyArray(std::move(values), shape, valuesStrides, dtype);
        auto domainArray = toPyArray(std::move(domain), shape, domainStrides, domainDtype);

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
        if constexpr (ReaderHasReadWithTimeout<ReaderType>::value)
        {
            reader->read(nullptr, &tmpCount, timeoutMs, &status);
        }
        else
        {
            reader->read(nullptr, &tmpCount, &status);
        }
        return status;
    }

    static py::list parseDataDescriptor(const daq::DataDescriptorPtr& dataDesc)
    {
        py::list dtype;
        if (dataDesc.assigned())
        {
            for (const auto& fieldDescriptor : dataDesc.getStructFields())
            {
                auto name = py::str(fieldDescriptor.getName());
                auto fieldFormatList = py::list();  // for structs
                auto fieldFormat = py::dtype();     // for non-structs

                if (fieldDescriptor.getSampleType() == daq::SampleType::Struct)
                    fieldFormatList = parseDataDescriptor(fieldDescriptor);
                else
                    fieldFormat = py::dtype(sampleTypeToNpyType(fieldDescriptor.getSampleType()));

                // fill dimensions
                auto dimensions = fieldDescriptor.getDimensions();
                auto dimTuple = py::tuple(dimensions.assigned() ? dimensions.getCount() : 0);
                if (dimensions.assigned() && !dimensions.empty())
                {
                    for (size_t i = 0; i < dimensions.getCount(); ++i)
                    {
                        dimTuple[i] = dimensions[i].getSize();
                    }
                }

                // assign dtype
                if (fieldFormatList.empty())
                {
                    if (dimTuple.empty())
                        dtype.append(py::make_tuple(name, fieldFormat));
                    else
                        dtype.append(py::make_tuple(name, fieldFormat, dimTuple));
                }
                else
                {
                    if (dimTuple.empty())
                        dtype.append(py::make_tuple(name, fieldFormatList));
                    else
                        dtype.append(py::make_tuple(name, fieldFormatList, dimTuple));
                }
            }
        }
        return dtype;
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
                break;
            case daq::SampleType::RangeInt64:
            case daq::SampleType::ComplexFloat32:
            case daq::SampleType::ComplexFloat64:
            case daq::SampleType::String:
            default:
                throw daq::InvalidParameterException("Unsupported sample type: " + convertSampleTypeToString(type));
        }
    }

    template <typename ReaderType>
    static daq::DataDescriptorPtr getDescriptor(const ReaderType& reader, const char* attribute_name)
    {
        py::gil_scoped_acquire acquire;
        py::object pyObject = py::cast(InterfaceWrapper<typename ReaderType::DeclaredInterface>(reader.addRefAndReturn()));
        try
        {
            auto descriptor = pyObject.attr(attribute_name).template cast<daq::IDataDescriptor*>();
            return daq::DataDescriptorPtr::Borrow(descriptor);
        }
        catch (const py::error_already_set& /*error*/)
        {
        }
        return {};
    }

    template <typename ReaderType>
    static void setDescriptor(const ReaderType& reader, const char* attribute_name, daq::DataDescriptorPtr&& descriptor)
    {
        py::gil_scoped_acquire acquire;
        auto pyObject = py::cast(InterfaceWrapper<typename ReaderType::DeclaredInterface>(reader.addRefAndReturn()));
        auto pyDesc = py::cast(InterfaceWrapper(descriptor.detach()));
        pyObject.attr(attribute_name) = pyDesc;
    }

    template <typename ReaderType>
    static inline void assignDescriptorsFromStatus(const ReaderType& reader, typename daq::ReaderStatusType<ReaderType>::Type& status)
    {
        auto [dataDescriptor, domainDescriptor] = getDescriptorsFromStatus(status);
        if (dataDescriptor.assigned())
            setDescriptor(reader, VALUE_DATA_DESCRIPTOR_ATTRIBUTE, std::move(dataDescriptor));
        if (domainDescriptor.assigned())
            setDescriptor(reader, DOMAIN_DATA_DESCRIPTOR_ATTRIBUTE, std::move(domainDescriptor));
    }

    template <typename ReaderStatusType>
    static inline std::tuple<daq::DataDescriptorPtr, daq::DataDescriptorPtr> getDescriptorsFromStatus(const ReaderStatusType& status)
    {
        daq::DataDescriptorPtr valueDescriptor;
        daq::DataDescriptorPtr domainDescriptor;

        if (status.assigned() && status.getReadStatus() == daq::ReadStatus::Event)
        {
            daq::EventPacketPtr packet;
            if constexpr (std::is_same_v<daq::IMultiReaderStatus, typename ReaderStatusType::DeclaredInterface>)
            {
                packet = status.getMainDescriptor();
            }
            else
            {
                packet = status.getEventPacket();
            }

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

    static inline unsigned sampleTypeToNpyType(daq::SampleType sampleType)
    {
        switch (sampleType)
        {
            case daq::SampleType::Float32:
                return py::detail::npy_api::NPY_FLOAT_;
            case daq::SampleType::Float64:
                return py::detail::npy_api::NPY_DOUBLE_;
            case daq::SampleType::UInt8:
                return py::detail::npy_api::NPY_UINT8_;
            case daq::SampleType::Int8:
                return py::detail::npy_api::NPY_INT8_;
            case daq::SampleType::UInt16:
                return py::detail::npy_api::NPY_UINT16_;
            case daq::SampleType::Int16:
                return py::detail::npy_api::NPY_INT16_;
            case daq::SampleType::UInt32:
                return py::detail::npy_api::NPY_UINT32_;
            case daq::SampleType::Int32:
                return py::detail::npy_api::NPY_INT32_;
            case daq::SampleType::UInt64:
                return py::detail::npy_api::NPY_UINT64_;
            case daq::SampleType::Int64:
                return py::detail::npy_api::NPY_INT64_;
            case daq::SampleType::ComplexFloat32:
                return py::detail::npy_api::NPY_CFLOAT_;
            case daq::SampleType::ComplexFloat64:
                return py::detail::npy_api::NPY_CDOUBLE_;
            case daq::SampleType::String:
                return py::detail::npy_api::NPY_STRING_;
            case daq::SampleType::Struct:
                return py::detail::npy_api::NPY_VOID_;
            case daq::SampleType::Undefined:
            case daq::SampleType::RangeInt64:
            case daq::SampleType::Binary:
            default:
                throw daq::InvalidParameterException("Invalid sample type");
        }
    }
};
