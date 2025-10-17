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

#include <algorithm>
#include <functional>
#include <thread>
#include <utility>

#include <boost/system/error_code.hpp>

#include <opendaq/connected_client_info.h>
#include <opendaq/network_interface.h>
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/opendaq.h>
#include <opendaq/server_capability.h>
#include <opendaq/server_impl.h>
#include <opendaq/server_type_factory.h>

#include <ws-streaming/ws-streaming.hpp>

#include <websocket_streaming/descriptor_to_metadata.h>
#include <websocket_streaming/ws_streaming_listener.h>
#include <websocket_streaming/ws_streaming_server.h>

using namespace std::placeholders;

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

PropertyObjectPtr WsStreamingServer::createDefaultConfig(const ContextPtr& context)
{
    constexpr Int minPortValue = 0;
    constexpr Int maxPortValue = 65535;

    auto defaultConfig = PropertyObject();

    const auto websocketPortProp =
        IntPropertyBuilder("WebsocketStreamingPort", 7414).setMinValue(minPortValue).setMaxValue(maxPortValue).build();
    defaultConfig.addProperty(websocketPortProp);

    const auto websocketControlPortProp =
        IntPropertyBuilder("WebsocketControlPort", 7438).setMinValue(minPortValue).setMaxValue(maxPortValue).build();
    defaultConfig.addProperty(websocketControlPortProp);

    defaultConfig.addProperty(StringProperty("Path", "/"));

    populateDefaultConfigFromProvider(context, defaultConfig);
    return defaultConfig;
}

ServerTypePtr WsStreamingServer::createType(const ContextPtr& context)
{
    return ServerType(
        ID,
        "openDAQ LT Streaming server",
        "Publishes device signals as a flat list and streams data over WebSocketTcp protocol",
        WsStreamingServer::createDefaultConfig(context));
}

void WsStreamingServer::populateDefaultConfigFromProvider(const ContextPtr& context, const PropertyObjectPtr& config)
{
    if (!context.assigned())
        return;
    if (!config.assigned())
        return;

    auto options = context.getModuleOptions("StreamingLtServer");
    for (const auto& [key, value] : options)
    {
        if (config.hasProperty(key))
        {
            config->setPropertyValue(key, value);
        }
    }
}

WsStreamingServer::WsStreamingServer(
        const InstancePtr& instance)
    : WsStreamingServer(
        instance.getRootDevice(),
        createDefaultConfig(instance.getContext()),
        instance.getContext())
{
}

WsStreamingServer::WsStreamingServer(
        const DevicePtr& rootDevice,
        const PropertyObjectPtr& config,
        const ContextPtr& context)
    : Server{ID, config, rootDevice, context}
    , _rootDevice{rootDevice}
    , _ioc{1}
    , _server{_ioc.get_executor()}
{
    _port = config.getPropertyValue("WebsocketStreamingPort");

    _server.add_listener(config.getPropertyValue("WebsocketStreamingPort"));
    _server.add_listener(config.getPropertyValue("WebsocketControlPort"), true);

    _onClientConnected = _server.on_client_connected.connect(
        std::bind(&WsStreamingServer::onClientConnected, this, _1));
    _onClientDisconnected = _server.on_client_disconnected.connect(
        std::bind(&WsStreamingServer::onClientDisconnected, this, _1, _2));

    _server.run();

    rescan();

    _thread = std::thread{[this]() { _ioc.run(); }};

    context.getOnCoreEvent() += event(&WsStreamingServer::onCoreEvent);

    addCapability();
}

WsStreamingServer::~WsStreamingServer()
{
    onStopServer();
}

wss::server& WsStreamingServer::getWsServer() noexcept
{
    return _server;
}

PropertyObjectPtr WsStreamingServer::getDiscoveryConfig()
{
    auto discoveryConfig = PropertyObject();
    discoveryConfig.addProperty(StringProperty("ServiceName", "_streaming-lt._tcp.local."));
    discoveryConfig.addProperty(StringProperty("ServiceCap", "LT"));
    discoveryConfig.addProperty(StringProperty("Path", config.getPropertyValue("Path")));
    discoveryConfig.addProperty(IntProperty("Port", config.getPropertyValue("WebsocketStreamingPort")));
    discoveryConfig.addProperty(StringProperty("ProtocolVersion", ""));
    return discoveryConfig;
}

void WsStreamingServer::onStopServer()
{
    _onClientConnected.disconnect();
    _onClientDisconnected.disconnect();

    context.getOnCoreEvent() -= event(&WsStreamingServer::onCoreEvent);

    // openDAQ can (but probably should not) call onStopServer() more than once.
    if (_thread.joinable())
    {
        _server.close();
        _ioc.stop();
        _thread.join();
    }
}

PropertyObjectPtr WsStreamingServer::populateDefaultConfig(
    const PropertyObjectPtr& config,
    const ContextPtr& context)
{
    const auto defConfig = createDefaultConfig(context);
    for (const auto& prop : defConfig.getAllProperties())
    {
        const auto name = prop.getName();
        if (config.hasProperty(name))
            defConfig.setPropertyValue(name, config.getPropertyValue(name));
    }

    return defConfig;
}

