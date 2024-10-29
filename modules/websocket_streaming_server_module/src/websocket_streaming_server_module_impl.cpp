#include <websocket_streaming_server_module/websocket_streaming_server_impl.h>
#include <websocket_streaming_server_module/websocket_streaming_server_module_impl.h>
#include <websocket_streaming_server_module/version.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coretypes/version_info_factory.h>
#include <chrono>
#include <opendaq/module_info_factory.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
WebsocketStreamingServerModule::WebsocketStreamingServerModule(const ContextPtr& ctx)
    : Module(
          daq::ModuleInfo(
              daq::VersionInfo(WS_STREAM_SRV_MODULE_MAJOR_VERSION, WS_STREAM_SRV_MODULE_MINOR_VERSION, WS_STREAM_SRV_MODULE_PATCH_VERSION),
              "OpenDAQWebsocketStreamingServerModule",
              "OpenDAQWebsocketStreamingServerModule"),
          ctx)
{
}

DictPtr<IString, IServerType> WebsocketStreamingServerModule::onGetAvailableServerTypes()
{
    auto result = Dict<IString, IServerType>();

    auto serverType = WebsocketStreamingServerImpl::createType(context);
    result.set(serverType.getId(), serverType);

    return result;
}

ServerPtr WebsocketStreamingServerModule::onCreateServer(const StringPtr& serverType,
                                                         const PropertyObjectPtr& serverConfig,
                                                         const DevicePtr& rootDevice)
{
    if (!context.assigned())
        throw InvalidParameterException{"Context parameter cannot be null."};

    auto wsConfig = serverConfig;
    if (!wsConfig.assigned())
        wsConfig = WebsocketStreamingServerImpl::createDefaultConfig(context);
    else
        wsConfig = WebsocketStreamingServerImpl::populateDefaultConfig(wsConfig, context);

    ServerPtr server(WebsocketStreamingServer_Create(rootDevice, wsConfig, context));
    return server;
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
