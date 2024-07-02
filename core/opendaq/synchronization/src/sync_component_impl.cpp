#include <opendaq/sync_component_impl.h>
#include <opendaq/sync_component.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_deserialize_context_factory.h>

BEGIN_NAMESPACE_OPENDAQ

const char* InterfacesKey = "Interfaces";
const char* InterfaceNamesKey = "InterfaceNames";
const char* SourceKey = "Source";
const char* SyncronizationLockedKey = "SyncronizationLocked";

SyncComponentImpl::SyncComponentImpl(const ContextPtr& context)
    : Super(), context(context)
{
    Super::addProperty(ObjectProperty(InterfacesKey, PropertyObject()));
    Super::addProperty(ListProperty(InterfaceNamesKey, List<IString>("Interface1", "Interface2", "Interface3")));
    Super::addProperty(SelectionProperty(SourceKey, EvalValue("$InterfaceNames"), 0));
    Super::addProperty(BoolProperty(SyncronizationLockedKey, false));
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr SyncComponentImpl::getTypedProperty(const StringPtr& name)
{
    return objPtr.getPropertyValue(name).template asPtr<T>();
}

ErrCode SyncComponentImpl::getSyncLocked(Bool* synchronizationLocked)
{
    return daqTry([&]() {
        *synchronizationLocked = getTypedProperty<IBoolean>(SyncronizationLockedKey);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode SyncComponentImpl::setSyncLocked(Bool synchronizationLocked)
{
    return Super::setPropertyValue(String(SyncronizationLockedKey), BooleanPtr(synchronizationLocked));
}

ErrCode SyncComponentImpl::getSelectedSource(Int* selectedSource)
{
    return daqTry([&]() {
        *selectedSource = getTypedProperty<IInteger>(SourceKey);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode SyncComponentImpl::setSelectedSource(Int selectedSource)
{
    return Super::setPropertyValue(String(SourceKey), Integer(selectedSource));
}

ErrCode SyncComponentImpl::getInterfaces(IList** interfaces)
{
    OPENDAQ_PARAM_NOT_NULL(interfaces);
    ListPtr<IPropertyObject> interfacesList = List<IPropertyObject>();

    BaseObjectPtr Interfaces;
    StringPtr str = InterfacesKey;
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

    //TBD: Check if interface inherits from SyncInterfaceBase
    StringPtr className = interfacePtr.getClassName();

    if (className != "SyncInterfaceBase")
    {
        auto typeManager = context.getTypeManager();
        if (typeManager == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "TypeManager is not assigned.");
        }

        TypePtr type;
        ErrCode errCode = typeManager->getType(className, &type);
        if (OPENDAQ_FAILED(errCode) || type == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format("Interface '{}' not found.", className));
        }

        if (auto objectClass = type.asPtrOrNull<IPropertyObjectClass>(true); objectClass.assigned())
        {
            auto parentName = objectClass.getParentName();
            if (!parentName.assigned() || parentName != "SyncInterfaceBase")
            {
                return OPENDAQ_ERR_INVALID_ARGUMENT;
            }
        }
        else
        {
            return OPENDAQ_ERR_INVALID_ARGUMENT;
        }
    }

    BaseObjectPtr Interfaces;
    StringPtr str = InterfacesKey;
    ErrCode err = this->getPropertyValue(str, &Interfaces);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto interfacesPtr = Interfaces.asPtr<IPropertyObject>(true);
    return interfacesPtr->addProperty(ObjectProperty(className, interface));
}


ErrCode SyncComponentImpl::removeInterface(IString* interfaceName)
{
    OPENDAQ_PARAM_NOT_NULL(interfaceName);

    BaseObjectPtr Interfaces;
    StringPtr str = InterfacesKey;
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