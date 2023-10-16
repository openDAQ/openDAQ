#include "opendaq/mock/mock_server.h"

using namespace daq;

MockServerImpl::MockServerImpl()
    : Server(nullptr, nullptr, nullptr, nullptr)
{
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockServer, IServer)
