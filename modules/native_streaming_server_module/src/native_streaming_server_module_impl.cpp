#include <native_streaming_server_module/native_streaming_server_impl.h>
#include <native_streaming_server_module/native_streaming_server_module_impl.h>
#include <native_streaming_server_module/version.h>
#include <native_streaming_protocol/native_streaming_constants.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coretypes/version_info_factory.h>
#include <chrono>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

namespace
{
    DictPtr<IString, IInteger> CreateAvailableTransports()
    {
        using namespace opendaq_native_streaming_protocol;

        auto transports = Dict<IString, IInteger>();
        transports.set(CONST_NATIVE_TRANSPORT_PLAIN, static_cast<Int>(DEFAULT_PORT));
#if NATIVE_STREAMING_ENABLE_TLS
        transports.set(CONST_NATIVE_TRANSPORT_TLS, static_cast<Int>(DEFAULT_TLS_PORT));
#endif
        transports.freeze();
        return transports;
    }
}

NativeStreamingServerModule::NativeStreamingServerModule(ContextPtr context)
    : Module("OpenDAQNativeStreamingServerModule",
             daq::VersionInfo(NATIVE_STREAM_SRV_MODULE_MAJOR_VERSION,
                              NATIVE_STREAM_SRV_MODULE_MINOR_VERSION,
                              NATIVE_STREAM_SRV_MODULE_PATCH_VERSION),
             std::move(context),
             "OpenDAQNativeStreamingServerModule")
    , availableTransports(CreateAvailableTransports())
{
}

DictPtr<IString, IServerType> NativeStreamingServerModule::onGetAvailableServerTypes()
{
    auto result = Dict<IString, IServerType>();

    auto serverType = NativeStreamingServerImpl::createType(context, availableTransports);
    result.set(serverType.getId(), serverType);

    return result;
}

ServerPtr NativeStreamingServerModule::onCreateServer(const StringPtr& serverType,
                                                      const PropertyObjectPtr& serverConfig,
                                                      const DevicePtr& rootDevice)
{
    if (!context.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Context parameter cannot be null.");

    PropertyObjectPtr config = serverConfig;
    if (!config.assigned())
        config = NativeStreamingServerImpl::createDefaultConfig(context);
    else
        config = NativeStreamingServerImpl::populateDefaultConfig(config, context);

    ServerPtr server(NativeStreamingServer_Create(rootDevice, config, context));
    return server;
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
