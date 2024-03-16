#include <opendaq/data_packet_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

DataPacketImpl::DataPacketImpl(const DataPacketPtr& domainPacket,
                               const DataDescriptorPtr& descriptor,
                               SizeT sampleCount,
                               const NumberPtr& offset,
                               AllocatorPtr allocator)
    : Super(domainPacket)
    , allocator(std::move(allocator))
    , descriptor(descriptor)
    , offset(offset)
    , sampleCount(sampleCount)
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
        memorySize = rawDataSize;

        if (this->allocator.assigned())
            data = this->allocator.allocate(descriptor, memorySize, rawSampleSize);
        else
            data = std::malloc(memorySize);

        if (data == nullptr)
            throw NoMemoryException();
    }

    initPacket();
}

DataPacketImpl::DataPacketImpl(const DataDescriptorPtr& descriptor, SizeT sampleCount, const NumberPtr& offset, AllocatorPtr allocator)
    : DataPacketImpl(nullptr, descriptor, sampleCount, offset, std::move(allocator))
{
}

DataPacketImpl::~DataPacketImpl()
{
    freeMemory();
}

void DataPacketImpl::initPacket()
{
    if (descriptor.getSampleType() == SampleType::Struct && rawSampleSize != sampleSize)
        throw InvalidParameterException("Packets with struct implicit descriptor not supported");

    const auto ruleType = descriptor.getRule().getType();

    if (ruleType == DataRuleType::Constant || (ruleType == DataRuleType::Linear && this->offset.assigned()))
        hasDataRuleCalc = descriptor.asPtr<IDataRuleCalcPrivate>(false)->hasDataRuleCalc();

    hasScalingCalc = descriptor.asPtr<IScalingCalcPrivate>(false)->hasScalingCalc();

    hasRawDataOnly = !hasScalingCalc && !hasDataRuleCalc;
}

ErrCode DataPacketImpl::getDataDescriptor(IDataDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    *descriptor = this->descriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataPacketImpl::getSampleCount(SizeT* sampleCount)
{
    OPENDAQ_PARAM_NOT_NULL(sampleCount);

    *sampleCount = this->sampleCount;
    return OPENDAQ_SUCCESS;
}

ErrCode DataPacketImpl::getOffset(INumber** offset)
{
    OPENDAQ_PARAM_NOT_NULL(offset);

    *offset = this->offset.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataPacketImpl::getRawData(void** address)
{
    OPENDAQ_PARAM_NOT_NULL(address);

    *address = data;
    return OPENDAQ_SUCCESS;
}

ErrCode DataPacketImpl::getData(void** address)
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

ErrCode DataPacketImpl::getDataSize(SizeT* dataSize)
{
    OPENDAQ_PARAM_NOT_NULL(dataSize);

    *dataSize = this->dataSize;
    return OPENDAQ_SUCCESS;
}

ErrCode DataPacketImpl::getRawDataSize(SizeT* rawDataSize)
{
    OPENDAQ_PARAM_NOT_NULL(rawDataSize);

    *rawDataSize = this->rawDataSize;
    return OPENDAQ_SUCCESS;
}

ErrCode DataPacketImpl::equals(IBaseObject* other, Bool* equals) const
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

ErrCode DataPacketImpl::queryInterface(const IntfID& id, void** intf)
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

ErrCode DataPacketImpl::borrowInterface(const IntfID& id, void** intf) const
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

ErrCode DataPacketImpl::reuse(IDataDescriptor* newDescriptor, SizeT newSampleCount,
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
        if (!canReallocMemory || allocator.assigned())
        {
            *success = False;
            return OPENDAQ_IGNORED;
        }

        callDestructCallbacks();

        freeMemory();
        scaledData = nullptr;

        memorySize = newRawDataSize;
        data = std::malloc(newRawDataSize);

        if (data == nullptr)
            throw NoMemoryException();
    }
    else
    {
        callDestructCallbacks();
    }
    
    packetId = generatePacketId();

    sampleCount = newSampleCount;
    rawSampleSize = newRawSampleSize;
    rawDataSize = newRawDataSize;
    if (newDescriptorPtr.assigned())
        descriptor = newDescriptorPtr;
    if (newOffset != nullptr)
        offset = newOffset;
    if (newDomainPacket != nullptr)
        domainPacket = newDomainPacket;

    sampleSize = descriptor.getSampleSize();
    dataSize = sampleCount * sampleSize;

    *success = True;
    return OPENDAQ_SUCCESS;
}

bool DataPacketImpl::isDataEqual(const DataPacketPtr& dataPacket) const
{
    if (rawDataSize != dataPacket.getRawDataSize())
        throw InvalidSampleTypeException();

    return data == dataPacket.getRawData() || std::memcmp(data, dataPacket.getRawData(), rawDataSize) == 0;
}

void DataPacketImpl::freeScaledData()
{
    std::free(scaledData);
}

void DataPacketImpl::freeMemory()
{
    if (allocator.assigned())
    {
        allocator.free(data);
    }
    else
    {
        std::free(data);
    }
    freeScaledData();
}

ErrCode DataPacketImpl::getLastValue(IBaseObject** value)
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
        default:
        {
            return OPENDAQ_IGNORED;
        }
    };
    return OPENDAQ_SUCCESS;
}


OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, DataPacketImpl,
    IDataPacket, createDataPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleCount,
    INumber*, offset,
    IAllocator*, allocator
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, DataPacketImpl,
    IDataPacket, createDataPacketWithDomain,
    IDataPacket*, domainPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleCount,
    INumber*, offset,
    IAllocator*, allocator
)

END_NAMESPACE_OPENDAQ
