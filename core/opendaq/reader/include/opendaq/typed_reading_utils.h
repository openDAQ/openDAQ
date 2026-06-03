/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

#include <coretypes/common.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/domain_value.h>


BEGIN_NAMESPACE_OPENDAQ

struct ReadLayout
{
    DataDescriptorPtr descriptor = nullptr;
    SizeT rawSampleSize = 0;
    SizeT valuesPerSample = 1;
};

class TypedReadingUtils
{
public:
    static ReadLayout createReadLayout(const DataDescriptorPtr& descriptor);

    static std::unique_ptr<DomainValue> readDomainValue(SampleType in,
                                                        SampleType out,
                                                        const ReadLayout& readLayout,
                                                        const DataPacketPtr& domainPacket,
                                                        SizeT index,
                                                        const DomainInfo& domainInfo);

    static SizeT findDomainValue(SampleType in,
                                 SampleType out,
                                 const ReadLayout& readLayout,
                                 const DataPacketPtr& domainPacket,
                                 const DomainValue* target,
                                 std::chrono::system_clock::rep* firstSampleAbsoluteTime = nullptr);

    static ErrCode readData(SampleType in,
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