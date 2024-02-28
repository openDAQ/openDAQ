#include <websocket_streaming_server_module/websocket_streaming_server_impl.h>
#include <websocket_streaming_server_module/websocket_streaming_server_module_impl.h>
#include <websocket_streaming_server_module/version.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coretypes/version_info_factory.h>
#include <chrono>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

WebsocketStreamingServerModule::WebsocketStreamingServerModule(ContextPtr context)
    : Module("openDAQ Websocket streaming server module",
             daq::VersionInfo(WS_STREAM_SRV_MODULE_MAJOR_VERSION, WS_STREAM_SRV_MODULE_MINOR_VERSION, WS_STREAM_SRV_MODULE_PATCH_VERSION),
             std::move(context),
             "WebsockerStreamingServer")
{
}

DictPtr<IString, IServerType> WebsocketStreamingServerModule::onGetAvailableServerTypes()
{
    auto result = Dict<IString, IServerType>();

    auto serverType = WebsocketStreamingServerImpl::createType();
    result.set(serverType.getId(), serverType);

    return result;
}

ServerPtr WebsocketStreamingServerModule::onCreateServer(StringPtr serverType,
                                                         PropertyObjectPtr serverConfig,
                                                         DevicePtr rootDevice)
{
    if (!context.assigned())
        throw InvalidParameterException{"Context parameter cannot be null."};

    if (!serverConfig.assigned())
        serverConfig = WebsocketStreamingServerImpl::createDefaultConfig();

    ServerPtr server(WebsocketStreamingServer_Create(rootDevice, serverConfig, context));
    return server;
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
