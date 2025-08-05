#include <newer_websocket_streaming_server_module/newer_websocket_streaming_server_impl.h>
#include <newer_websocket_streaming_server_module/newer_websocket_streaming_server_module_impl.h>
#include <newer_websocket_streaming_server_module/version.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coretypes/version_info_factory.h>
#include <chrono>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

NewerWebsocketStreamingServerModule::NewerWebsocketStreamingServerModule(ContextPtr context)
    : Module("OpenDAQNewerWebsocketStreamingServerModule",
             daq::VersionInfo(NEWER_WS_STREAM_SRV_MODULE_MAJOR_VERSION, NEWER_WS_STREAM_SRV_MODULE_MINOR_VERSION, NEWER_WS_STREAM_SRV_MODULE_PATCH_VERSION),
             std::move(context),
             "OpenDAQNewerWebsocketStreamingServerModule")
{
}

DictPtr<IString, IServerType> NewerWebsocketStreamingServerModule::onGetAvailableServerTypes()
{
    auto result = Dict<IString, IServerType>();

    auto serverType = NewerWebsocketStreamingServerImpl::createType(context);
    result.set(serverType.getId(), serverType);

    return result;
}

ServerPtr NewerWebsocketStreamingServerModule::onCreateServer(const StringPtr& serverType,
                                                              const PropertyObjectPtr& serverConfig,
                                                              const DevicePtr& rootDevice)
{
    if (!context.assigned())
        throw InvalidParameterException{"Context parameter cannot be null."};

    auto wsConfig = serverConfig;
    if (!wsConfig.assigned())
        wsConfig = NewerWebsocketStreamingServerImpl::createDefaultConfig(context);
    else
        wsConfig = NewerWebsocketStreamingServerImpl::populateDefaultConfig(wsConfig, context);

    ServerPtr server(NewerWebsocketStreamingServer_Create(rootDevice, wsConfig, context));
    return server;
}

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
