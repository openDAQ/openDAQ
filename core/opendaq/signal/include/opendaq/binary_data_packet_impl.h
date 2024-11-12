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
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/deleter_ptr.h>
#include <opendaq/generic_data_packet_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <bool ExternalMemory>
struct BinaryDataDeleter
{
};

template <>
struct BinaryDataDeleter<true>
{
    void operator()(void* data)
    {
        deleter.deleteMemory(data);
    }

    DeleterPtr deleter;
};

template <>
struct BinaryDataDeleter<false>
{
    void operator()(void* data)
    {
        std::free(data);
    }
};

template <bool ExternalMemory>
class BinaryDataPacketImpl : public GenericDataPacketImpl<IDataPacket>
{
public:
    template <bool B = ExternalMemory, std::enable_if_t<!B, int> = 0>
    explicit BinaryDataPacketImpl(const DataPacketPtr& domainPacket, const DataDescriptorPtr& dataDescriptor, SizeT sampleSize);

    template <bool B = ExternalMemory, std::enable_if_t<B, int> = 0>
    explicit BinaryDataPacketImpl(const DataPacketPtr& domainPacket,
                                  const DataDescriptorPtr& dataDescriptor,
                                  SizeT sampleSize,
                                  void* data,
                                  const DeleterPtr& deleter);

    ErrCode INTERFACE_FUNC getDataDescriptor(IDataDescriptor** dataDescriptor) override;
    ErrCode INTERFACE_FUNC getSampleCount(SizeT* sampleCount) override;
    ErrCode INTERFACE_FUNC getOffset(INumber** offset) override;
    ErrCode INTERFACE_FUNC getRawData(void** address) override;
    ErrCode INTERFACE_FUNC getData(void** address) override;
    ErrCode INTERFACE_FUNC getDataSize(SizeT* dataSize) override;
    ErrCode INTERFACE_FUNC getRawDataSize(SizeT* dataSize) override;
    ErrCode INTERFACE_FUNC getLastValue(IBaseObject** value, ITypeManager* typeManager = nullptr) override;
    ErrCode INTERFACE_FUNC getValueByIndex(IBaseObject** value, SizeT index, ITypeManager* typeManager) override;

private:
    DataPacketPtr domainPacket;
    DataDescriptorPtr dataDescriptor;
    SizeT sampleSize;
    std::unique_ptr<void, BinaryDataDeleter<ExternalMemory>> data;

#ifdef OPENDAQ_ENABLE_PARAMETER_VALIDATION
    void validateDescriptor();
#endif
};

#ifdef OPENDAQ_ENABLE_PARAMETER_VALIDATION
template <bool ExternalMemory>
void BinaryDataPacketImpl<ExternalMemory>::validateDescriptor()
{
    if (!dataDescriptor.assigned())
        throw ArgumentNullException();

    if (dataDescriptor.getSampleType() != SampleType::Binary)
        throw InvalidParameterException("Sample type is not Binary.");
}
#endif

template <bool ExternalMemory>
template <bool B, std::enable_if_t<!B, int>>
BinaryDataPacketImpl<ExternalMemory>::BinaryDataPacketImpl(const DataPacketPtr& domainPacket,
                                                           const DataDescriptorPtr& dataDescriptor,
                                                           SizeT sampleSize)
    : GenericDataPacketImpl<IDataPacket>(domainPacket)
    , dataDescriptor(dataDescriptor)
    , sampleSize(sampleSize)
    , data(std::malloc(sampleSize))
{
#ifdef OPENDAQ_ENABLE_PARAMETER_VALIDATION
    validateDescriptor();
#endif
}

template <bool ExternalMemory>
template <bool B, std::enable_if_t<B, int>>
BinaryDataPacketImpl<ExternalMemory>::BinaryDataPacketImpl(
    const DataPacketPtr& domainPacket, const DataDescriptorPtr& dataDescriptor, SizeT sampleSize, void* data, const DeleterPtr& deleter)
    : GenericDataPacketImpl<IDataPacket>(domainPacket)
    , dataDescriptor(dataDescriptor)
    , sampleSize(sampleSize)
    , data(data, BinaryDataDeleter<true>{deleter})
{
#ifdef OPENDAQ_ENABLE_PARAMETER_VALIDATION
    validateDescriptor();
    if (!this->data)
        throw InvalidParameterException("Data parameter must not be null.");
#endif
    type = PacketType::Data;
}

template <bool ExternalMemory>
ErrCode BinaryDataPacketImpl<ExternalMemory>::getDataDescriptor(IDataDescriptor** dataDescriptor)
{
    OPENDAQ_PARAM_NOT_NULL(dataDescriptor);

    *dataDescriptor = this->dataDescriptor.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <bool ExternalMemory>
ErrCode BinaryDataPacketImpl<ExternalMemory>::getSampleCount(SizeT* sampleCount)
{
    OPENDAQ_PARAM_NOT_NULL(sampleCount);

    *sampleCount = 1;

    return OPENDAQ_SUCCESS;
}

template <bool ExternalMemory>
ErrCode BinaryDataPacketImpl<ExternalMemory>::getRawData(void** address)
{
    OPENDAQ_PARAM_NOT_NULL(address);

    *address = data.get();

    return OPENDAQ_SUCCESS;
}

template <bool ExternalMemory>
ErrCode BinaryDataPacketImpl<ExternalMemory>::getData(void** address)
{
    OPENDAQ_PARAM_NOT_NULL(address);

    *address = data.get();

    return OPENDAQ_SUCCESS;
}

template <bool ExternalMemory>
ErrCode BinaryDataPacketImpl<ExternalMemory>::getDataSize(SizeT* dataSize)
{
    OPENDAQ_PARAM_NOT_NULL(dataSize);

    *dataSize = this->sampleSize;

    return OPENDAQ_SUCCESS;
}

template <bool ExternalMemory>
ErrCode BinaryDataPacketImpl<ExternalMemory>::getRawDataSize(SizeT* rawDataSize)
{
    OPENDAQ_PARAM_NOT_NULL(rawDataSize);

    *rawDataSize = this->sampleSize;

    return OPENDAQ_SUCCESS;
}

template <bool ExternalMemory>
inline ErrCode INTERFACE_FUNC BinaryDataPacketImpl<ExternalMemory>::getLastValue(IBaseObject** value, ITypeManager* typeManager)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    *value = nullptr;

    return OPENDAQ_IGNORED;
}
template <bool ExternalMemory>
ErrCode BinaryDataPacketImpl<ExternalMemory>::getValueByIndex(IBaseObject** value, SizeT index, ITypeManager* typeManager)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    *value = nullptr;

    return OPENDAQ_IGNORED;
}

template <bool ExternalMemory>
ErrCode BinaryDataPacketImpl<ExternalMemory>::getOffset(INumber** offset)
{
    OPENDAQ_PARAM_NOT_NULL(offset);

    *offset = NumberPtr(0).detach();

    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
