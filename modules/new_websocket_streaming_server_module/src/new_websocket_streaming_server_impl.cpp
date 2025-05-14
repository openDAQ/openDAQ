#include <new_websocket_streaming_server_module/new_websocket_streaming_server_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/server_type_factory.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_NEW_WEBSOCKET_STREAMING_SERVER_MODULE

using namespace daq;

NewWebsocketStreamingServerImpl::NewWebsocketStreamingServerImpl(const DevicePtr& rootDevice,
                                                                 const PropertyObjectPtr& config,
                                                                 const ContextPtr& context)
    : Server("OpenDAQNewLTStreaming", config, rootDevice, context)
    , server(rootDevice,
        config.getPropertyValue("WebsocketStreamingPort"),
        config.getPropertyValue("WebsocketControlPort"))
{
}

void NewWebsocketStreamingServerImpl::populateDefaultConfigFromProvider(const ContextPtr& context, const PropertyObjectPtr& config)
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

PropertyObjectPtr NewWebsocketStreamingServerImpl::createDefaultConfig(const ContextPtr& context)
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

PropertyObjectPtr NewWebsocketStreamingServerImpl::getDiscoveryConfig()
{
    auto discoveryConfig = PropertyObject();
    discoveryConfig.addProperty(StringProperty("ServiceName", "_streaming-lt._tcp.local."));
    discoveryConfig.addProperty(StringProperty("ServiceCap", "LT"));
    discoveryConfig.addProperty(StringProperty("Path", config.getPropertyValue("Path")));
    discoveryConfig.addProperty(IntProperty("Port", config.getPropertyValue("WebsocketStreamingPort")));
    discoveryConfig.addProperty(StringProperty("ProtocolVersion", ""));
    return discoveryConfig;
}


ServerTypePtr NewWebsocketStreamingServerImpl::createType(const ContextPtr& context)
{
    return ServerType(
        "OpenDAQNewLTStreaming",
        "openDAQ LT Streaming server",
        "Publishes device signals as a flat list and streams data over WebsocketTcp protocol",
        NewWebsocketStreamingServerImpl::createDefaultConfig(context));
}

void NewWebsocketStreamingServerImpl::onStopServer()
{
}

PropertyObjectPtr NewWebsocketStreamingServerImpl::populateDefaultConfig(const PropertyObjectPtr& config, const ContextPtr& context)
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

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NewWebsocketStreamingServer, daq::IServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_NEW_WEBSOCKET_STREAMING_SERVER_MODULE
