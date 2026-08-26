#include <opendaq/synchronization_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template class GenericSynchronizationImpl<IPropertyObject>;

SynchronizationImpl::SynchronizationImpl(const TypeManagerPtr& manager)
    : Super(manager)
{
    source = createWithImplementation<ISyncInterface, ClockSyncInterfaceImpl>(manager);
    source.asPtr<ISyncInterfaceInternal>(true).setAsSource(true);
    subscribeSourceClockType(source);

    auto interfaces = PropertyObject();
    interfaces.addProperty(ObjectProperty(source.getName(), source));
    this->addProperty(ObjectProperty("Interfaces", interfaces));

    const auto souceProperty = StringPropertyBuilder("Source", source.getName())
                                                        .setSelectionValues(EvalValue("%Interfaces:PropertyNames"))
                                                        .build();
    this->addProperty(souceProperty);

    this->objPtr.getOnPropertyValueWrite("Source") += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr& args)
    {
       onSourceChanged(args.getValue());
    };
}

ErrCode SynchronizationImpl::getSource(ISyncInterface** selectedSource)
{
    OPENDAQ_PARAM_NOT_NULL(selectedSource);
    auto lock = this->getRecursiveConfigLock2();
    *selectedSource = this->source.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SynchronizationImpl::addInterface(ISyncInterface* syncInterface)
{
    OPENDAQ_PARAM_NOT_NULL(syncInterface);
    if (this->getPropertyObjectParent().assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION, "Sync interfaces can only be added before the Synchronization component is attached to its parent");
    
    return daqTry([&]
    {
        const SyncInterfacePtr interfacePtr = SyncInterfacePtr::Borrow(syncInterface);

        const PropertyObjectPtr interfacesProperty = this->objPtr.getPropertyValue("Interfaces");

        interfacesProperty.addProperty(ObjectProperty(interfacePtr.getName(), interfacePtr));
    });
}

void SynchronizationImpl::onSourceChanged(const StringPtr& sourceName)
{
    auto lock = this->getRecursiveConfigLock2();
    const PropertyObjectPtr interfacesProperty = this->objPtr.getPropertyValue("Interfaces");
    SyncInterfacePtr newSource = interfacesProperty.getPropertyValue(sourceName);
    SyncInterfacePtr oldSource = source;
    const auto oldSourceMode = oldSource.getMode();

    oldSource.asPtr<ISyncInterfaceInternal>(true).setAsSource(False);
    unsubscribeSourceClockType(oldSource);

    try
    {
        newSource.asPtr<ISyncInterfaceInternal>(true).setAsSource(True);
        source = newSource.detach();
        subscribeSourceClockType(source);

        // setAsSource can itself change the clock type (or it may never fire a write event at all),
        // so push the current value once instead of relying solely on the subscription
        const StringPtr clockType = source.asPtr<IPropertyObject>(true).getPropertyValue("Status.ClockType");
        notifySourceClockTypeChanged(clockType);
    }
    catch(...)
    {
        oldSource.asPtr<ISyncInterfaceInternal>(true).setAsSource(True);
        subscribeSourceClockType(oldSource);
        oldSource.asPtr<IPropertyObject>(true).setPropertyValue("Mode", static_cast<Int>(oldSourceMode));
        throw;
    }
}

void SynchronizationImpl::onSourceClockTypeChanged(PropertyObjectPtr& /*sender*/, PropertyValueEventArgsPtr& args)
{
    notifySourceClockTypeChanged(args.getValue());
}

void SynchronizationImpl::notifySourceClockTypeChanged(const StringPtr& clockType)
{
    const PropertyObjectPtr interfacesProperty = this->objPtr.getPropertyValue("Interfaces");
    for (const auto& interfaceProp : interfacesProperty.getAllProperties())
    {
        const SyncInterfacePtr syncInterface = interfaceProp.getValue();
        if (syncInterface != source)
            syncInterface.asPtr<ISyncInterfaceInternal>(true).sourceClockTypeChanged(clockType);
    }
}

void SynchronizationImpl::subscribeSourceClockType(const SyncInterfacePtr& syncInterface)
{
    syncInterface.asPtr<IPropertyObject>(true).getOnPropertyValueWrite("Status.ClockType") += event(&SynchronizationImpl::onSourceClockTypeChanged);
}

void SynchronizationImpl::unsubscribeSourceClockType(const SyncInterfacePtr& syncInterface)
{
    syncInterface.asPtr<IPropertyObject>(true).getOnPropertyValueWrite("Status.ClockType") -= event(&SynchronizationImpl::onSourceClockTypeChanged);
}

ErrCode SynchronizationImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_CLONEABLE);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, SynchronizationImpl,
    ISynchronization, createSynchronization,
    ITypeManager*, manager
)

END_NAMESPACE_OPENDAQ