void WsStreamingServer::addCapability()
{
    auto info = _rootDevice.getInfo();
    if (info.hasServerCapability("OpenDAQLTStreaming"))
        DAQ_THROW_EXCEPTION(InvalidStateException,
            fmt::format("Device \"{}\" already has an OpenDAQLTStreaming server capability.", info.getName()));

    auto cap = ServerCapability("OpenDAQLTStreaming", "OpenDAQLTStreaming", ProtocolType::Streaming);
    cap.setPrefix("daq.lt");
    cap.setPort(_port);
    cap.setConnectionType("TCP/IP");
    info.asPtr<IDeviceInfoInternal>(true).addServerCapability(cap);

}

void WsStreamingServer::createListener(const SignalPtr& signal)
{
    SignalPtr domainSignal = signal.getDomainSignal();

    if (domainSignal.assigned())
        createListener(domainSignal);

    auto it = _localSignals.find(signal.getGlobalId());
    if (it != _localSignals.end())
    {
        // Check if the signal has acquired a new/different domain signal since it was added. If
        // so, we need to unregister the local_signal from ws-streaming and re-register it so that
        // the domain tab table is linked correctly.

        if (domainSignal != it->second.domainSignal)
        {
            _server.remove_local_signal(it->second.localSignal);
            _localSignals.erase(it);
        }

        else
        {
            return;
        }
    }

    auto& streamableSignal = _localSignals.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(
                signal.getGlobalId()),
            std::forward_as_tuple(
                signal.getGlobalId(),
                daq::websocket_streaming::descriptorToMetadata(signal, signal.getDescriptor()),
                signal))
        .first->second;

    streamableSignal.domainSignal = domainSignal;

    streamableSignal.localSignal.on_subscribed.connect([
        =,
        &streamableSignal,
        signal_id = signal.getGlobalId().toStdString()
    ]()
    {
        streamableSignal.listener = createWithImplementation<IInputPortNotifications, WsStreamingListener>(
            this->template thisPtr<ComponentPtr>().getContext(),
            signal,
            &streamableSignal.localSignal);
        reinterpret_cast<WsStreamingListener *>(streamableSignal.listener.getObject())->start();
    });

    streamableSignal.localSignal.on_unsubscribed.connect([
        =,
        &streamableSignal,
        signal_id = signal.getGlobalId().toStdString()
    ]()
    {
        streamableSignal.listener.release();
    });

    _server.add_local_signal(streamableSignal.localSignal);
}

void WsStreamingServer::onClientConnected(
    const wss::connection_ptr& connection)
{
    SizeT clientNumber = 0;
    if (_rootDevice.assigned() && !_rootDevice.isRemoved())
    {
        _rootDevice.getInfo().asPtr<IDeviceInfoInternal>(true).addConnectedClient(
            &clientNumber,
            ConnectedClientInfo(connection->socket().remote_endpoint().address().to_string(),
                ProtocolType::Streaming,
                "OpenDAQLTStreaming",
                "",
                ""));
    }
    _registeredClientIds.insert({connection.get(), clientNumber});
}

void WsStreamingServer::onClientDisconnected(
    const wss::connection_ptr& connection,
    const boost::system::error_code& ec)
{
    if (auto it = _registeredClientIds.find(connection.get()); it != _registeredClientIds.end())
    {
        if (_rootDevice.assigned() && !_rootDevice.isRemoved() && it->second != 0)
            _rootDevice.getInfo().asPtr<IDeviceInfoInternal>(true).removeConnectedClient(it->second);
        _registeredClientIds.erase(it);
    }
}

void WsStreamingServer::onCoreEvent(
    ComponentPtr& component,
    CoreEventArgsPtr& args)
{
    switch (static_cast<CoreEventId>(args.getEventId()))
    {
        case CoreEventId::ComponentAdded:       return onComponentAdded(component, args);
        case CoreEventId::ComponentRemoved:     return onComponentRemoved(component, args);
        case CoreEventId::ComponentUpdateEnd:   return onComponentUpdateEnd(component, args);
        case CoreEventId::AttributeChanged:     return onAttributeChanged(component, args);
        default: break;
    }
}

void WsStreamingServer::onComponentAdded(
    ComponentPtr& component,
    CoreEventArgsPtr& args)
{
    rescan();
}

void WsStreamingServer::onComponentRemoved(
    ComponentPtr& component,
    CoreEventArgsPtr& args)
{
    rescan();
}

void WsStreamingServer::onComponentUpdateEnd(
    ComponentPtr& component,
    CoreEventArgsPtr& args)
{
    rescan();
}

void WsStreamingServer::onAttributeChanged(
    ComponentPtr& component,
    CoreEventArgsPtr& args)
{
    rescan();
}

void WsStreamingServer::rescan()
{
    auto it = _localSignals.begin();
    while (it != _localSignals.end())
    {
        if (it->second.openDaqSignal.isRemoved())
        {
            auto jt = it++;
            _server.remove_local_signal(jt->second.localSignal);
            _localSignals.erase(jt);
        }

        else
            ++it;
    }

    auto items = _rootDevice.getItems(search::Recursive(search::Any()));
    for (const auto& item : items)
        if (auto signal = item.asPtrOrNull<daq::ISignal>(); signal.assigned())
            createListener(signal);
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
