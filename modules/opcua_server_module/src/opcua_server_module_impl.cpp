#include <opcua_server_module/opcua_server_impl.h>
#include <opcua_server_module/opcua_server_module_impl.h>
#include <opcua_server_module/version.h>
#include <coretypes/version_info_factory.h>
#include <chrono>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_SERVER_MODULE

OpcUaServerModule::OpcUaServerModule(ContextPtr context)
    : Module("OpenDAQOPCUAServerModule",
             daq::VersionInfo(OPCUA_SERVER_MODULE_MAJOR_VERSION, OPCUA_SERVER_MODULE_MINOR_VERSION, OPCUA_SERVER_MODULE_PATCH_VERSION),
             std::move(context),
             "OpenDAQOPCUAServerModule")
{
}

DictPtr<IString, IServerType> OpcUaServerModule::onGetAvailableServerTypes()
{
    auto result = Dict<IString, IServerType>();

    auto serverType = OpcUaServerImpl::createType(context);
    result.set(serverType.getId(), serverType);

    return result;
}

ServerPtr OpcUaServerModule::onCreateServer(const StringPtr& serverType,
                                            const PropertyObjectPtr& serverConfig,
                                            const DevicePtr& rootDevice)
{
    if (!context.assigned())
        throw InvalidParameterException{"Context parameter cannot be null."};

    PropertyObjectPtr config = serverConfig;
    if (!config.assigned())
        config = OpcUaServerImpl::createDefaultConfig(context);
    else
        config = OpcUaServerImpl::populateDefaultConfig(config, context);

    ServerPtr server(OpcUaServer_Create(rootDevice, config, context));
    return server;
}

END_NAMESPACE_OPENDAQ_OPCUA_SERVER_MODULE
