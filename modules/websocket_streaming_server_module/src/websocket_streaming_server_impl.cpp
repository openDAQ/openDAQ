#include <websocket_streaming_server_module/websocket_streaming_server_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/server_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

using namespace daq;

WebsocketStreamingServerImpl::WebsocketStreamingServerImpl(DevicePtr rootDevice, PropertyObjectPtr config, const ContextPtr& context)
    : Server(config, rootDevice, nullptr, nullptr)
    , websocketStreamingServer(rootDevice, context)
    , config(config)
{
    const uint16_t streamingPort = config.getPropertyValue("Port");
    const uint16_t controlPort = config.getPropertyValue("WebsocketControlPort");

    websocketStreamingServer.setStreamingPort(streamingPort);
    websocketStreamingServer.setControlPort(controlPort);
    websocketStreamingServer.start();
}

void WebsocketStreamingServerImpl::populateDefaultConfigFromProvider(const ContextPtr& context, const PropertyObjectPtr& config)
{
    if (!context.assigned())
        return;
    if (!config.assigned())
        return;

    auto options = context.getModuleOptions("StreamingLtServer");
    for (const auto& [key, value] : options)
    {
        if (config.hasProperty(key))
            config->setPropertyValue(key, value);
        else
        {
            auto property = ObjectPropertyBuilder(key, value).setValueType(value.getCoreType()).build();
            config.addProperty(property);
        }
    }
}

PropertyObjectPtr WebsocketStreamingServerImpl::createDefaultConfig(const ContextPtr& context)
{
    constexpr Int minPortValue = 0;
    constexpr Int maxPortValue = 65535;

    auto defaultConfig = PropertyObject();

    defaultConfig.addProperty(StringProperty("Name", "OpenDAQ_Server"));
    defaultConfig.addProperty(StringProperty("Manufacturer", "openDAQ"));
    defaultConfig.addProperty(StringProperty("Model", ""));
    defaultConfig.addProperty(StringProperty("SerialNumber", ""));
    defaultConfig.addProperty(BoolProperty("ServiceDiscoverable", false));

    const auto websocketPortProp =
        IntPropertyBuilder("Port", 7414).setMinValue(minPortValue).setMaxValue(maxPortValue).build();
    defaultConfig.addProperty(websocketPortProp);

    const auto websocketControlPortProp =
        IntPropertyBuilder("WebsocketControlPort", 7438).setMinValue(minPortValue).setMaxValue(maxPortValue).build();
    defaultConfig.addProperty(websocketControlPortProp);

    const auto websocketServiceProp = StringPropertyBuilder("ServiceName", "_streaming-lt._tcp.local.")
        .setReadOnly(true)
        .build();
    defaultConfig.addProperty(websocketServiceProp);

    const auto serviceCapProp = StringPropertyBuilder("ServiceCap", "LT")
        .setReadOnly(true)
        .build();
    defaultConfig.addProperty(serviceCapProp);

    defaultConfig.addProperty(StringProperty("ServicePath", "/"));

    populateDefaultConfigFromProvider(context, defaultConfig);
    return defaultConfig;
}

ServerTypePtr WebsocketStreamingServerImpl::createType(const ContextPtr& context)
{
    return ServerType(
        "openDAQ LT Streaming",
        "openDAQ LT Streaming server",
        "Publishes device signals as a flat list and streams data over WebsocketTcp protocol",
        WebsocketStreamingServerImpl::createDefaultConfig(context));
}

void WebsocketStreamingServerImpl::onStopServer()
{
    websocketStreamingServer.stop();
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, WebsocketStreamingServer, daq::IServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
