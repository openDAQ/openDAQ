#include <opendaq/clock_sync_interface_impl.h>

BEGIN_NAMESPACE_OPENDAQ

ClockSyncInterfaceImpl::ClockSyncInterfaceImpl(const TypeManagerPtr& manager, const StringPtr& deviceId)
    : Super(manager, "InternalClock", {SyncMode::Off, SyncMode::Input})
{
    if (deviceId.assigned() && deviceId.getLength() != 0)
        internalReferenceDomainId = fmt::format("internal:{}", deviceId);
}

ErrCode ClockSyncInterfaceImpl::getSyncType(IString** syncType)
{
    OPENDAQ_PARAM_NOT_NULL(syncType);
    *syncType = String("internal").detach();
    return OPENDAQ_SUCCESS;
}

ErrCode ClockSyncInterfaceImpl::setAsSource(Bool source)
{
    OPENDAQ_RETURN_IF_FAILED(Super::setAsSource(source));
    return daqTry([&]()
    {
        if (source)
        {
            this->setReferenceDomainId(internalReferenceDomainId);
            this->setSyncRoleStatus(SyncRoleStatus::Input);
            this->setSyncSourceStatus(SyncSourceStatus::Synced);
        }
        else
        {
            this->setReferenceDomainId(String(""));
            this->setSyncRoleStatus(SyncRoleStatus::Off);
            this->setSyncSourceStatus(SyncSourceStatus::Off);
        }
    });
}

END_NAMESPACE_OPENDAQ
