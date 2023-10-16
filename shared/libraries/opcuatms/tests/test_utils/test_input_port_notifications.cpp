#include "test_input_port_notifications.h"

using namespace daq;

TestInputPortNotificationsImpl::TestInputPortNotificationsImpl()
    : ImplementationOfWeak<IInputPortNotifications>()
{
}

ErrCode TestInputPortNotificationsImpl::acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept)
{
    *accept = true;
    return OPENDAQ_SUCCESS;
}

ErrCode TestInputPortNotificationsImpl::connected(IInputPort* port)
{
    return OPENDAQ_SUCCESS;
}

ErrCode TestInputPortNotificationsImpl::disconnected(IInputPort* port)
{
    return OPENDAQ_SUCCESS;
}

daq::ErrCode TestInputPortNotificationsImpl::packetReceived(daq::IInputPort* port)
{
    return OPENDAQ_SUCCESS;
}
