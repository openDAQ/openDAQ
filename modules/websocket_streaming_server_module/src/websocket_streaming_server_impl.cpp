#include <websocket_streaming_server_module/websocket_streaming_server_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/server_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

using namespace daq;

WebsocketStreamingServerImpl::WebsocketStreamingServerImpl(DevicePtr rootDevice, PropertyObjectPtr config, const ContextPtr& context)
    : Server(nullptr, rootDevice, nullptr, nullptr)
    , websocketStreamingServer(rootDevice, context)
    , config(config)
{
    const uint16_t port = config.getPropertyValue("WebsocketStreamingPort");

    websocketStreamingServer.setStreamingPort(port);
    websocketStreamingServer.start();
}

PropertyObjectPtr WebsocketStreamingServerImpl::createDefaultConfig()
{
    constexpr Int minPortValue = 0;
    constexpr Int maxPortValue = 65535;

    auto defaultConfig = PropertyObject();

    const auto websocketPortProp =
        IntPropertyBuilder("WebsocketStreamingPort", 7414).setMinValue(minPortValue).setMaxValue(maxPortValue).build();

    defaultConfig.addProperty(websocketPortProp);

    return defaultConfig;
}

ServerTypePtr WebsocketStreamingServerImpl::createType()
{
    auto configurationCallback = [](IBaseObject* input, IBaseObject** output) -> ErrCode
    {
        PropertyObjectPtr propObjPtr;
        ErrCode errCode = wrapHandlerReturn(&WebsocketStreamingServerImpl::createDefaultConfig, propObjPtr);
        *output = propObjPtr.detach();
        return errCode;
    };

    return ServerType(
        "openDAQ WebsocketTcp Streaming",
        "openDAQ WebsocketTcp Streaming server",
        "Publishes device signals as a flat list and streams data over WebsocketTcp protocol",
        configurationCallback);
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
