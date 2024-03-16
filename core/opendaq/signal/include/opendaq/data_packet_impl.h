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
#include <opendaq/reusable_data_packet.h>

BEGIN_NAMESPACE_OPENDAQ

class DataPacketImpl : public GenericDataPacketImpl<IDataPacket, IReusableDataPacket>
{
public:
    using Super = GenericDataPacketImpl<IDataPacket, IReusableDataPacket>;

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
    ErrCode INTERFACE_FUNC queryInterface(const IntfID& id, void** intf) override;
    ErrCode INTERFACE_FUNC borrowInterface(const IntfID& id, void** intf) const override;

    ErrCode INTERFACE_FUNC reuse(IDataDescriptor* newDescriptor, SizeT newSampleCount,
                                 INumber* newOffset,
                                 IDataPacket* newDomainPacket,
                                 Bool canReallocMemory,
                                 Bool* success) override;

private:
    bool isDataEqual(const DataPacketPtr& dataPacket) const;
    void freeMemory();
    void freeScaledData();
    void initPacket();

    AllocatorPtr allocator;
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
};

END_NAMESPACE_OPENDAQ
