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
#include <coretypes/intfs.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_rule_calc_private.h>
#include <opendaq/deleter_ptr.h>
#include <opendaq/generic_data_packet_impl.h>
#include <opendaq/range_factory.h>
#include <opendaq/reference_domain_offset_adder.h>
#include <opendaq/reusable_data_packet.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/scaling_calc_private.h>
#include <opendaq/signal_exceptions.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IDataPacket>
class DataPacketImpl : public GenericDataPacketImpl<TInterface, IReusableDataPacket>
{
public:
    using Super = GenericDataPacketImpl<TInterface, IReusableDataPacket>;

    explicit DataPacketImpl(const DataPacketPtr& domainPacket,
                            const DataDescriptorPtr& descriptor,
                            SizeT sampleCount,
                            const NumberPtr& offset);

    explicit DataPacketImpl(const DataDescriptorPtr& descriptor, SizeT sampleCount, const NumberPtr& offset);

    explicit DataPacketImpl(const DataPacketPtr& domainPacket,
                            const DataDescriptorPtr& descriptor,
                            SizeT sampleCount,
                            const NumberPtr& offset,
                            void* externalMemory,
                            const DeleterPtr& deleter,
                            SizeT bufferSize = std::numeric_limits<SizeT>::max());

    explicit DataPacketImpl(const DataPacketPtr& domainPacket,
                            const DataDescriptorPtr& descriptor,
                            SizeT sampleCount,
                            void* initialValue,
                            void* otherValues,
                            SizeT otherValueCount);

    ~DataPacketImpl() override;

    ErrCode INTERFACE_FUNC getDataDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getSampleCount(SizeT* sampleCount) override;
    ErrCode INTERFACE_FUNC getOffset(INumber** offset) override;
    ErrCode INTERFACE_FUNC getRawData(void** address) override;
    ErrCode INTERFACE_FUNC getData(void** address) override;
    ErrCode INTERFACE_FUNC getDataSize(SizeT* dataSize) override;
    ErrCode INTERFACE_FUNC getRawDataSize(SizeT* rawDataSize) override;
    ErrCode INTERFACE_FUNC getLastValue(IBaseObject** value, ITypeManager* typeManager = nullptr) override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;
    ErrCode INTERFACE_FUNC queryInterface(const IntfID& id, void** intf) override;
    ErrCode INTERFACE_FUNC borrowInterface(const IntfID& id, void** intf) const override;

    ErrCode INTERFACE_FUNC reuse(IDataDescriptor* newDescriptor,
                                 SizeT newSampleCount,
                                 INumber* newOffset,
                                 IDataPacket* newDomainPacket,
                                 Bool canReallocMemory,
                                 Bool* success) override;

private:
    bool isDataEqual(const DataPacketPtr& dataPacket) const;
    void freeMemory();
    void freeScaledData();
    void initPacket();

    DeleterPtr deleter;
    DataDescriptorPtr descriptor;
    NumberPtr offset = nullptr;
    uint32_t sampleCount;
    uint32_t sampleSize, dataSize;
    uint32_t rawSampleSize, rawDataSize;
    uint32_t memorySize;

    void* data;
    void* scaledData;

    std::mutex readLock;

    bool hasScalingCalc;
    bool hasDataRuleCalc;
    bool hasRawDataOnly;
    bool externalMemory;
    bool hasReferenceDomainOffset;

    BaseObjectPtr dataToObj(void* addr, const SampleType& type) const;
    BaseObjectPtr dataToObjAndIncreaseAddr(void*& addr, const SampleType& sampleType) const;
    StructPtr buildStructFromFields(const DataDescriptorPtr& descriptor, const TypeManagerPtr& typeManager, void*& addr) const;
    BaseObjectPtr buildFromDescriptor(void*& addr, const DataDescriptorPtr& descriptor, const TypeManagerPtr& typeManager) const;
};

template <typename TInterface>
void DataPacketImpl<TInterface>::initPacket()
{
    if (descriptor.getSampleType() == SampleType::Struct && rawSampleSize != sampleSize)
        throw InvalidParameterException("Packets with struct implicit descriptor not supported");

    const auto ruleType = descriptor.getRule().getType();

    if (ruleType == DataRuleType::Constant || (ruleType == DataRuleType::Linear && this->offset.assigned()))
        hasDataRuleCalc = descriptor.asPtr<IDataRuleCalcPrivate>(false)->hasDataRuleCalc();

    hasScalingCalc = descriptor.asPtr<IScalingCalcPrivate>(false)->hasScalingCalc();

    hasReferenceDomainOffset =
        descriptor.getReferenceDomainInfo().assigned() && descriptor.getReferenceDomainInfo().getReferenceDomainOffset().assigned();

    hasRawDataOnly = !hasScalingCalc && !hasDataRuleCalc && !hasReferenceDomainOffset;
}

