#include <idds_server_module/idds_server_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/server_type_factory.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_IDDS_SERVER_MODULE

using namespace daq;

iDDSServerImpl::iDDSServerImpl(DevicePtr rootDevice, const ContextPtr& context)
    : Server("OpenDAQiDDS", config, rootDevice, context)
    //, iDDSServer(rootDevice, context)
{
    //iDDSServer.startwrapper();
}

ServerTypePtr iDDSServerImpl::createType()
{
    return ServerType(
        "iDDS",
        "iDDS",
        "iDDS server module");
}

void iDDSServerImpl::onStopServer()
{
    //iDDSServer.stop();
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    iDDSServer,
    daq::IServer,
    daq::DevicePtr,
    rootDevice,
    const ContextPtr&,
    context
    )

END_NAMESPACE_OPENDAQ_IDDS_SERVER_MODULE
