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
#include <opendaq/generic_data_packet_impl.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/allocator_ptr.h>
#include <coretypes/intfs.h>
#include <opendaq/scaling_calc_private.h>
#include <opendaq/data_rule_calc_private.h>
#include <opendaq/signal_exceptions.h>
#include <opendaq/sample_type_traits.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IDataPacket>
class DataPacketImpl : public GenericDataPacketImpl<TInterface>
{
public:
    using Super = PacketImpl<TInterface>;

    explicit DataPacketImpl(const DataPacketPtr& domainPacket, const DataDescriptorPtr& descriptor, SizeT sampleCount, const NumberPtr& offset, AllocatorPtr allocator);
    explicit DataPacketImpl(const DataDescriptorPtr& descriptor, SizeT sampleCount, const NumberPtr& offset, AllocatorPtr allocator);
    ~DataPacketImpl() override;

    ErrCode INTERFACE_FUNC getDataDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getSampleCount(SizeT* sampleCount) override;
    ErrCode INTERFACE_FUNC getOffset(INumber** offset) override;
    ErrCode INTERFACE_FUNC getRawData(void** address) override;
    ErrCode INTERFACE_FUNC getData(void** address) override;
    ErrCode INTERFACE_FUNC getSampleMemSize(SizeT* sampleMemSize) override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

private:
    void calculateSampleMemSize();
    bool isDataEqual(const DataPacketPtr& dataPacket) const;

    AllocatorPtr allocator;
    DataDescriptorPtr descriptor;
    SizeT sampleCount;
    NumberPtr offset = nullptr;
    SizeT sampleMemSize = 0;

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

    calculateSampleMemSize();

    if (descriptor.isStructDescriptor())
    {
        if (this->allocator.assigned())
            data = this->allocator.allocate(descriptor, sampleCount * this->sampleMemSize, this->sampleMemSize);
        else
            data = std::malloc(sampleCount * this->sampleMemSize);

        if (data == nullptr)
            throw NoMemoryException();

        return;
    }

    const DataRuleType ruleType = descriptor.getRule().getType();
    if (ruleType == DataRuleType::Explicit)
    {
        if (this->allocator.assigned())
            data = this->allocator.allocate(descriptor, sampleCount * this->sampleMemSize, this->sampleMemSize);
        else
            data = std::malloc(sampleCount * this->sampleMemSize);

        if (data == nullptr)
            throw NoMemoryException();
    }

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
        daqTry([&]() {
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
ErrCode INTERFACE_FUNC DataPacketImpl<TInterface>::getSampleMemSize(SizeT* sampleMemSize)
{
    *sampleMemSize = this->sampleMemSize;
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

    return daqTry([this, &other, &equals]() {
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
void DataPacketImpl<TInterface>::calculateSampleMemSize()
{
    // todo: we need to adapt this method to work with struct descriptors as well
    if (!descriptor.isStructDescriptor())
    {
        auto type = descriptor.getSampleType();
        if (descriptor.getPostScaling().assigned())
            type = descriptor.getPostScaling().getInputSampleType();
        sampleMemSize = getSampleSize(type);
    }
}

template <typename TInterface>
bool DataPacketImpl<TInterface>::isDataEqual(const DataPacketPtr& dataPacket) const
{
    if (sampleMemSize == 0 || dataPacket.getSampleMemSize() == 0)
        throw InvalidSampleTypeException();

    const size_t memSize = sampleMemSize * sampleCount;
    const size_t otherMemSize = dataPacket.getSampleMemSize() * dataPacket.getSampleCount();
    if (memSize != otherMemSize)
        return false;

    return data == dataPacket.getRawData() || std::memcmp(data, dataPacket.getRawData(), memSize) == 0;
}

END_NAMESPACE_OPENDAQ
