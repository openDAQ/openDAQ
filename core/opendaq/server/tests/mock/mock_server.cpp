#include "mock_server.h"
#include  <coretypes/impl.h>

using namespace daq;

MockServerImpl::MockServerImpl()
    : Super()
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

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockServer, daq::IServer)
