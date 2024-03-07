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

#pragma once
#include <coretypes/intfs.h>
#include <opendaq/allocator_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_rule_calc_private.h>
#include <opendaq/generic_data_packet_impl.h>
#include <opendaq/range_factory.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/scaling_calc_private.h>
#include <opendaq/signal_exceptions.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IDataPacket>
class DataPacketImpl : public GenericDataPacketImpl<TInterface>
{
public:
    using Super = PacketImpl<TInterface>;

    explicit DataPacketImpl(const DataPacketPtr& domainPacket,
                            const DataDescriptorPtr& descriptor,
                            SizeT sampleCount,
                            const NumberPtr& offset,
                            AllocatorPtr allocator);
    explicit DataPacketImpl(const DataDescriptorPtr& descriptor, SizeT sampleCount, const NumberPtr& offset, AllocatorPtr allocator);
    ~DataPacketImpl() override;

    ErrCode INTERFACE_FUNC getDataDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getSampleCount(SizeT* sampleCount) override;
    ErrCode INTERFACE_FUNC getOffset(INumber** offset) override;
    ErrCode INTERFACE_FUNC getRawData(void** address) override;
    ErrCode INTERFACE_FUNC getData(void** address) override;
    ErrCode INTERFACE_FUNC getDataSize(SizeT* dataSize) override;
    ErrCode INTERFACE_FUNC getRawDataSize(SizeT* rawDataSize) override;
    ErrCode INTERFACE_FUNC getLastValue(IBaseObject** value) override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

private:
    bool isDataEqual(const DataPacketPtr& dataPacket) const;

    StructTypePtr createStructTypeFromDescriptor(DataDescriptorPtr dataDescriptor) const;

    AllocatorPtr allocator;
    DataDescriptorPtr descriptor;
    SizeT sampleCount;
    NumberPtr offset = nullptr;
    SizeT sampleSize, dataSize;
    SizeT rawSampleSize, rawDataSize;

    void* data;
    void* scaledData;

    std::mutex readLock;

    bool hasScalingCalc;
    bool hasDataRuleCalc;
    bool hasRawDataOnly;
};

template <typename TInterface>
DataPacketImpl<TInterface>::DataPacketImpl(const DataPacketPtr& domainPacket,
                                           const DataDescriptorPtr& descriptor,
                                           SizeT sampleCount,
                                           const NumberPtr& offset,
                                           AllocatorPtr allocator)
    : GenericDataPacketImpl<TInterface>(domainPacket)
    , allocator(std::move(allocator))
    , descriptor(descriptor)
    , sampleCount(sampleCount)
    , offset(offset)
    , hasScalingCalc(false)
    , hasDataRuleCalc(false)
    , hasRawDataOnly(true)
{
    scaledData = nullptr;
    data = nullptr;

    if (!descriptor.assigned())
        throw ArgumentNullException("Data descriptor in packet is null.");

    sampleSize = descriptor.getSampleSize();
    rawSampleSize = descriptor.getRawSampleSize();
    dataSize = sampleCount * sampleSize;
    rawDataSize = sampleCount * rawSampleSize;

    if (rawDataSize > 0)
    {
        if (this->allocator.assigned())
            data = this->allocator.allocate(descriptor, rawDataSize, rawSampleSize);
        else
            data = std::malloc(rawDataSize);

        if (data == nullptr)
            throw NoMemoryException();
    }

    if (descriptor.getSampleType() == SampleType::Struct && rawSampleSize != sampleSize)
        throw InvalidParameterException("Packets with struct implicit descriptor not supported");

    const auto ruleType = descriptor.getRule().getType();

    if (ruleType == DataRuleType::Constant || (ruleType == DataRuleType::Linear && this->offset.assigned()))
        hasDataRuleCalc = descriptor.asPtr<IDataRuleCalcPrivate>(false)->hasDataRuleCalc();

    hasScalingCalc = descriptor.asPtr<IScalingCalcPrivate>(false)->hasScalingCalc();

    hasRawDataOnly = !hasScalingCalc && !hasDataRuleCalc;
}

