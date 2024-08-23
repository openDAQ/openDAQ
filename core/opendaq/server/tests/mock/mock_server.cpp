#include "mock_server.h"
#include  <coretypes/impl.h>
#include <coreobjects/property_object_factory.h>

using namespace daq;

MockServerImpl::MockServerImpl()
{
}

ErrCode MockServerImpl::enableDiscovery()
{
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerImpl::stop()
{
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerImpl::getId(IString** serverId)
{
    if (serverId != nullptr)
    {
        *serverId = String("MockServer").detach();
    }
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerImpl::getConfig(IPropertyObject** config)
{
    if (config != nullptr)
    {
        *config = PropertyObject().detach();
    }
    return OPENDAQ_SUCCESS;
}



OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockServer, daq::IServer)
