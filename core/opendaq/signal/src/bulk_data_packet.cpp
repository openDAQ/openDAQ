#include <opendaq/data_packet_impl.h>
#include <coretypes/integer_impl.h>
#include <opendaq/deleter_factory.h>
#include <opendaq/bulk_data_packet.h>
#include <atomic>

BEGIN_NAMESPACE_OPENDAQ

template <class Impl, class OnDestruct>
class BulkImpl : public Impl
{
public:
    using Impl::Impl;

    int INTERFACE_FUNC releaseRef() override
    {
        const auto newRefCount = this->internalReleaseRef();
        assert(newRefCount >= 0);
        if (newRefCount == 0)
        {
            Impl::checkAndCallDispose();
            // do not call delete
            onDestruct();
        }

        return newRefCount;
    }

    template <class F>
    void setDestruct(F&& f)
    {
        onDestruct = std::forward<F>(f);
    }

private:
    OnDestruct onDestruct;
};

struct BulkDestruct;

using BulkDataPacketImpl = BulkImpl<DataPacketImpl<>, BulkDestruct>;
using BulkIntegerImpl = BulkImpl<IntegerImpl, BulkDestruct>;

struct BulkDestruct
{
    void* memory;

    void operator()() const
    {
        assert(memory != nullptr);

        std::atomic<int>* refCount = static_cast<std::atomic<int>*>(memory);
        const auto newRefCount = std::atomic_fetch_sub_explicit(refCount, 1, std::memory_order_acq_rel) - 1;
        if (newRefCount == 0)
            destruct();
    }

    void destruct() const
    {
        uint8_t* memPtr = static_cast<uint8_t*>(memory) + sizeof(std::atomic<int>);
        const auto count = *reinterpret_cast<size_t*>(memPtr);
        memPtr += sizeof(size_t);
        const auto offsets = static_cast<bool>(*reinterpret_cast<size_t*>(memPtr));
        memPtr += sizeof(size_t);
        for (size_t i = 0; i < count; i++)
        {
            if (offsets)
            {
                reinterpret_cast<BulkIntegerImpl*>(memPtr)->~BulkIntegerImpl();
                memPtr += sizeof(BulkIntegerImpl);
            }
            reinterpret_cast<BulkDataPacketImpl*>(memPtr)->~BulkDataPacketImpl();
            memPtr += sizeof(BulkDataPacketImpl);
            reinterpret_cast<BulkDataPacketImpl*>(memPtr)->~BulkDataPacketImpl();
            memPtr += sizeof(BulkDataPacketImpl);
        }

        std::free(memory);
    }
};

static size_t alignTo(size_t value, size_t align)
{
    return (value + align - 1) / align * align;
}

extern "C" PUBLIC_EXPORT ErrCode daqBulkCreateDataPackets(
    IDataPacket** dataPackets,
    IDataDescriptor** valueDescriptors,
    IDataDescriptor** domainDescriptors,
    size_t* sampleCounts,
    int64_t* offsets,
    size_t count,
    size_t dataAlign)
{
    OPENDAQ_PARAM_NOT_NULL(dataPackets);
    OPENDAQ_PARAM_NOT_NULL(valueDescriptors);
    OPENDAQ_PARAM_NOT_NULL(sampleCounts);
    OPENDAQ_PARAM_GE(count, 1);
    OPENDAQ_PARAM_GE(dataAlign, 1);

    const DataRuleType dataRuleType = DataDescriptorPtr::Borrow(domainDescriptors[0]).getRule().getType();

#ifndef NDEBUG
    for (size_t i = 0; i < count; i++)
    {
        auto domainDescriptorPtr = DataDescriptorPtr::Borrow(domainDescriptors[i]);
        if (domainDescriptorPtr.getSampleType() != SampleType::Int64)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Domain descriptor sample type must be int64.", nullptr);
        if (domainDescriptorPtr.getRule().getType() != dataRuleType)
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_INVALIDPARAMETER, "Domain descriptor rule must be the same for all domain descriptors.", nullptr);
    }

    if (dataRuleType == DataRuleType::Linear)
    {
        if (offsets == nullptr)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Offsets parameter must not be null for linear data rule.", nullptr);
    }
