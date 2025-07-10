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
#include <opendaq/context_ptr.h>
#include <opendaq/device_ptr.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/server.h>
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/property_factory.h>
#include <opendaq/discovery_server_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/server_impl.h>
#include <opendaq/streaming_server.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IStreamingServer, typename... Interfaces>
class StreamingServerImpl;

using StreamingServer = StreamingServerImpl<>;

template <typename TInterface, typename... Interfaces>
class StreamingServerImpl : public ServerImpl<TInterface, Interfaces...>
{
public:
    using Self = StreamingServerImpl<TInterface, Interfaces...>;
    using Super = ServerImpl<TInterface, Interfaces...>;

    explicit StreamingServerImpl(const StringPtr& id,
                                 const PropertyObjectPtr& serverConfig,
                                 const DevicePtr& rootDevice,
                                 const ContextPtr& context,
                                 const ComponentPtr& parent = nullptr)
        : Super(id, serverConfig, rootDevice, context, parent)
    {
    }
};

END_NAMESPACE_OPENDAQ
