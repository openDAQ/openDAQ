#include <idds_server_module/idds_server_impl.h>
#include <idds_server_module/idds_server_module_impl.h>
#include <idds_server_module/version.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coretypes/version_info_factory.h>

BEGIN_NAMESPACE_OPENDAQ_IDDS_SERVER_MODULE

iDDSServerModule::iDDSServerModule(ContextPtr context)
    : Module("OpenDAQiDDSServerModule",
             daq::VersionInfo(IDDS_SRV_MODULE_MAJOR_VERSION, IDDS_SRV_MODULE_MINOR_VERSION, IDDS_SRV_MODULE_PATCH_VERSION),
             std::move(context),
             "OpenDAQiDDSServerModule")
{
}

DictPtr<IString, IServerType> iDDSServerModule::onGetAvailableServerTypes()
{
    auto result = Dict<IString, IServerType>();

    auto serverType = iDDSServerImpl::createType();
    result.set(serverType.getId(), serverType);

    return result;
}

ServerPtr iDDSServerModule::onCreateServer(StringPtr serverType,
                                                         PropertyObjectPtr serverConfig,
                                                         DevicePtr rootDevice)
{
    ServerPtr server(iDDSServer_Create(rootDevice, context));
    return server;
}

END_NAMESPACE_OPENDAQ_IDDS_SERVER_MODULE