template <typename TInterface>
DataPacketImpl<TInterface>::DataPacketImpl(const DataPacketPtr& domainPacket,
                                           const DataDescriptorPtr& descriptor,
                                           SizeT sampleCount,
                                           const NumberPtr& offset)
    : Super(domainPacket)
    , descriptor(descriptor)
    , offset(offset)
    , sampleCount(sampleCount)
    , hasScalingCalc(false)
    , hasDataRuleCalc(false)
    , hasRawDataOnly(true)
    , externalMemory(false)
    , hasReferenceDomainOffset(false)
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
        data = std::malloc(rawDataSize);

        if (data == nullptr)
            throw NoMemoryException();
    }
    memorySize = rawDataSize;

    initPacket();
}

template <typename TInterface>
DataPacketImpl<TInterface>::DataPacketImpl(const DataPacketPtr& domainPacket,
                                           const DataDescriptorPtr& descriptor,
                                           SizeT sampleCount,
                                           const NumberPtr& offset,
                                           void* externalMemory,
                                           const DeleterPtr& deleter,
                                           SizeT bufferSize)
    : Super(domainPacket)
    , deleter(deleter)
    , descriptor(descriptor)
    , offset(offset)
    , sampleCount(sampleCount)
    , hasScalingCalc(false)
    , hasDataRuleCalc(false)
    , hasRawDataOnly(true)
    , externalMemory(true)
    , hasReferenceDomainOffset(false)
{
    scaledData = nullptr;
    data = nullptr;

    if (!descriptor.assigned())
        throw ArgumentNullException("Data descriptor in packet is null.");

    if (!deleter.assigned())
        throw ArgumentNullException("Deleter must be assigned.");

    sampleSize = descriptor.getSampleSize();
    rawSampleSize = descriptor.getRawSampleSize();
    dataSize = sampleCount * sampleSize;

    if (bufferSize == std::numeric_limits<SizeT>::max())
        rawDataSize = sampleCount * rawSampleSize;
    else
        rawDataSize = bufferSize;

    memorySize = rawDataSize;
    data = externalMemory;

    initPacket();
}

template <typename TInterface>
DataPacketImpl<TInterface>::DataPacketImpl(const DataDescriptorPtr& descriptor, SizeT sampleCount, const NumberPtr& offset)
    : DataPacketImpl<TInterface>(nullptr, descriptor, sampleCount, offset)
{
}

template <typename TInterface>
DataPacketImpl<TInterface>::DataPacketImpl(const DataPacketPtr& domainPacket,
                                           const DataDescriptorPtr& descriptor,
                                           SizeT sampleCount,
                                           void* initialValue,
                                           void* otherValues,
                                           SizeT otherValueCount)
    : Super(domainPacket)
    , descriptor(descriptor)
    , sampleCount(sampleCount)
    , hasRawDataOnly(true)
    , externalMemory(false)
{
    scaledData = nullptr;
    data = nullptr;

    if (!descriptor.assigned())
        throw ArgumentNullException("Data descriptor in packet is null.");
    const auto ruleType = descriptor.getRule().getType();
    if (ruleType != DataRuleType::Constant)
        throw InvalidParameterException("Data rule must be constant.");

    sampleSize = descriptor.getSampleSize();
    dataSize = sampleCount * sampleSize;
    const auto structSize = sampleSize + sizeof(uint32_t);

    rawDataSize = sampleSize + structSize * otherValueCount;
    data = std::malloc(rawDataSize);
    std::memcpy(data, initialValue, sampleSize);
    if (otherValueCount > 0)
        std::memcpy(reinterpret_cast<uint8_t*>(data) + sampleSize, otherValues, otherValueCount * structSize);

    hasDataRuleCalc = true;
    hasScalingCalc = descriptor.asPtr<IScalingCalcPrivate>(false)->hasScalingCalc();
    if (hasScalingCalc)
        throw InvalidParameterException("Constant data rule with post scaling not supported.");
    hasRawDataOnly = false;

    hasReferenceDomainOffset =
        descriptor.getReferenceDomainInfo().assigned() && descriptor.getReferenceDomainInfo().getReferenceDomainOffset().assigned();
}

