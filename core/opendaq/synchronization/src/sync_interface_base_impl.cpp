#include <opendaq/sync_interface_base_impl.h>
#include <opendaq/component_status_container_private_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template class GenericSyncInterfaceImpl<IPropertyObject, ISyncInterfaceInternal>;

SyncInterfaceBaseImpl::SyncInterfaceBaseImpl(const TypeManagerPtr& manager, const StringPtr& name, const std::vector<SyncMode>& availableModes)
    : Super(manager)
    , manager(manager)
    , name(name)
{
    initAvailiableModes(availableModes);
    initProperties();
    initSynchronizationStatus();
}

void SyncInterfaceBaseImpl::initAvailiableModes(const std::vector<SyncMode>& availableModes)
{
    sourceModes = Dict<IInteger, IString>();
    outputModes = Dict<IInteger, IString>({{static_cast<Int>(SyncMode::Off), "Off"}});

    for (const auto& mode : availableModes)
    {
        switch (mode)
        {
            case SyncMode::Off:
                outputModes.set(static_cast<Int>(mode), "Off");
                break;
            case SyncMode::Input:
                sourceModes.set(static_cast<Int>(mode), "Input");
                break;
            case SyncMode::Output:
                outputModes.set(static_cast<Int>(mode), "Output");
                break;
            case SyncMode::Auto:
                sourceModes.set(static_cast<Int>(mode), "Auto");
                break;
        }
    }
}

void SyncInterfaceBaseImpl::initProperties()
{
    this->objPtr.addProperty(StringPropertyBuilder("Name", name).setReadOnly(true).build());

    status = PropertyObject();
    const EnumerationTypePtr syncRoleStatusType = manager.getType("SynchronizationRoleStatusType");
    const EnumerationTypePtr syncSourceStatusType = manager.getType("SynchronizationSourceStatusType");

    status.addProperty(SelectionPropertyBuilder("SynchronizationRoleStatus", syncRoleStatusType.getEnumeratorNames(), static_cast<Int>(SyncRoleStatus::Off)).setReadOnly(true).build());
    status.addProperty(SelectionPropertyBuilder("SynchronizationSourceStatus", syncSourceStatusType.getEnumeratorNames(), static_cast<Int>(SyncSourceStatus::Off)).setReadOnly(true).build());
    status.addProperty(StringPropertyBuilder("ReferenceDomainId", "").setReadOnly(true).build());
    this->objPtr.addProperty(ObjectPropertyBuilder("Status", status).setReadOnly(true).build());

    configuration = PropertyObject();
    configuration.addProperty(DictPropertyBuilder("ModeOptions", outputModes).setReadOnly(true).setVisible(false).build());
    configuration.addProperty(SparseSelectionProperty("Mode", EvalValue("$ModeOptions"), Integer(SyncMode::Off)));
    configuration.setPropertyOrder(List<IString>("ModeOptions"));
    this->objPtr.addProperty(ObjectProperty("Configuration", configuration));

    configuration.getOnAnyPropertyValueWrite() += [this](PropertyObjectPtr&, PropertyValueEventArgsPtr& arg)
    {
        auto lock = this->getRecursiveConfigLock2();
        onConfigurationChanged(arg.getProperty().getName(), arg.getValue());
    };
}

void SyncInterfaceBaseImpl::initSynchronizationStatus()
{
    const auto statusContainerPrivate = this->statusContainer.asPtr<IComponentStatusContainerPrivate>(true);

    const auto syncRoleStatus =
        EnumerationWithIntValue("SynchronizationRoleStatusType", static_cast<Int>(SyncRoleStatus::Off), manager);
    statusContainerPrivate.addStatus("SynchronizationRoleStatus", syncRoleStatus);
    
    const auto syncSourceStatus =
        EnumerationWithIntValue("SynchronizationSourceStatusType", static_cast<Int>(SyncSourceStatus::Off), manager);
    statusContainerPrivate.addStatus("SynchronizationSourceStatus", syncSourceStatus);
}

