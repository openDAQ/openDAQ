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

#include <cstdint>

#include <coretypes/string_ptr.h>
#include <opendaq/data_descriptor_ptr.h>

#include <ws-streaming/data_types.hpp>
#include <ws-streaming/metadata.hpp>
#include <ws-streaming/metadata_builder.hpp>

#include <newer_websocket_streaming_server_module/common.h>
#include <newer_websocket_streaming_server_module/descriptor_to_metadata.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

wss::metadata descriptorToMetadata(
    const DataDescriptorPtr& descriptor,
    const StringPtr& domainSignalId)
{
    auto builder = wss::metadata_builder{descriptor.getName()};

    switch (descriptor.getSampleType())
    {
        case SampleType::Float32:
            builder.data_type(wss::data_types::real32_t);
            break;
        case SampleType::Float64:
            builder.data_type(wss::data_types::real64_t);
            break;
        case SampleType::Int8:
            builder.data_type(wss::data_types::int8_t);
            break;
        case SampleType::Int16:
            builder.data_type(wss::data_types::int16_t);
            break;
        case SampleType::Int32:
            builder.data_type(wss::data_types::int32_t);
            break;
        case SampleType::Int64:
            builder.data_type(wss::data_types::int64_t);
            break;
        case SampleType::UInt8:
            builder.data_type(wss::data_types::uint8_t);
            break;
        case SampleType::UInt16:
            builder.data_type(wss::data_types::uint16_t);
            break;
        case SampleType::UInt32:
            builder.data_type(wss::data_types::uint32_t);
            break;
        case SampleType::UInt64:
            builder.data_type(wss::data_types::uint64_t);
            break;
        default:
            break;
    }

    if (auto originPtr = descriptor.getOrigin(); originPtr.assigned())
        builder.origin(originPtr);

    if (auto resolutionPtr = descriptor.getTickResolution(); resolutionPtr.assigned())
        builder.tick_resolution(
            resolutionPtr.getNumerator(),
            resolutionPtr.getDenominator());

    if (auto rangePtr = descriptor.getValueRange(); rangePtr.assigned())                
        builder.range(
            rangePtr.getLowValue(),
            rangePtr.getHighValue());

    if (auto unitPtr = descriptor.getUnit(); unitPtr.assigned())
        builder.unit(
            unitPtr.getId(),
            unitPtr.getName(),
            unitPtr.getQuantity(),
            unitPtr.getSymbol());

    if (domainSignalId.assigned() && domainSignalId != "")
        builder.table(domainSignalId);

    if (auto rulePtr = descriptor.getRule(); rulePtr.assigned())
    {
        if (rulePtr.getType() == DataRuleType::Linear)
        {
            std::int64_t start = 0, delta = 1;

            if (auto paramsPtr = rulePtr.getParameters(); paramsPtr.assigned())
            {
                if (auto startPtr = paramsPtr.get("start"); startPtr.assigned())
                    start = startPtr;
                if (auto deltaPtr = paramsPtr.get("delta"); deltaPtr.assigned())
                    delta = deltaPtr;
            }

            builder.linear_rule(start, delta);
        }
    }

    return builder.build();
}

wss::metadata descriptorToMetadata(
    const SignalPtr& signal,
    const DataDescriptorPtr& descriptor)
{
    StringPtr domainSignalId = "";

    if (auto domainSignal = signal.getDomainSignal(); domainSignal.assigned())
        domainSignalId = domainSignal.getGlobalId();

    return descriptorToMetadata(descriptor, domainSignalId);
}

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
