#include <opendaq/clock_sync_interface_impl.h>

BEGIN_NAMESPACE_OPENDAQ

ClockSyncInterfaceImpl::ClockSyncInterfaceImpl(const TypeManagerPtr& manager)
    : Super(manager, "ClockSyncInterface", {SyncMode::Off, SyncMode::Input})
{
}

ErrCode ClockSyncInterfaceImpl::getSyncType(IString** syncType)
{
    OPENDAQ_PARAM_NOT_NULL(syncType);
    *syncType = String("local").detach();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
