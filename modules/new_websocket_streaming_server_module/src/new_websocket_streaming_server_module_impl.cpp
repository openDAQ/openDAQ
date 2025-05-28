#include <new_websocket_streaming_server_module/new_websocket_streaming_server_impl.h>
#include <new_websocket_streaming_server_module/new_websocket_streaming_server_module_impl.h>
#include <new_websocket_streaming_server_module/version.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coretypes/version_info_factory.h>
#include <chrono>

BEGIN_NAMESPACE_OPENDAQ_NEW_WEBSOCKET_STREAMING_SERVER_MODULE

NewWebsocketStreamingServerModule::NewWebsocketStreamingServerModule(ContextPtr context)
    : Module("OpenDAQNewWebsocketStreamingServerModule",
             daq::VersionInfo(NEW_WS_STREAM_SRV_MODULE_MAJOR_VERSION, NEW_WS_STREAM_SRV_MODULE_MINOR_VERSION, NEW_WS_STREAM_SRV_MODULE_PATCH_VERSION),
             std::move(context),
             "OpenDAQNewWebsocketStreamingServerModule")
{
}

DictPtr<IString, IServerType> NewWebsocketStreamingServerModule::onGetAvailableServerTypes()
{
    auto result = Dict<IString, IServerType>();

    auto serverType = NewWebsocketStreamingServerImpl::createType(context);
    result.set(serverType.getId(), serverType);

    return result;
}

ServerPtr NewWebsocketStreamingServerModule::onCreateServer(const StringPtr& serverType,
                                                            const PropertyObjectPtr& serverConfig,
                                                            const DevicePtr& rootDevice)
{
    if (!context.assigned())
        throw InvalidParameterException{"Context parameter cannot be null."};

    auto wsConfig = serverConfig;
    if (!wsConfig.assigned())
        wsConfig = NewWebsocketStreamingServerImpl::createDefaultConfig(context);
    else
        wsConfig = NewWebsocketStreamingServerImpl::populateDefaultConfig(wsConfig, context);

    ServerPtr server(NewWebsocketStreamingServer_Create(rootDevice, wsConfig, context));
    return server;
}

END_NAMESPACE_OPENDAQ_NEW_WEBSOCKET_STREAMING_SERVER_MODULE