template <typename TInterface>
DataPacketImpl<TInterface>::~DataPacketImpl()
{
    freeMemory();
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
                        scaledData = descriptor.asPtr<IDataRuleCalcPrivate>(false)->calculateRule(offset, sampleCount, data, rawDataSize);
                    }

                    if (hasReferenceDomainOffset)
                    {
                        auto referenceDomainOffsetAdder = std::unique_ptr<ReferenceDomainOffsetAdder>(createReferenceDomainOffsetAdderTyped(
                            descriptor.getSampleType(), descriptor.getReferenceDomainInfo().getReferenceDomainOffset(), sampleCount));

                        if (data)
                        {
                            // Explicit data rule, apply Reference Domain Offset
                            // Uses malloc to create a new array
                            scaledData = referenceDomainOffsetAdder->addReferenceDomainOffset(data);
                        }
                        else
                        {
                            // Linear data rule, apply Reference Domain Offset
                            // Modifies existing array
                            referenceDomainOffsetAdder->addReferenceDomainOffset(&scaledData);
                        }
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
    {
        if (descriptor.getRule().getType() == DataRuleType::Constant)
            return false;
        else
            throw InvalidSampleTypeException();
    }

    return data == dataPacket.getRawData() || std::memcmp(data, dataPacket.getRawData(), rawDataSize) == 0;
}

template <typename TInterface>
inline BaseObjectPtr DataPacketImpl<TInterface>::dataToObj(void* addr, const SampleType& type) const
{
    switch (type)
    {
        case SampleType::Float32:
        {
            const auto data = static_cast<float*>(addr);
            return Floating(*data);
        }
        case SampleType::Float64:
        {
            const auto data = static_cast<double*>(addr);
            return Floating(*data);
        }
        case SampleType::Int8:
        {
            const auto data = static_cast<int8_t*>(addr);
            return Integer(*data);
        }
        case SampleType::UInt8:
        {
            const auto data = static_cast<uint8_t*>(addr);
            return Integer(*data);
        }
        case SampleType::Int16:
        {
            const auto data = static_cast<int16_t*>(addr);
            return Integer(*data);
        }
        case SampleType::UInt16:
        {
            const auto data = static_cast<uint16_t*>(addr);
            return Integer(*data);
        }
        case SampleType::Int32:
        {
            const auto data = static_cast<int32_t*>(addr);
            return Integer(*data);
        }
        case SampleType::UInt32:
        {
            const auto data = static_cast<uint32_t*>(addr);
            return Integer(*data);
        }
        case SampleType::Int64:
        {
            const auto data = static_cast<int64_t*>(addr);
            return Integer(*data);
        }
        case SampleType::UInt64:
        {
            const auto data = static_cast<uint64_t*>(addr);
            return Integer(*data);
        }
        case SampleType::RangeInt64:
        {
            const auto data = static_cast<int64_t*>(addr);
            return Range(data[0], data[1]);
        }
        case SampleType::ComplexFloat32:
        {
            const auto data = static_cast<float*>(addr);
            return ComplexNumber(data[0], data[1]);
        }
        case SampleType::ComplexFloat64:
        {
            const auto data = static_cast<double*>(addr);
            return ComplexNumber(data[0], data[1]);
        }
        default:
        {
            return BaseObject();
        }
    }
}

template <typename TInterface>
inline BaseObjectPtr DataPacketImpl<TInterface>::dataToObjAndIncreaseAddr(void*& addr, const SampleType& sampleType) const
{
    const auto ptr = dataToObj(addr, sampleType);
    addr = static_cast<char*>(addr) + getSampleSize(sampleType);
    return ptr;
}

template <typename TInterface>
inline StructPtr DataPacketImpl<TInterface>::buildStructFromFields(const DataDescriptorPtr& descriptor,
                                                                   const TypeManagerPtr& typeManager,
                                                                   void*& addr) const
{
    const auto builder = StructBuilder(descriptor.getName(), typeManager);
    const auto fields = descriptor.getStructFields();
    for (const auto& field : fields)
    {
        const auto ptr = buildFromDescriptor(addr, field, typeManager);
        builder.set(field.getName(), ptr);
    }
    return builder.build();
}

template <typename TInterface>
inline BaseObjectPtr DataPacketImpl<TInterface>::buildFromDescriptor(void*& addr,
                                                                     const DataDescriptorPtr& descriptor,
                                                                     const TypeManagerPtr& typeManager) const
{
    const auto dimensions = descriptor.getDimensions();

    if (!dimensions.assigned())
        throw NotAssignedException{"Dimensions of data descriptor not assigned."};

    const auto dimensionCount = dimensions.getCount();

    if (dimensionCount > 1)
        throw NotSupportedException{"getLastValue on packets with dimensions supports only up to one dimension."};

    const auto sampleType = descriptor.getSampleType();

    if (dimensionCount == 1)
    {
        // List
        auto listPtr = List<IBaseObject>();
        const auto size = dimensions.getItemAt(0).getSize();

        for (size_t i = 0; i < size; i++)
        {
            if (sampleType == SampleType::Struct)
            {
                // Struct
                listPtr.pushBack(buildStructFromFields(descriptor, typeManager, addr));
            }
            else
            {
                // Not struct
                listPtr.pushBack(dataToObjAndIncreaseAddr(addr, sampleType));
            }
        }
        return listPtr;
    }
    else
    {
        // Not list
        if (sampleType == SampleType::Struct)
        {
            // Struct
            return buildStructFromFields(descriptor, typeManager, addr);
        }
        // Not struct
        return dataToObjAndIncreaseAddr(addr, sampleType);
    }
}

