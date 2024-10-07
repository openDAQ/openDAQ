#include "opendaq/mock/mock_server.h"

using namespace daq;

MockServerImpl::MockServerImpl(const StringPtr& id, const DevicePtr& rootDevice, const ContextPtr& context)
    : Server(id, nullptr, rootDevice, context)
{
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, MockServer, IServer,
    const StringPtr&, id,
    const DevicePtr&, rootDevice,
    const ContextPtr&, context
)
