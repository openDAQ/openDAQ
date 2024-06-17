#include <opendaq/sync_component_impl.h>
#include <opendaq/sync_component.h>

BEGIN_NAMESPACE_OPENDAQ

SyncComponentImpl::SyncComponentImpl()
    : Super()
{
    Super::addProperty(ObjectProperty(Interfaces, PropertyObject()));
    Super::addProperty(BoolProperty(SyncronizationLocked, false));
    Super::addProperty(SelectionProperty(Source, List<IString>("Interface1", "Interface2", "Interface3"), 0));
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr SyncComponentImpl::getTypedProperty(const StringPtr& name)
{
    return objPtr.getPropertyValue(name).template asPtr<T>();
}

ErrCode SyncComponentImpl::test()
{
    return OPENDAQ_SUCCESS;
}

ErrCode SyncComponentImpl::getSyncLocked(Bool* syncLocked)
{
    return daqTry([&]() {
        *syncLocked = getTypedProperty<IBoolean>(SyncronizationLocked);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode SyncComponentImpl::getSelectedSource(IString** selectedSource)
{
    return daqTry([&]() {
        *selectedSource = getTypedProperty<IString>(Source).detach();
        return OPENDAQ_SUCCESS;
    });
}

//ErrCode INTERFACE_FUNC setSyncLocked(Bool syncronizationLocked)
//{
//    return Super::setPropertyValue(String(SyncronizationLocked), BooleanPtr(syncronizationLocked));
//}

//ErrCode INTERFACE_FUNC setSelectedSource(IString* selectedSource)
//{
//    OPENDAQ_PARAM_NOT_NULL(selectedSource);
//    return daqTry([&]() {
//        checkErrorInfo(Super::setPropertyValue(String(Source), selectedSource));
//        return OPENDAQ_SUCCESS;
//    });
//}

ErrCode SyncComponentImpl::getInterfaces(IList** interfaces)
{
    OPENDAQ_PARAM_NOT_NULL(interfaces);
    ListPtr<IPropertyObject> interfacesList = List<IPropertyObject>();

    BaseObjectPtr Interfaces;
    StringPtr str = "interfaces";
    ErrCode err = this->getPropertyValue(str, &Interfaces);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto InterfacesPtr = Interfaces.asPtr<IPropertyObject>();
    for (const auto& prop : InterfacesPtr.getAllProperties())
    {
        if (prop.getValueType() == ctObject)
        {
            BaseObjectPtr interfaceProperty;
            err = InterfacesPtr->getPropertyValue(prop.getName(), &interfaceProperty);
            if (OPENDAQ_FAILED(err))
                return err;

            interfacesList.pushBack(interfaceProperty.detach());
        }
    }

    *interfaces = interfacesList.detach();
    return OPENDAQ_SUCCESS;
}


ErrCode SyncComponentImpl::addInterface(IPropertyObject* interface)
{
    OPENDAQ_PARAM_NOT_NULL(interface);

    //TBD: Check if interface inherits from SyncInterfaceBase

    BaseObjectPtr Interfaces;
    StringPtr str = "interfaces";
    ErrCode err = this->getPropertyValue(str, &Interfaces);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto InterfacesPtr = Interfaces.asPtr<IPropertyObject>();
    for (const auto& prop : InterfacesPtr.getAllProperties())
    {
        if (prop.getValueType() != ctObject)
            continue;

        auto interfaceProperty = InterfacesPtr.getPropertyValue(prop.getName());
        //check for duplicates of the interface here
    }

    InterfacesPtr.addProperty(ObjectProperty(interface));
    return OPENDAQ_SUCCESS;
}


ErrCode SyncComponentImpl::removeInterface(IString* interfaceName)
{
    OPENDAQ_PARAM_NOT_NULL(interfaceName);

    BaseObjectPtr Interfaces;
    StringPtr str = "interfaces";
    ErrCode err = this->getPropertyValue(str, &Interfaces);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto InterfacesPtr = Interfaces.asPtr<IPropertyObject>();
    if (!InterfacesPtr.hasProperty(interfaceName))
        return OPENDAQ_ERR_NOTFOUND;


    return InterfacesPtr->removeProperty(interfaceName);
}

ErrCode SyncComponentImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr SyncComponentImpl::SerializeId()
{
    return "Synchronization";
}

ErrCode SyncComponentImpl::Deserialize(ISerializedObject* serialized,
                                                IBaseObject* context,
                                                IFunction* factoryCallback,
                                                IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializePropertyObject(
                    serialized,
                    context,
                    factoryCallback,
                       [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
                       {
                           const auto sync = createWithImplementation<ISyncComponent, SyncComponentImpl>();
                           return sync;
                       }).detach();
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SyncComponent, ISyncComponent
)

END_NAMESPACE_OPENDAQ