#endif

    const auto offsetSize = offsets != nullptr ? sizeof(BulkIntegerImpl) : 0;
    size_t memSize = sizeof(std::atomic<int>) + sizeof(size_t) + sizeof(size_t) + (offsetSize + sizeof(BulkDataPacketImpl) * 2) * count;
    // align to 128
    memSize = alignTo(memSize, dataAlign);
    const size_t dataOffset = memSize;

    for (size_t i = 0; i < count; i++)
    {
        size_t rawSampleSize;
        valueDescriptors[i]->getRawSampleSize(&rawSampleSize);
        size_t dataSize = 0;
        if (dataRuleType == DataRuleType::Explicit)
        {
            dataSize += sampleCounts[i] * sizeof(int64_t);
            dataSize = alignTo(dataSize, dataAlign);
        }
        dataSize += sampleCounts[i] * rawSampleSize;
        dataSize = alignTo(dataSize, dataAlign);
        memSize += dataSize;
    }

    uint8_t* memory = static_cast<uint8_t*>(std::malloc(memSize));
    BulkDestruct onDestruct{memory};

    auto* memPtr = memory;
    new (memPtr) std::atomic<int>(static_cast<int>(count * (offsets != nullptr ? 3 : 2)));
    memPtr += sizeof(std::atomic<int>);
    *reinterpret_cast<size_t*>(memPtr) = count;
    memPtr += sizeof(size_t);
    *reinterpret_cast<size_t*>(memPtr) = offsets != nullptr ? 1 : 0;
    memPtr += sizeof(size_t);

    auto* dataPtr = memory + dataOffset;

    for (size_t i = 0; i < count; i++)
    {
        BulkDataPacketImpl* domainPacketImpl;

        INumber* offset = nullptr;

        if (offsets != nullptr)
        {
            const auto offsetImpl = new (memPtr) BulkIntegerImpl(offsets[i]);
            offsetImpl->setDestruct(onDestruct);
            offsetImpl->borrowInterface(INumber::Id, reinterpret_cast<void**>(&offset));
            memPtr += sizeof(BulkIntegerImpl);
        }

        if (dataRuleType == DataRuleType::Explicit)
        {
            domainPacketImpl = new (memPtr)
                BulkDataPacketImpl(
                nullptr,
                domainDescriptors[i],
                sampleCounts[i],
                offset,
                reinterpret_cast<void*>(dataPtr),
                nullptr);

            const auto packetMemSize = sizeof(int64_t) * sampleCounts[i];
            dataPtr += alignTo(packetMemSize, dataAlign);

        }
        else
        {
            domainPacketImpl = new (memPtr) BulkDataPacketImpl(
                nullptr,
                domainDescriptors[i],
                sampleCounts[i],
                offset,
                nullptr,
                nullptr);
        }

        domainPacketImpl->setDestruct(onDestruct);
        IDataPacket* domainPacket;
        domainPacketImpl->borrowInterface(IDataPacket::Id, reinterpret_cast<void**>(&domainPacket));
        memPtr += sizeof(BulkDataPacketImpl);

        size_t rawSampleSize;
        valueDescriptors[i]->getRawSampleSize(&rawSampleSize);
        const auto packetMemSize = rawSampleSize * sampleCounts[i];

        BulkDataPacketImpl* packetImpl = new (memPtr) BulkDataPacketImpl(
            domainPacket,
            valueDescriptors[i],
            sampleCounts[i],
            nullptr,
            reinterpret_cast<void*>(dataPtr),
            nullptr,
            packetMemSize);

        packetImpl->setDestruct(onDestruct);
        IDataPacket* dataPacket;
        packetImpl->queryInterface(IDataPacket::Id, reinterpret_cast<void**>(&dataPacket));

        dataPackets[i] = dataPacket;
        memPtr += sizeof(BulkDataPacketImpl);
        dataPtr += alignTo(packetMemSize, dataAlign);
    }

    return OPENDAQ_SUCCESS;
}

extern "C" PUBLIC_EXPORT ErrCode daqCreateValuePacketWithImplicitDomainPacket(
    IDataPacket** dataPacket,
    IDataDescriptor* valueDescriptor,
    IDataDescriptor* domainDescriptor,
    size_t sampleCount,
    int64_t offset,
    size_t dataAlign)
{
    return daqBulkCreateDataPackets(dataPacket, &valueDescriptor, &domainDescriptor, &sampleCount, &offset, 1, dataAlign);
}

extern "C" PUBLIC_EXPORT ErrCode daqCreateValuePacketWithExplicitDomainPacket(
    IDataPacket** dataPacket,
    IDataDescriptor* valueDescriptor,
    IDataDescriptor* domainDescriptor,
    size_t sampleCount,
    size_t dataAlign)
{
    return daqBulkCreateDataPackets(dataPacket, &valueDescriptor, &domainDescriptor, &sampleCount, nullptr, 1, dataAlign);
}


END_NAMESPACE_OPENDAQ
