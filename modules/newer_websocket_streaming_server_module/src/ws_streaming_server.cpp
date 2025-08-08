#include <algorithm>
#include <functional>
#include <thread>
#include <utility>

#include <boost/system/error_code.hpp>

#include <opendaq/opendaq.h>
#include <opendaq/server_impl.h>
#include <opendaq/server_type_factory.h>

#include <ws-streaming/ws-streaming.hpp>

#include <websocket_streaming_server_module/ws_streaming_listener.h>
#include <websocket_streaming_server_module/ws_streaming_server.h>

using namespace std::placeholders;

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

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
        const DevicePtr& rootDevice,
        const PropertyObjectPtr& config,
        const ContextPtr& context)
    : Server{ID, config, rootDevice, context}
    , _rootDevice{rootDevice}
    , _ioc{1}
    , _server{_ioc.get_executor()}
{
    _server.add_listener(config.getPropertyValue("WebsocketStreamingPort"));
    _server.add_listener(config.getPropertyValue("WebsocketControlPort"), true);

    _server.run();

    rescan();

    _thread = std::thread{[this]() { _ioc.run(); }};

    context.getOnCoreEvent() += event(&WsStreamingServer::onCoreEvent);
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
    context.getOnCoreEvent() -= event(&WsStreamingServer::onCoreEvent);

    _server.close();
    _thread.join();
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

void WsStreamingServer::createListener(const SignalPtr& signal)
{
    if (auto domainSignal = signal.getDomainSignal(); domainSignal.assigned())
        createListener(domainSignal);

    auto it = _localSignals.find(signal.getGlobalId());
    if (it != _localSignals.end())
        return;

    std::cout << "registering signal " << signal.getGlobalId() << std::endl;

    auto& streamableSignal = _localSignals.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(
                signal.getGlobalId()),
            std::forward_as_tuple(
                signal.getGlobalId(),
                wss::metadata_builder(signal.getName()).build(),
                signal))
        .first->second;

    streamableSignal.localSignal.on_subscribed.connect([
        =,
        &streamableSignal,
        signal_id = signal.getGlobalId().toStdString()
    ]()
    {
        std::cout << signal_id << " subscribed" << std::endl;

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
        std::cout << signal_id << " unsubscribed" << std::endl;
        streamableSignal.listener.release();
    });

    _server.add_local_signal(streamableSignal.localSignal);
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
        default: break;
    }
}

void WsStreamingServer::onComponentAdded(
    ComponentPtr& component,
    CoreEventArgsPtr& args)
{
    std::cout << "[CORE EVENT] component " << component.getGlobalId() << " added" << std::endl;
    rescan();
}

void WsStreamingServer::onComponentRemoved(
    ComponentPtr& component,
    CoreEventArgsPtr& args)
{
    std::cout << "[CORE EVENT] component " << component.getGlobalId() << " removed" << std::endl;
    rescan();
}

void WsStreamingServer::onComponentUpdateEnd(
    ComponentPtr& component,
    CoreEventArgsPtr& args)
{
    std::cout << "[CORE EVENT] component " << component.getGlobalId() << " updated" << std::endl;
    rescan();
}

void WsStreamingServer::rescan()
{
    auto it = _localSignals.begin();
    while (it != _localSignals.end())
    {
        if (it->second.openDaqSignal.isRemoved())
        {
            std::cout << it->first << " was removed" << std::endl;

            auto jt = it++;
            _server.remove_local_signal(jt->second.localSignal);
            _localSignals.erase(jt);
        }

        else
            ++it;
    }

    for (const auto& signal : _rootDevice.getSignalsRecursive())
        createListener(signal);
}

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
