#include <opendaq/sync_interface_base_impl.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

template class GenericSyncInterfaceImpl<IPropertyObject, ISyncInterfaceInternal>;

SyncInterfaceBaseImpl::SyncInterfaceBaseImpl(const TypeManagerPtr& manager,
                                             const StringPtr& id,
                                             const std::vector<SyncMode>& availableModes,
                                             const PropertyObjectPtr& config)
    : Super(manager)
    , config(config)
    , manager(manager)
{
    this->permissionManager.setPermissions(PermissionsBuilder().inherit(true).build());
    config.getPermissionManager().setPermissions(PermissionsBuilder().inherit(true).build());
    initAvailiableModes(availableModes);
    initProperties(id);
    initSynchronizationStatus();
}

void SyncInterfaceBaseImpl::initAvailiableModes(const std::vector<SyncMode>& availableModes)
{
    const auto has = [&](SyncMode mode) { return std::find(availableModes.begin(), availableModes.end(), mode) != availableModes.end(); };

    sourceModes = List<IString>();
    if (has(SyncMode::Auto))
        sourceModes.pushBack("Auto");
    if (has(SyncMode::Input))
        sourceModes.pushBack("Input");

    outputModes = List<IString>("Off");
    if (has(SyncMode::Output))
        outputModes.pushBack("Output");
}

void SyncInterfaceBaseImpl::initProperties(const StringPtr& id)
{
    this->objPtr.addProperty(StringPropertyBuilder("Id", id).setReadOnly(true).build());
    this->objPtr.addProperty(StringPropertyBuilder("ReferenceDomainId", "").setReadOnly(true).build());
    this->objPtr.addProperty(BoolPropertyBuilder("SourceSupported", sourceModes.getCount() > 0).setReadOnly(true).build());
    this->objPtr.addProperty(ListPropertyBuilder("ModeSelection", outputModes).setReadOnly(true).setVisible(false).build());
    this->objPtr.addProperty(StringPropertyBuilder("Mode", "Off").setSelectionValues(EvalValue("$ModeSelection")).build());
    this->objPtr.addProperty(ObjectProperty("Config", config));
    this->objPtr.setPropertyOrder({"ModeSelection"});

    this->objPtr.getOnPropertyValueWrite("Mode") += [this](PropertyObjectPtr&, PropertyValueEventArgsPtr& args)
    {
        const SyncMode mode = syncModeFromName(args.getValue());
        if (mode == SyncMode::Off)
            setReferenceDomainId("");
        onModeChanged(mode);
    };
}

void SyncInterfaceBaseImpl::initSynchronizationStatus()
{
    const auto statusContainerPrivate = this->statusContainer.asPtr<IComponentStatusContainerPrivate>(true);

    const auto syncRoleStatus =
        EnumerationWithIntValue("SyncRoleStatusType", static_cast<Int>(SyncRoleStatus::Off), manager);
    statusContainerPrivate.addStatus("SyncRoleStatus", syncRoleStatus);

    const auto syncSourceStatus =
        EnumerationWithIntValue("SyncSourceStatusType", static_cast<Int>(SyncSourceStatus::Off), manager);
    statusContainerPrivate.addStatus("SyncSourceStatus", syncSourceStatus);
}

ErrCode SyncInterfaceBaseImpl::setAsSource(Bool source)
{
    if (source)
    {
        if (sourceModes.getCount() == 0)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED, "Current sync interface can not be chossen as source");

        OPENDAQ_RETURN_IF_FAILED(this->setProtectedPropertyValue(String("ModeSelection"), sourceModes));
        return this->setPropertyValue(String("Mode"), sourceModes.getItemAt(0));
    }

    OPENDAQ_RETURN_IF_FAILED(this->setProtectedPropertyValue(String("ModeSelection"), outputModes));
    return this->setMode(SyncMode::Off);
}

ErrCode SyncInterfaceBaseImpl::sourceChanged(ISyncInterface* source)
{
    OPENDAQ_PARAM_NOT_NULL(source);
    return OPENDAQ_SUCCESS;
}

void SyncInterfaceBaseImpl::onModeChanged(SyncMode /*mode*/)
{
}

void SyncInterfaceBaseImpl::setReferenceDomainId(const StringPtr& referenceDomainId)
{
    checkErrorInfo(this->setProtectedPropertyValue(String("ReferenceDomainId"), referenceDomainId.assigned() ? referenceDomainId : String("")));
}

void SyncInterfaceBaseImpl::setSyncSourceStatus(SyncSourceStatus status, const StringPtr& message)
{
    const auto syncSourceStatus =
        EnumerationWithIntValue("SyncSourceStatusType", static_cast<Int>(status), manager);

    const auto statusContainerPrivate = this->statusContainer.asPtr<IComponentStatusContainerPrivate>(true);
    statusContainerPrivate.setStatusWithMessage("SyncSourceStatus", syncSourceStatus, message);
}

void SyncInterfaceBaseImpl::setSyncRoleStatus(SyncRoleStatus status, const StringPtr& message)
{
    const auto syncRoleStatus =
        EnumerationWithIntValue("SyncRoleStatusType", static_cast<Int>(status), manager);

    const auto statusContainerPrivate = this->statusContainer.asPtr<IComponentStatusContainerPrivate>(true);
    statusContainerPrivate.setStatusWithMessage("SyncRoleStatus", syncRoleStatus, message);
}

ErrCode SyncInterfaceBaseImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_CLONEABLE);
}

END_NAMESPACE_OPENDAQ
