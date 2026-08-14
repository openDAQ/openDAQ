#include <opendaq/sync_interface_base_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template class GenericSyncInterfaceImpl<IPropertyObject, ISyncInterfaceInternal>;

SyncInterfaceBaseImpl::SyncInterfaceBaseImpl(const StringPtr& name, const std::vector<SyncMode>& availableModes)
    : Super()
    , name(name)
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

    this->objPtr.addProperty(StringPropertyBuilder("Name", name).setReadOnly(true).build());
    this->objPtr.addProperty(DictPropertyBuilder("ModeOptions", outputModes).setReadOnly(true).setVisible(false).build());
    this->objPtr.addProperty(SparseSelectionProperty("Mode", EvalValue("$ModeOptions"), Integer(SyncMode::Off)));
    this->objPtr.setPropertyOrder(List<IString>("ModeOptions"));

    status = PropertyObject();
    status.addProperty(BoolPropertyBuilder("Synchronized", False).setReadOnly(true).build());
    status.addProperty(StringPropertyBuilder("ReferenceDomainId", "").setReadOnly(true).build());
    this->objPtr.addProperty(ObjectPropertyBuilder("Status", status).setReadOnly(true).build());

    configuration = PropertyObject();
    this->objPtr.addProperty(ObjectProperty("Configuration", configuration));
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

ErrCode SyncInterfaceBaseImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_CLONEABLE);
}

ErrCode SyncInterfaceBaseImpl::setAsSource(Bool source)
{
    if (source)
    {
        if (sourceModes.getCount() == 0)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION, "Current sync interface can not be chossen as source");

        OPENDAQ_RETURN_IF_FAILED(this->setProtectedPropertyValue(String("ModeOptions"), sourceModes));

        if (sourceModes.hasKey(static_cast<Int>(SyncMode::Auto)))
            OPENDAQ_RETURN_IF_FAILED(this->setMode(SyncMode::Auto));
        else
            OPENDAQ_RETURN_IF_FAILED(this->setMode(SyncMode::Input));

        isSource = True;
        return OPENDAQ_SUCCESS;
    }

    OPENDAQ_RETURN_IF_FAILED(this->setProtectedPropertyValue(String("ModeOptions"), outputModes));
    isSource = False;
    OPENDAQ_RETURN_IF_FAILED(this->setPropertyValue(String("Mode"), Integer(SyncMode::Off)));
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
