#pragma once

#include <coretypes/common.h>
#include <opendaq/domain_value.h>
#include <opendaq/data_packet_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

struct ReadLayout
{
    DataDescriptorPtr descriptor = nullptr;
    SizeT rawSampleSize = 0;
    SizeT valuesPerSample = 1;
};

class TypedReadingUtils
{
    static std::unique_ptr<DomainValue> readDomainValue(
        SampleType in,
        SampleType out,
        const ReadLayout& readLayout,
        const DataPacketPtr& domainPacket,
        SizeT index,
        const DomainInfo& domainInfo);

    static SizeT findDomainValue(
        SampleType in,
        SampleType out,
        const ReadLayout& readLayout,
        const DataPacketPtr& domainPacket,
        const DomainValue* target,
        std::chrono::system_clock::rep* firstSampleAbsoluteTime = nullptr);
    
    static ErrCode readData(
        SampleType in,
        SampleType out,
        bool isDomain,
        const ReadLayout& readLayout,
        void* inputBuffer,
        SizeT offset,
        void** outputBuffer,
        SizeT count,
        const FunctionPtr transform = nullptr);
};

END_NAMESPACE_OPENDAQ