#include <native_streaming_server_module/native_streaming_server_impl.h>
#include <native_streaming_server_module/native_streaming_server_module_impl.h>
#include <native_streaming_server_module/version.h>
#include <native_streaming_protocol/native_streaming_constants.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coretypes/version_info_factory.h>
#include <opendaq/transport_option_factory.h>
#include <chrono>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

namespace
{
    ListPtr<ITransportOption> CreateTransportOptions()
    {
        using namespace opendaq_native_streaming_protocol;

        auto options = List<ITransportOption>();
        options.pushBack(TransportOption(CONST_NATIVE_TRANSPORT_PLAIN, DEFAULT_PORT, TransportProtection::None, False));
#if NATIVE_STREAMING_ENABLE_TLS
        options.pushBack(TransportOption(CONST_NATIVE_TRANSPORT_TLS, DEFAULT_TLS_PORT, TransportProtection::Encrypted, False));
#endif
        options.freeze();
        return options;
    }
}

NativeStreamingServerModule::NativeStreamingServerModule(ContextPtr context)
    : Module("OpenDAQNativeStreamingServerModule",
             daq::VersionInfo(NATIVE_STREAM_SRV_MODULE_MAJOR_VERSION,
                              NATIVE_STREAM_SRV_MODULE_MINOR_VERSION,
                              NATIVE_STREAM_SRV_MODULE_PATCH_VERSION),
             std::move(context),
             "OpenDAQNativeStreamingServerModule")
    , transportOptions(CreateTransportOptions())
{
}

DictPtr<IString, IServerType> NativeStreamingServerModule::onGetAvailableServerTypes()
{
    auto result = Dict<IString, IServerType>();

    auto serverType = NativeStreamingServerImpl::createType(context, transportOptions);
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
