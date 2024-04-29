#include "mock_server.h"
#include  <coretypes/impl.h>

using namespace daq;

MockServerImpl::MockServerImpl()
{
}

ErrCode MockServerImpl::stop()
{
    return OPENDAQ_SUCCESS;
}

ErrCode MockServerImpl::getServerId(IString** serverId)
{
    if (serverId != nullptr)
    {
        *serverId = String("MockServer").detach();
    }
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockServer, daq::IServer)
