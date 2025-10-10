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

#include <string>

#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/dimension_factory.h>

#include <ws-streaming/data_types.hpp>
#include <ws-streaming/metadata.hpp>
#include <ws-streaming/rule_types.hpp>

#include <websocket_streaming/common.h>
#include <websocket_streaming/metadata_to_descriptor.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

static SampleType getOpenDaqSampleType(const std::string& type)
{
    if (type == wss::data_types::real32_t) return SampleType::Float32;
    else if (type == wss::data_types::real64_t) return SampleType::Float64;
    else if (type == wss::data_types::int8_t) return SampleType::Int8;
    else if (type == wss::data_types::int16_t) return SampleType::Int16;
    else if (type == wss::data_types::int32_t) return SampleType::Int32;
    else if (type == wss::data_types::int64_t) return SampleType::Int64;
    else if (type == wss::data_types::uint8_t) return SampleType::UInt8;
    else if (type == wss::data_types::uint16_t) return SampleType::UInt16;
    else if (type == wss::data_types::uint32_t) return SampleType::UInt32;
    else if (type == wss::data_types::uint64_t) return SampleType::UInt64;
    else if (type == wss::data_types::struct_t) return SampleType::Struct;
    else return SampleType::Undefined;
}

DataDescriptorPtr metadataToDescriptor(
    const wss::metadata& metadata)
{
    auto builder = DataDescriptorBuilder();

    builder.setName(metadata.name());

    builder.setSampleType(
        getOpenDaqSampleType(
            metadata.data_type()));

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

    auto rule = metadata.rule();

    if (rule == wss::rule_types::linear_rule)
    {
        auto startDelta = metadata.linear_start_delta();
        builder.setRule(
            LinearDataRule(
                startDelta.second.value_or(0),
                startDelta.first.value_or(0)));
    }

    else if (rule == wss::rule_types::constant_rule)
        builder.setRule(ConstantDataRule());

    if (builder.getSampleType() == SampleType::Struct)
    {
        auto fields = List<IDataDescriptor>();

        for (const auto& field : metadata.struct_fields())
        {
            auto dimensions = List<IDimension>();

            for (const auto& dim : field.dimensions())
            {
                auto dimBuilder = DimensionBuilder()
                    .setName(dim.name());

                if (dim.rule() == wss::rule_types::linear_rule)
                {
                    auto [start, delta, size] = dim.linear_start_delta_size();

                    dimBuilder.setRule(
                        LinearDimensionRule(
                            delta.value_or(0),
                            start.value_or(0),
                            size.value_or(0)));
                }

                dimensions.pushBack(dimBuilder.build());
            }

            auto descriptor = DataDescriptorBuilder()
                .setName(field.name())
                .setSampleType(getOpenDaqSampleType(field.data_type()))
                .setRule(ExplicitDataRule())
                .setDimensions(dimensions)
                .build();

            fields.pushBack(descriptor);
        }

        builder.setStructFields(fields);
    }

    return builder.build();
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
