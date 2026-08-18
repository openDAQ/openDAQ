#include <opendaq/clock_sync_interface_impl.h>

BEGIN_NAMESPACE_OPENDAQ

ClockSyncInterfaceImpl::ClockSyncInterfaceImpl(const TypeManagerPtr& manager)
    : Super(manager, "ClockSyncInterface", {SyncMode::Off, SyncMode::Input})
{
}

END_NAMESPACE_OPENDAQ