template <typename TInterface>
ErrCode DataPacketImpl<TInterface>::getLastValue(IBaseObject** value, ITypeManager* typeManager)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    const auto dimensionCount = descriptor.getDimensions().getCount();

    if (dimensionCount > 1)
        return OPENDAQ_IGNORED;

    void* addr;
    ErrCode err = this->getData(&addr);
    if (OPENDAQ_FAILED(err))
        return err;

    addr = static_cast<char*>(addr) + (sampleCount - 1) * descriptor.getSampleSize();

    return daqTry(
        [&]()
        {
            auto ptr = buildFromDescriptor(addr, descriptor, typeManager);
            *value = ptr.detach();
            return OPENDAQ_SUCCESS;
        });
}

template <typename TInterface>
ErrCode DataPacketImpl<TInterface>::queryInterface(const IntfID& id, void** intf)
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == IDataPacket::Id)
    {
        *intf = static_cast<IDataPacket*>(this);
        this->addRef();

        return OPENDAQ_SUCCESS;
    }

    if (id == IPacket::Id)
    {
        *intf = static_cast<IPacket*>(this);
        this->addRef();

        return OPENDAQ_SUCCESS;
    }

    if (id == IReusableDataPacket::Id)
    {
        *intf = static_cast<IReusableDataPacket*>(this);
        this->addRef();

        return OPENDAQ_SUCCESS;
    }

    return Super::queryInterface(id, intf);
}

template <typename TInterface>
ErrCode DataPacketImpl<TInterface>::borrowInterface(const IntfID& id, void** intf) const
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == IDataPacket::Id)
    {
        *intf = const_cast<IDataPacket*>(static_cast<const IDataPacket*>(this));

        return OPENDAQ_SUCCESS;
    }

    if (id == IPacket::Id)
    {
        *intf = const_cast<IPacket*>(static_cast<const IPacket*>(this));

        return OPENDAQ_SUCCESS;
    }

    if (id == IReusableDataPacket::Id)
    {
        *intf = const_cast<IReusableDataPacket*>(static_cast<const IReusableDataPacket*>(this));

        return OPENDAQ_SUCCESS;
    }

    return Super::borrowInterface(id, intf);
}

template <typename TInterface>
ErrCode DataPacketImpl<TInterface>::reuse(IDataDescriptor* newDescriptor,
                                          SizeT newSampleCount,
                                          INumber* newOffset,
                                          IDataPacket* newDomainPacket,
                                          Bool canReallocMemory,
                                          Bool* success)
{
    OPENDAQ_PARAM_NOT_NULL(success);

    if (newSampleCount == std::numeric_limits<SizeT>::max())
        newSampleCount = sampleCount;

    SizeT newRawSampleSize;
    const auto newDescriptorPtr = DataDescriptorPtr::Borrow(newDescriptor);
    if (newDescriptorPtr.assigned())
        newRawSampleSize = newDescriptorPtr.getRawSampleSize();
    else
        newRawSampleSize = descriptor.getRawSampleSize();
    SizeT newRawDataSize = newSampleCount * newRawSampleSize;

    if (newRawDataSize > memorySize)
    {
        if (!canReallocMemory || externalMemory)
        {
            *success = False;
            return OPENDAQ_IGNORED;
        }

        this->callDestructCallbacks();

        freeMemory();
        scaledData = nullptr;

        memorySize = newRawDataSize;
        data = std::malloc(newRawDataSize);

        if (data == nullptr)
            throw NoMemoryException();
    }
    else
    {
        this->callDestructCallbacks();
    }

    this->packetId = generatePacketId();

    sampleCount = newSampleCount;
    rawSampleSize = newRawSampleSize;
    rawDataSize = newRawDataSize;
    if (newDescriptorPtr.assigned())
        descriptor = newDescriptorPtr;
    if (newOffset != nullptr)
        offset = newOffset;
    if (newDomainPacket != nullptr)
        this->domainPacket = newDomainPacket;

    sampleSize = descriptor.getSampleSize();
    dataSize = sampleCount * sampleSize;

    *success = True;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface>
void DataPacketImpl<TInterface>::freeScaledData()
{
    std::free(scaledData);
}

template <typename TInterface>
void DataPacketImpl<TInterface>::freeMemory()
{
    if (externalMemory)
    {
        deleter.deleteMemory(data);
    }
    else
    {
        std::free(data);
    }
    freeScaledData();
}

END_NAMESPACE_OPENDAQ
