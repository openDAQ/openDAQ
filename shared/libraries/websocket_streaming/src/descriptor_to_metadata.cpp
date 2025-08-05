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
#include <ws-streaming/struct_field_builder.hpp>

#include <websocket_streaming/common.h>
#include <websocket_streaming/descriptor_to_metadata.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

static std::string getWebSocketDataType(SampleType type)
{
    switch (type)
    {
        case SampleType::Float32: return wss::data_types::real32_t;
        case SampleType::Float64: return wss::data_types::real64_t;
        case SampleType::Int8: return wss::data_types::int8_t;
        case SampleType::Int16: return wss::data_types::int16_t;
        case SampleType::Int32: return wss::data_types::int32_t;
        case SampleType::Int64: return wss::data_types::int64_t;
        case SampleType::UInt8: return wss::data_types::uint8_t;
        case SampleType::UInt16: return wss::data_types::uint16_t;
        case SampleType::UInt32: return wss::data_types::uint32_t;
        case SampleType::UInt64: return wss::data_types::uint64_t;
        case SampleType::Struct: return wss::data_types::struct_t;
        default: return "";
    }
}

static wss::metadata descriptorToMetadata(
    const DataDescriptorPtr& descriptor,
    const StringPtr& signalId,
    const StringPtr& domainSignalId)
{
    auto builder = wss::metadata_builder{descriptor.getName()};

    std::string dataType = getWebSocketDataType(descriptor.getSampleType());
    if (!dataType.empty())
        builder.data_type(dataType);

    if (auto fields = descriptor.getStructFields(); fields.assigned())
    {
        for (auto field : fields)
        {
            wss::struct_field_builder fieldBuilder(field.getName());
            fieldBuilder.data_type(getWebSocketDataType(field.getSampleType()));

            if (auto dims = field.getDimensions(); dims.assigned())
            {
                for (const auto& dim : dims)
                {
                    wss::dimension_builder dimBuilder(dim.getName());

                    if (auto rule = dims[0].getRule(); rule.assigned()
                        && rule.getType() == DimensionRuleType::Linear)
                    {
                        dimBuilder.linear_rule(
                            rule.getParameters().get("start"),
                            rule.getParameters().get("delta"),
                            rule.getParameters().get("size"));
                    }

                    fieldBuilder.dimension(dimBuilder);
                }
            }

            builder.struct_field(fieldBuilder);
        }
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
    else
        builder.table(signalId);

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

        else if (rulePtr.getType() == DataRuleType::Constant)
        {
            builder.constant_rule();
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

    return descriptorToMetadata(
        descriptor,
        signal.getGlobalId(),
        domainSignalId);
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
