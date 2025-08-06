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

#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_descriptor_ptr.h>

#include <ws-streaming/data_types.hpp>
#include <ws-streaming/metadata.hpp>
#include <ws-streaming/rule_types.hpp>

#include <newer_websocket_streaming_server_module/common.h>
#include <newer_websocket_streaming_server_module/metadata_to_descriptor.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

DataDescriptorPtr metadataToDescriptor(
    const wss::metadata& metadata)
{
    auto builder = DataDescriptorBuilder();

    builder.setName(metadata.name());

    std::string dataType = metadata.data_type();
    if (dataType == wss::data_types::real32_t)
        builder.setSampleType(SampleType::Float32);
    else if (dataType == wss::data_types::real64_t)
        builder.setSampleType(SampleType::Float64);
    else if (dataType == wss::data_types::int8_t)
        builder.setSampleType(SampleType::Int8);
    else if (dataType == wss::data_types::int16_t)
        builder.setSampleType(SampleType::Int16);
    else if (dataType == wss::data_types::int32_t)
        builder.setSampleType(SampleType::Int32);
    else if (dataType == wss::data_types::int64_t)
        builder.setSampleType(SampleType::Int64);
    else if (dataType == wss::data_types::uint8_t)
        builder.setSampleType(SampleType::UInt8);
    else if (dataType == wss::data_types::uint16_t)
        builder.setSampleType(SampleType::UInt16);
    else if (dataType == wss::data_types::uint32_t)
        builder.setSampleType(SampleType::UInt32);
    else if (dataType == wss::data_types::uint64_t)
        builder.setSampleType(SampleType::UInt64);

    std::string origin = metadata.origin();
    if (!origin.empty())
        builder.setOrigin(origin);

    auto resolution = metadata.tick_resolution();
    if (resolution.has_value())
        builder.setTickResolution(
            Ratio(
                resolution.value().first,
                resolution.value().second));

    auto range = metadata.range();
    if (range.has_value())
        builder.setValueRange(
            Range(
                range.value().first,
                range.value().second));

    auto unit = metadata.unit();
    if (unit.has_value())
        builder.setUnit(
            Unit(
                unit.value().symbol(),
                unit.value().id(),
                unit.value().name(),
                unit.value().quantity()));

    // XXX TODO: domain signal id

    auto rule = metadata.rule();
    if (rule == wss::rule_types::linear_rule)
    {
        auto startDelta = metadata.linear_start_delta();
        builder.setRule(
            LinearDataRule(
                startDelta.second,
                startDelta.first));
    }

    return builder.build();
}

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
