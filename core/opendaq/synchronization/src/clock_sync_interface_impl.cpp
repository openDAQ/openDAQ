#include <opendaq/clock_sync_interface_impl.h>

BEGIN_NAMESPACE_OPENDAQ

ClockSyncInterfaceImpl::ClockSyncInterfaceImpl()
    : Super("ClockSyncInterface", {SyncMode::Off, SyncMode::Input})
{
}

END_NAMESPACE_OPENDAQ
