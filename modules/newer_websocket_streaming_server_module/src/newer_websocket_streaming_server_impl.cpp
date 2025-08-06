#include <iostream>
#include <thread>
#include <utility>

#include <coretypes/impl.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/server_type_factory.h>

#include <ws-streaming/ws-streaming.hpp>

#include <newer_websocket_streaming_server_module/newer_websocket_streaming_listener.h>
#include <newer_websocket_streaming_server_module/newer_websocket_streaming_server_impl.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

NewerWebsocketStreamingServerImpl::NewerWebsocketStreamingServerImpl(const DevicePtr& rootDevice,
                                                                 const PropertyObjectPtr& config,
                                                                 const ContextPtr& context)
    : Server{"OpenDAQNewerLTStreaming", config, rootDevice, context}
    , _ioc{1}
    , _server{_ioc.get_executor()}
{
    _server.add_listener(config.getPropertyValue("WebsocketStreamingPort"));
    _server.add_listener(config.getPropertyValue("WebsocketControlPort"));

    _server.run();

    for (const auto& signal : rootDevice.getSignalsRecursive())
        createListener(signal);

    _server.on_available.connect(
        [self = ObjectPtr<NewerWebsocketStreamingServerImpl>(this)]
        (wss::connection_ptr connection, wss::remote_signal_ptr signal)
        {
            std::cout << "acquired client-supplied signal " << signal->id() << std::endl;

            self->_remoteSignals.emplace(
                signal->id(),
                std::make_shared<RemoteSignalHandler>(signal)).first->second->attach();
        });

    _server.on_unavailable.connect(
        [self = ObjectPtr<NewerWebsocketStreamingServerImpl>(this)]
        (wss::connection_ptr connection, wss::remote_signal_ptr signal)
        {
            std::cout << "lost client-supplied signal " << signal->id() << std::endl;

            if (auto handler = self->_remoteSignals.extract(signal->id()); handler)
                handler.mapped()->detach();
        });

    _thread = std::thread{[this]()
    {
        std::cout << "running io context" << std::endl;
        _ioc.run();
    }};
}

void NewerWebsocketStreamingServerImpl::populateDefaultConfigFromProvider(const ContextPtr& context, const PropertyObjectPtr& config)
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

PropertyObjectPtr NewerWebsocketStreamingServerImpl::createDefaultConfig(const ContextPtr& context)
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

PropertyObjectPtr NewerWebsocketStreamingServerImpl::getDiscoveryConfig()
{
    auto discoveryConfig = PropertyObject();
    discoveryConfig.addProperty(StringProperty("ServiceName", "_streaming-lt._tcp.local."));
    discoveryConfig.addProperty(StringProperty("ServiceCap", "LT"));
    discoveryConfig.addProperty(StringProperty("Path", config.getPropertyValue("Path")));
    discoveryConfig.addProperty(IntProperty("Port", config.getPropertyValue("WebsocketStreamingPort")));
    discoveryConfig.addProperty(StringProperty("ProtocolVersion", ""));
    return discoveryConfig;
}


ServerTypePtr NewerWebsocketStreamingServerImpl::createType(const ContextPtr& context)
{
    return ServerType(
        "OpenDAQNewerLTStreaming",
        "openDAQ LT Streaming server",
        "Publishes device signals as a flat list and streams data over WebsocketTcp protocol",
        NewerWebsocketStreamingServerImpl::createDefaultConfig(context));
}

void NewerWebsocketStreamingServerImpl::onStopServer()
{
    _server.close();
    _thread.join();
}

PropertyObjectPtr NewerWebsocketStreamingServerImpl::populateDefaultConfig(const PropertyObjectPtr& config, const ContextPtr& context)
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

void NewerWebsocketStreamingServerImpl::createListener(const SignalPtr& signal)
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
                wss::metadata_builder(
                        signal.getName())
                    .build()))
        .first->second;

    streamableSignal.localSignal.on_subscribed.connect([
        =,
        &streamableSignal,
        signal_id = signal.getGlobalId().toStdString()
    ]()
    {
        std::cout << signal_id << " subscribed" << std::endl;

        streamableSignal.listener = NewerWebsocketStreamingListener_Create(
            this->template thisPtr<ComponentPtr>().getContext(),
            signal,
            &streamableSignal.localSignal);
        reinterpret_cast<NewerWebsocketStreamingListenerImpl *>(streamableSignal.listener.getObject())->start();
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

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NewerWebsocketStreamingServer, daq::IServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
