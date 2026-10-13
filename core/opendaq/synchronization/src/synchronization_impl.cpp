#include <opendaq/synchronization_impl.h>
#include <coretypes/serialized_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template class GenericSynchronizationImpl<IPropertyObject>;

SynchronizationImpl::SynchronizationImpl(const TypeManagerPtr& manager, const StringPtr& deviceId)
    : Super(manager)
    , sourceInterfaces(List<IString>())
{
    this->addProperty(ObjectProperty("Interfaces", PropertyObject()));
    this->addProperty(ListPropertyBuilder("SourceInterfaces", List<IString>()).setVisible(false).setReadOnly(true).build());

    source = createWithImplementation<ISyncInterface, ClockSyncInterfaceImpl>(manager, deviceId);
    checkErrorInfo(this->addInterface(source));
    source.asPtr<ISyncInterfaceInternal>(true).setAsSource(true);

    const auto sourceProperty = StringPropertyBuilder("Source", source.getId())
                                                        .setSelectionValues(EvalValue("$SourceInterfaces"))
                                                        .build();
    this->addProperty(sourceProperty);

    // Values serialize in name order, which puts "Source" before the list it selects from.
    // "Source" goes before "Interfaces" so that a load sets the source before the source's own mode.
    this->objPtr.setPropertyOrder({"SourceInterfaces", "Source"});

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

        interfacesProperty.addProperty(ObjectProperty(interfacePtr.getId(), interfacePtr));

        if (interfacePtr.getSourceSupported())
        {
            sourceInterfaces.pushBack(interfacePtr.getId());
            OPENDAQ_RETURN_IF_FAILED(this->setProtectedPropertyValue(String("SourceInterfaces"), sourceInterfaces));
        }
        return OPENDAQ_SUCCESS;
    });
}

void SynchronizationImpl::onSourceChanged(const StringPtr& sourceName)
{
    auto lock = this->getRecursiveConfigLock2();
    const PropertyObjectPtr interfacesProperty = this->objPtr.getPropertyValue("Interfaces");
    SyncInterfacePtr newSource = interfacesProperty.getPropertyValue(sourceName);

    SyncInterfacePtr oldSource = source;
    const auto oldSourceMode = oldSource.getMode();
    const auto newSourceMode = newSource.getMode();

    oldSource.asPtr<ISyncInterfaceInternal>(true).setAsSource(False);

    try
    {
        newSource.asPtr<ISyncInterfaceInternal>(true).setAsSource(True);
        source = newSource.detach();
    }
    catch(...)
    {
        if (OPENDAQ_FAILED(newSource.asPtr<ISyncInterfaceInternal>(true)->setAsSource(False)))
            clearErrorInfo();
        else if (newSourceMode != SyncMode::Off && OPENDAQ_FAILED(newSource->setMode(newSourceMode)))
            clearErrorInfo();

        oldSource.asPtr<ISyncInterfaceInternal>(true).setAsSource(True);
        oldSource.setMode(oldSourceMode);
        throw;
    }

    for (const auto& interfaceProp : interfacesProperty.getAllProperties())
    {
        SyncInterfacePtr interface = interfacesProperty.getPropertyValue(interfaceProp.getName());
        // The return value of sourceChanged is ignored
        const ErrCode errCode = interface.asPtr<ISyncInterfaceInternal>(true)->sourceChanged(source);
        if (OPENDAQ_FAILED(errCode))
            clearErrorInfo();
    }
}

ErrCode SynchronizationImpl::update(ISerializedObject* obj, IBaseObject* config)
{
    // update() holds this object's own values until endUpdate but applies the interfaces at once,
    // so the saved source is set before the update block starts.
    OPENDAQ_RETURN_IF_FAILED(applySavedSource(obj));
    return Super::update(obj, config);
}

ErrCode SynchronizationImpl::updateInternal(ISerializedObject* obj, IBaseObject* context)
{
    OPENDAQ_RETURN_IF_FAILED(applySavedSource(obj));
    return Super::updateInternal(obj, context);
}

// The source is set before the interfaces, otherwise the source's saved mode is checked against the
// modes of a non-source interface and rejected. A source equal to the default is not saved, and the
// clear that would restore it is ignored, so a missing source is set to the default here.
ErrCode SynchronizationImpl::applySavedSource(ISerializedObject* obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        StringPtr savedSource = this->objPtr.getProperty("Source").getDefaultValue();

        const auto serialized = SerializedObjectPtr::Borrow(obj);
        if (serialized.hasKey("propValues"))
        {
            const auto values = serialized.readSerializedObject("propValues");
            if (values.hasKey("Source"))
                savedSource = values.readString("Source");
        }

        checkErrorInfo(this->setPropertyValue(String("Source"), savedSource));
    });
}

// The values derive from the interfaces and the selected source; a clear would leave no valid source.
ErrCode SynchronizationImpl::clearPropertyValue(IString* /*propertyName*/)
{
    return OPENDAQ_IGNORED;
}

ErrCode SynchronizationImpl::clearProtectedPropertyValue(IString* /*propertyName*/)
{
    return OPENDAQ_IGNORED;
}

ErrCode SynchronizationImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_CLONEABLE);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, SynchronizationImpl,
    ISynchronization, createSynchronization,
    ITypeManager*, manager,
    IString*, deviceId
)

END_NAMESPACE_OPENDAQ
