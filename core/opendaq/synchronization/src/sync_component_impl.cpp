#include <opendaq/sync_component_impl.h>
#include <opendaq/sync_component.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_deserialize_context_factory.h>

BEGIN_NAMESPACE_OPENDAQ

const char* Interfaces = "interfaces";
const char* SyncronizationLocked = "SyncronizationLocked";
const char* Source = "Source";

SyncComponentImpl::SyncComponentImpl(const ContextPtr& context)
    : Super(), context(context)
{
    Super::addProperty(ObjectProperty(Interfaces, PropertyObject()));
    Super::addProperty(BoolProperty(SyncronizationLocked, false));
    Super::addProperty(ListProperty("InterfaceNames", List<IString>("Interface1", "Interface2", "Interface3")));
    Super::addProperty(SelectionProperty(Source, EvalValue("$InterfaceNames"), 0));
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr SyncComponentImpl::getTypedProperty(const StringPtr& name)
{
    return objPtr.getPropertyValue(name).template asPtr<T>();
}

ErrCode SyncComponentImpl::getSyncLocked(Bool* synchronizationLocked)
{
    return daqTry([&]() {
        *synchronizationLocked = getTypedProperty<IBoolean>(SyncronizationLocked);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode SyncComponentImpl::setSyncLocked(Bool synchronizationLocked)
{
    return Super::setPropertyValue(String(SyncronizationLocked), BooleanPtr(synchronizationLocked));
}

ErrCode SyncComponentImpl::getSelectedSource(Int* selectedSource)
{
    return daqTry([&]() {
        *selectedSource = getTypedProperty<IInteger>(Source);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode SyncComponentImpl::setSelectedSource(Int selectedSource)
{
    return Super::setPropertyValue(String(Source), Integer(selectedSource));
}

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

    PropertyObjectPtr interfacePtr = interface;

    //TBD: Check if interface inherits from SyncInterfaceBaseauto
    StringPtr className = interfacePtr.getClassName();

    if (className != "SyncInterfaceBase")
        return OPENDAQ_ERR_INVALID_ARGUMENT;

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
    return OPENDAQ_ERR_NOTIMPLEMENTED;
    //
    //return daqTry(
    //    [&obj, &serialized, &context, &factoryCallback]()
    //    {
    //        *obj = Super::DeserializePropertyObject(
    //                serialized,
    //                context,
    //                factoryCallback,
    //                   [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
    //                   {
    //                       const auto sync = createWithImplementation<ISyncComponent, SyncComponentImpl>(NullContext());
    //                       return sync;
    //                   }).detach();
    //    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SyncComponent, ISyncComponent,
    IContext*, context
)

END_NAMESPACE_OPENDAQ