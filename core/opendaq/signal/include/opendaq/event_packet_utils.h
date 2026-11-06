/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_descriptor_factory.h>

BEGIN_NAMESPACE_OPENDAQ

inline std::tuple<bool, bool, DataDescriptorPtr, DataDescriptorPtr> parseDataDescriptorEventPacket(const EventPacketPtr& eventPacket)
{
    if (!eventPacket.assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException, "Event packet not assigned");

    if (!(eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED))
        DAQ_THROW_EXCEPTION(InvalidParameterException, R"(Invalid event packet id: {})", eventPacket.getEventId());

    const auto params = eventPacket.getParameters();
    const DataDescriptorPtr valueDescriptorParam = params[event_packet_param::DATA_DESCRIPTOR];
    const DataDescriptorPtr domainDescriptorParam = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];
    const bool valueDescriptorChanged = valueDescriptorParam.assigned();
    const bool domainDescriptorChanged = domainDescriptorParam.assigned();
    // A null descriptor is told by its sample type: building one per event and comparing by value took microseconds
    const auto real = [](const DataDescriptorPtr& descriptor) { return descriptor.assigned() && descriptor.getSampleType() != SampleType::Null; };
    const DataDescriptorPtr newValueDescriptor = real(valueDescriptorParam) ? valueDescriptorParam : nullptr;
    const DataDescriptorPtr newDomainDescriptor = real(domainDescriptorParam) ? domainDescriptorParam : nullptr;

    return std::make_tuple(valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptor);
}

inline DataDescriptorPtr descriptorToEventPacketParam(const DataDescriptorPtr& dataDescriptor)
{
    return dataDescriptor.assigned() ? dataDescriptor : NullDataDescriptor();
}

END_NAMESPACE_OPENDAQ
