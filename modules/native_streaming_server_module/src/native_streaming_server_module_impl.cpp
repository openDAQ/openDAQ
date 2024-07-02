#include <native_streaming_server_module/native_streaming_server_impl.h>
#include <native_streaming_server_module/native_streaming_server_module_impl.h>
#include <native_streaming_server_module/version.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coretypes/version_info_factory.h>
#include <chrono>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

NativeStreamingServerModule::NativeStreamingServerModule(ContextPtr context)
    : Module("OpenDAQNativeStreamingServerModule",
             daq::VersionInfo(NATIVE_STREAM_SRV_MODULE_MAJOR_VERSION,
                              NATIVE_STREAM_SRV_MODULE_MINOR_VERSION,
                              NATIVE_STREAM_SRV_MODULE_PATCH_VERSION),
             std::move(context),
             "OpenDAQNativeStreamingServerModule")
{
}

DictPtr<IString, IServerType> NativeStreamingServerModule::onGetAvailableServerTypes()
{
    auto result = Dict<IString, IServerType>();

    auto serverType = NativeStreamingServerImpl::createType(context);
    result.set(serverType.getId(), serverType);

    return result;
}

ServerPtr NativeStreamingServerModule::onCreateServer(StringPtr serverType,
                                                PropertyObjectPtr serverConfig,
                                                DevicePtr rootDevice)
{
    if (!context.assigned())
        throw InvalidParameterException{"Context parameter cannot be null."};

    PropertyObjectPtr config = serverConfig;
    if (!config.assigned())
        config = NativeStreamingServerImpl::createDefaultConfig(context);
    else
        config = NativeStreamingServerImpl::populateDefaultConfig(config, context);

    ServerPtr server(NativeStreamingServer_Create(rootDevice, config, context));
    return server;
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
