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
#include <opendaq/streaming_to_device_server.h>
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/property_factory.h>
#include <opendaq/discovery_server_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/streaming_server_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IStreamingToDeviceServer, typename... Interfaces>
class StreamingToDeviceServerImpl;

using StreamingToDeviceServer = StreamingToDeviceServerImpl<>;

template <typename TInterface, typename... Interfaces>
class StreamingToDeviceServerImpl : public StreamingServerImpl<TInterface, IStreaming, Interfaces...>
{
public:
    using Self = StreamingToDeviceServerImpl<TInterface, Interfaces...>;
    using Super = StreamingServerImpl<TInterface, IStreaming, Interfaces...>;

    explicit StreamingToDeviceServerImpl(const StringPtr& id,
                                         const PropertyObjectPtr& serverConfig,
                                         const DevicePtr& rootDevice,
                                         const ContextPtr& context,
                                         const StreamingPtr& streaming,
                                         const ComponentPtr& parent = nullptr)
        : Super(id, serverConfig, rootDevice, context, parent)
        , streaming(streaming)
    {
    }

    // IStreamingToDeviceServer
//    ErrCode INTERFACE_FUNC attachConnectedSignal(IMirroredSignalConfig* signal, IInputPort* port) override;
//    ErrCode INTERFACE_FUNC detachConnectedSignal(IMirroredSignalConfig* signal, IInputPort* port) override;

    // IStreaming
    ErrCode INTERFACE_FUNC getActive(Bool* active) override;
    ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC addSignals(IList* signals) override;
    ErrCode INTERFACE_FUNC removeSignals(IList* signals) override;
    ErrCode INTERFACE_FUNC removeAllSignals() override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) const override;
    ErrCode INTERFACE_FUNC getConnectionStatus(IEnumeration** connectionStatus) override;

protected:
    StreamingPtr streaming;
};

template <typename TInterface, typename... Interfaces>
ErrCode StreamingToDeviceServerImpl<TInterface, Interfaces...>::getActive(Bool* active)
{
    return streaming->getActive(active);
}

template <typename TInterface, typename... Interfaces>
ErrCode StreamingToDeviceServerImpl<TInterface, Interfaces...>::setActive(Bool active)
{
    return streaming->setActive(active);
}

template <typename TInterface, typename... Interfaces>
ErrCode StreamingToDeviceServerImpl<TInterface, Interfaces...>::addSignals(IList* signals)
{
    return streaming->addSignals(signals);
}

template <typename TInterface, typename... Interfaces>
ErrCode StreamingToDeviceServerImpl<TInterface, Interfaces...>::removeSignals(IList* signals)
{
    return streaming->removeSignals(signals);
}

template <typename TInterface, typename... Interfaces>
ErrCode StreamingToDeviceServerImpl<TInterface, Interfaces...>::removeAllSignals()
{
    return streaming->removeAllSignals();
}

template <typename TInterface, typename... Interfaces>
ErrCode StreamingToDeviceServerImpl<TInterface, Interfaces...>::getConnectionString(IString** connectionString) const
{
    return streaming->getConnectionString(connectionString);
}

template <typename TInterface, typename... Interfaces>
ErrCode StreamingToDeviceServerImpl<TInterface, Interfaces...>::getConnectionStatus(IEnumeration** connectionStatus)
{
    return streaming->getConnectionStatus(connectionStatus);
}

END_NAMESPACE_OPENDAQ