template <typename TInterface>
DataPacketImpl<TInterface>::DataPacketImpl(const DataDescriptorPtr& descriptor,
                                           SizeT sampleCount,
                                           const NumberPtr& offset,
                                           AllocatorPtr allocator)
    : DataPacketImpl<TInterface>(nullptr, descriptor, sampleCount, offset, std::move(allocator))
{
}

template <typename TInterface>
DataPacketImpl<TInterface>::~DataPacketImpl()
{
    if (allocator.assigned())
    {
        allocator.free(data);
    }
    else
    {
        std::free(data);
    }
    std::free(scaledData);
}

template <typename TInterface>
ErrCode DataPacketImpl<TInterface>::getDataDescriptor(IDataDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    *descriptor = this->descriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface>
ErrCode DataPacketImpl<TInterface>::getSampleCount(SizeT* sampleCount)
{
    OPENDAQ_PARAM_NOT_NULL(sampleCount);

    *sampleCount = this->sampleCount;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface>
ErrCode DataPacketImpl<TInterface>::getOffset(INumber** offset)
{
    OPENDAQ_PARAM_NOT_NULL(offset);

    *offset = this->offset.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface>
ErrCode DataPacketImpl<TInterface>::getRawData(void** address)
{
    OPENDAQ_PARAM_NOT_NULL(address);

    *address = data;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface>
ErrCode DataPacketImpl<TInterface>::getData(void** address)
{
    OPENDAQ_PARAM_NOT_NULL(address);

    if (hasRawDataOnly)
    {
        *address = data;
        return OPENDAQ_SUCCESS;
    }

    readLock.lock();

    if (scaledData)
    {
        *address = scaledData;
    }
    else
    {
        if (sampleCount == 0)
            *address = nullptr;
        else
            daqTry(
                [&]()
                {
                    if (hasScalingCalc)
                    {
                        scaledData = descriptor.asPtr<IScalingCalcPrivate>(false)->scaleData(data, sampleCount);
                    }
                    else if (hasDataRuleCalc)
                    {
                        scaledData = descriptor.asPtr<IDataRuleCalcPrivate>(false)->calculateRule(offset, sampleCount);
                    }

                    *address = scaledData;
                    return OPENDAQ_SUCCESS;
                });
    }

    readLock.unlock();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface>
ErrCode INTERFACE_FUNC DataPacketImpl<TInterface>::getDataSize(SizeT* dataSize)
{
    OPENDAQ_PARAM_NOT_NULL(dataSize);

    *dataSize = this->dataSize;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface>
ErrCode INTERFACE_FUNC DataPacketImpl<TInterface>::getRawDataSize(SizeT* rawDataSize)
{
    OPENDAQ_PARAM_NOT_NULL(rawDataSize);

    *rawDataSize = this->rawDataSize;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface>
ErrCode INTERFACE_FUNC DataPacketImpl<TInterface>::equals(IBaseObject* other, Bool* equals) const
{
    if (equals == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equals = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    return daqTry(
        [this, &other, &equals]()
        {
            ErrCode errCode = Super::equals(other, equals);
            checkErrorInfo(errCode);

            if (!(*equals))
                return errCode;

            *equals = false;
            const DataPacketPtr packetOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IDataPacket>();
            if (packetOther == nullptr)
                return errCode;

            if (!BaseObjectPtr::Equals(this->domainPacket, packetOther.getDomainPacket()))
                return errCode;
            if (!BaseObjectPtr::Equals(this->descriptor, packetOther.getDataDescriptor()))
                return errCode;
            if (this->sampleCount != packetOther.getSampleCount())
                return errCode;
            if (this->offset != packetOther.getOffset())
                return errCode;
            if (!this->isDataEqual(packetOther))
                return errCode;

            *equals = true;
            return errCode;
        });
}

template <typename TInterface>
bool DataPacketImpl<TInterface>::isDataEqual(const DataPacketPtr& dataPacket) const
{
    if (rawDataSize != dataPacket.getRawDataSize())
        throw InvalidSampleTypeException();

    return data == dataPacket.getRawData() || std::memcmp(data, dataPacket.getRawData(), rawDataSize) == 0;
}

template <typename TInterface>
inline StructTypePtr DataPacketImpl<TInterface>::createStructTypeFromDescriptor(DataDescriptorPtr dataDescriptor) const
{
    auto fields = dataDescriptor.getStructFields();
    auto fieldNames = List<IString>();
    auto fieldTypes = List<IType>();
    for (auto field& : fields)
    {
    }

    StructTypePtr structType = StructType(dataDescriptor.getName(), fieldNames, fieldTypes);

    return StructTypePtr();
}

template <typename TInterface>
ErrCode DataPacketImpl<TInterface>::getLastValue(IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    if (descriptor.getDimensions().getCount() != 0)
        return OPENDAQ_IGNORED;

    {
        auto descriptorStructFields = descriptor.getStructFields();
        if (descriptorStructFields.assigned() && !descriptorStructFields.empty())
            return OPENDAQ_IGNORED;
    }

    void* addr;
    ErrCode err = this->getData(&addr);
    if (OPENDAQ_FAILED(err))
        return err;

    auto idx = sampleCount - 1;

    switch (descriptor.getSampleType())
    {
        case SampleType::Float32:
        {
            auto data = static_cast<float*>(addr);
            *value = Floating(data[idx]).detach();
            break;
        }
        case SampleType::Float64:
        {
            auto data = static_cast<double*>(addr);
            *value = Floating(data[idx]).detach();
            break;
        }
        case SampleType::Int8:
        {
            auto data = static_cast<int8_t*>(addr);
            *value = Integer(data[idx]).detach();
            break;
        }
        case SampleType::UInt8:
        {
            auto data = static_cast<uint8_t*>(addr);
            *value = Integer(data[idx]).detach();
            break;
        }
        case SampleType::Int16:
        {
            auto data = static_cast<int16_t*>(addr);
            *value = Integer(data[idx]).detach();
            break;
        }
        case SampleType::UInt16:
        {
            auto data = static_cast<uint16_t*>(addr);
            *value = Integer(data[idx]).detach();
            break;
        }
        case SampleType::Int32:
        {
            auto data = static_cast<int32_t*>(addr);
            *value = Integer(data[idx]).detach();
            break;
        }
        case SampleType::UInt32:
        {
            auto data = static_cast<uint32_t*>(addr);
            *value = Integer(data[idx]).detach();
            break;
        }
        case SampleType::Int64:
        {
            auto data = static_cast<int64_t*>(addr);
            *value = Integer(data[idx]).detach();
            break;
        }
        case SampleType::UInt64:
        {
            auto data = static_cast<uint64_t*>(addr);
            *value = Integer(data[idx]).detach();
            break;
        }
        case SampleType::RangeInt64:
        {
            auto data = static_cast<int64_t*>(addr);
            *value = Range(data[idx * 2], data[idx * 2 + 1]).detach();
            break;
        }
        case SampleType::ComplexFloat32:
        {
            auto data = static_cast<float*>(addr);
            *value = ComplexNumber(data[idx * 2], data[idx * 2 + 1]).detach();
            break;
        }
        case SampleType::ComplexFloat64:
        {
            auto data = static_cast<double*>(addr);
            *value = ComplexNumber(data[idx * 2], data[idx * 2 + 1]).detach();
            break;
        }
        case SampleType::Struct:
        {
            const auto typeManager = TypeManager();
            // const auto structType = createStructTypeFromDescriptor();
            // typeManager.addType(structType);

            break;
        }
        default:
        {
            return OPENDAQ_IGNORED;
        }
    };
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
