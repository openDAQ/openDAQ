#include <websocket_streaming_server_module/websocket_streaming_server_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/server_type_factory.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

using namespace daq;

WebsocketStreamingServerImpl::WebsocketStreamingServerImpl(DevicePtr rootDevice, PropertyObjectPtr config, const ContextPtr& context)
    : Server(config, rootDevice, context, nullptr)
    , websocketStreamingServer(rootDevice, context)
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

    LoggerComponentPtr loggerComponent;

    auto options = context.getModuleOptions("StreamingLtServer");
    for (const auto& [key, value] : options)
    {
        if (config.hasProperty(key))
        {
            ErrCode err = config->setPropertyValue(key, value);
            if (err != OPENDAQ_SUCCESS)
            {
                loggerComponent = context.getLogger().getOrAddComponent("ConfigProvider/Modules/StreamingLtServer");
                LOG_W("Ignoring property \"{}\". Using default value \"{}\"", key, config.getPropertyValue(key));
            }
        }
    }
}

PropertyObjectPtr WebsocketStreamingServerImpl::createDefaultConfig(const ContextPtr& context)
{
    constexpr Int minPortValue = 0;
    constexpr Int maxPortValue = 65535;

    auto defaultConfig = PropertyObject();

    const auto websocketPortProp =
        IntPropertyBuilder("Port", 7414).setMinValue(minPortValue).setMaxValue(maxPortValue).build();
    defaultConfig.addProperty(websocketPortProp);

    const auto websocketControlPortProp =
        IntPropertyBuilder("WebsocketControlPort", 7438).setMinValue(minPortValue).setMaxValue(maxPortValue).build();
    defaultConfig.addProperty(websocketControlPortProp);

    defaultConfig.addProperty(BoolProperty("ServiceDiscoverable", false));
    defaultConfig.addProperty(StringProperty("ServicePath", "/"));

    const auto websocketServiceProp = StringPropertyBuilder("ServiceName", "_streaming-lt._tcp.local.")
        .setReadOnly(true)
        .build();
    defaultConfig.addProperty(websocketServiceProp);

    const auto serviceCapProp = StringPropertyBuilder("ServiceCap", "LT")
        .setReadOnly(true)
        .build();
    defaultConfig.addProperty(serviceCapProp);

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