ErrCode SyncInterfaceBaseImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SyncInterfaceBaseImpl::getAvailableModes(IDict** availableModes)
{
    OPENDAQ_PARAM_NOT_NULL(availableModes);
    *availableModes = (isSource ? sourceModes : outputModes).addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SyncInterfaceBaseImpl::getReferenceDomainId(IString** referenceDomainId)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainId);
    *referenceDomainId = status.getPropertyValue("ReferenceDomainId").as<IString>();
    return OPENDAQ_SUCCESS;
}

ErrCode SyncInterfaceBaseImpl::getStatus(IPropertyObject** status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    *status = this->status.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SyncInterfaceBaseImpl::getConfiguration(IPropertyObject** configuration)
{
    OPENDAQ_PARAM_NOT_NULL(configuration);
    *configuration = this->configuration.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SyncInterfaceBaseImpl::setAsSource(Bool source)
{
    if (source)
    {
        if (sourceModes.getCount() == 0)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION, "Current sync interface can not be chossen as source");

        OPENDAQ_RETURN_IF_FAILED(this->setProtectedPropertyValue(String("Configuration.ModeOptions"), sourceModes));

        if (sourceModes.hasKey(static_cast<Int>(SyncMode::Auto)))
            OPENDAQ_RETURN_IF_FAILED(this->setMode(SyncMode::Auto));
        else
            OPENDAQ_RETURN_IF_FAILED(this->setMode(SyncMode::Input));

        isSource = True;
        return OPENDAQ_SUCCESS;
    }

    OPENDAQ_RETURN_IF_FAILED(this->setProtectedPropertyValue(String("Configuration.ModeOptions"), outputModes));
    isSource = False;
    OPENDAQ_RETURN_IF_FAILED(this->setPropertyValue(String("Configuration.Mode"), Integer(SyncMode::Off)));
    return OPENDAQ_SUCCESS;
}

void SyncInterfaceBaseImpl::onConfigurationChanged(const StringPtr& name, const BaseObjectPtr& value)
{
}

void SyncInterfaceBaseImpl::setSyncSourceStatus(SyncSourceStatus status, const StringPtr& message)
{
    this->status.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("SynchronizationSourceStatus", static_cast<Int>(status));
    const auto syncSourceStatus =
        EnumerationWithIntValue("SynchronizationSourceStatusType", static_cast<Int>(status), manager);

    const auto statusContainerPrivate = this->statusContainer.asPtr<IComponentStatusContainerPrivate>(true);
    statusContainerPrivate.setStatusWithMessage("SynchronizationSourceStatus", syncSourceStatus, message);
}

void SyncInterfaceBaseImpl::setSyncRoleStatus(SyncRoleStatus status, const StringPtr& message)
{
    this->status.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("SynchronizationRoleStatus", static_cast<Int>(status));
    const auto syncRoleStatus =
        EnumerationWithIntValue("SynchronizationRoleStatusType", static_cast<Int>(status), manager);
    
    const auto statusContainerPrivate = this->statusContainer.asPtr<IComponentStatusContainerPrivate>(true);
    statusContainerPrivate.setStatusWithMessage("SynchronizationRoleStatus", syncRoleStatus, message);
}

ErrCode SyncInterfaceBaseImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_CLONEABLE);
}

ErrCode SyncInterfaceBaseImpl::serializeCustomValues(ISerializer* serializer, bool forUpdate)
{
    OPENDAQ_RETURN_IF_FAILED(Super::serializeCustomValues(serializer, forUpdate));

    if (!forUpdate)
    {
        serializer->key("SyncStatus");
        auto serializable = statusContainer.asOrNull<ISerializable>(true);
        OPENDAQ_RETURN_IF_FAILED(serializable->serialize(serializer));
    }
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
