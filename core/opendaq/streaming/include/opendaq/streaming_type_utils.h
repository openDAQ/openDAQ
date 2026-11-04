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
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Gets the streaming types advertised by streaming capabilities.
 * @param capabilities The server capabilities of a device
 * @param streamingTypes The available streaming types, keyed by ID
 *
 * A streaming type is advertised by a capability of protocol type Streaming or ConfigurationAndStreaming with the same prefix.
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
            if (streamingType.getConnectionStringPrefix() == capability.getPrefix() && !advertisedTypes.hasKey(id))
                advertisedTypes.set(id, streamingType);
    }

    return advertisedTypes;
}

END_NAMESPACE_OPENDAQ
