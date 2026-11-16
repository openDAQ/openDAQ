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
#include <opendaq/streaming_type_ptr.h>
#include <opendaq/server_capability_ptr.h>
#include <opendaq/connection_string_utils.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/custom_log.h>
#include <coretypes/dictobject_factory.h>
#include <string>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Checks whether a Streaming type ID is reserved.
 * @param id The Streaming type ID
 *
 * Add streaming options can't list a Streaming type with a reserved ID.
 * An empty ID is reserved.
 * An ID that contains `.` or `'` is reserved.
 * `StreamingType`, `StreamingMode` and `None` are reserved.
 */
inline bool isStreamingTypeIdReserved(const StringPtr& id)
{
    const std::string value = id;
    return value.empty() || value == "StreamingType" || value == "StreamingMode" || value == "None" ||
           value.find_first_of(".'") != std::string::npos;
}

/*!
 * @brief Gets the streaming types whose IDs are not reserved.
 * @param streamingTypes The streaming types, keyed by ID
 * @param loggerComponent The logger component that warnings are written to
 *
 * A warning is logged for each reserved ID.
 */
inline DictPtr<IString, IStreamingType> getListableStreamingTypes(const DictPtr<IString, IStreamingType>& streamingTypes,
                                                                  const LoggerComponentPtr& loggerComponent)
{
    auto listableTypes = Dict<IString, IStreamingType>();
    for (const auto& [id, streamingType] : streamingTypes)
        if (isStreamingTypeIdReserved(id))
        {
            LOG_W(R"(Streaming type ID "{}" is reserved)", id);
        }
        else
            listableTypes.set(id, streamingType);

    return listableTypes;
}

/*!
 * @brief Gets the connection string prefix a server capability is matched to types by.
 * @param capability The server capability
 *
 * The prefix is the capability's prefix when it is set.
 * Otherwise, it is the prefix of the capability's connection string, as `getPrefixFromConnectionString` gives it.
 */
inline StringPtr getServerCapabilityPrefix(const ServerCapabilityPtr& capability)
{
    const StringPtr prefix = capability.getPrefix();
    if (prefix.getLength() > 0)
        return prefix;

    return getPrefixFromConnectionString(capability.getConnectionString());
}

/*!
 * @brief Gets the streaming types advertised by streaming capabilities.
 * @param capabilities The server capabilities of a device
 * @param streamingTypes The available streaming types, keyed by ID
 *
 * A streaming type is advertised by a capability of protocol type Streaming or ConfigurationAndStreaming with the same prefix.
 * The capability's prefix is given by `getServerCapabilityPrefix`.
 * The types are in capability order, each listed once.
 */
inline DictPtr<IString, IStreamingType> getAdvertisedStreamingTypes(const ListPtr<IServerCapability>& capabilities,
                                                                    const DictPtr<IString, IStreamingType>& streamingTypes)
{
    auto advertisedTypes = Dict<IString, IStreamingType>();
    for (const auto& capability : capabilities)
    {
        const auto protocolType = capability.getProtocolType();
        if (protocolType != ProtocolType::Streaming && protocolType != ProtocolType::ConfigurationAndStreaming)
            continue;

        for (const auto& [id, streamingType] : streamingTypes)
            if (streamingType.getConnectionStringPrefix() == getServerCapabilityPrefix(capability) && !advertisedTypes.hasKey(id))
                advertisedTypes.set(id, streamingType);
    }

    return advertisedTypes;
}

/*!
 * @brief Selects the default streaming type for a device type.
 * @param streamingTypes The streaming types to select from, keyed by ID
 * @param deviceType The device type
 * @param fallBackToFirst Whether to fall back to the first type
 *
 * The type linked to the device type is selected.
 * Without one, the first type is selected when fallBackToFirst is set.
 * Otherwise, the result is null.
 */
inline StreamingTypePtr selectStreamingType(const DictPtr<IString, IStreamingType>& streamingTypes,
                                            const DeviceTypePtr& deviceType,
                                            bool fallBackToFirst)
{
    if (deviceType.assigned())
    {
        for (const auto& [_, streamingType] : streamingTypes)
        {
            const DeviceTypePtr linkedDeviceType = streamingType.getLinkedDeviceType();
            if (linkedDeviceType.assigned() && linkedDeviceType.getId() == deviceType.getId())
                return streamingType;
        }
    }

    if (fallBackToFirst && streamingTypes.getCount() > 0)
        return streamingTypes.getValueList()[0];

    return nullptr;
}

END_NAMESPACE_OPENDAQ
