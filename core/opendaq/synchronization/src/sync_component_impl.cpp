#include <opendaq/sync_component_impl.h>
#include <opendaq/sync_component.h>
#include <coretypes/type_manager_factory.h>
#include <opendaq/component_deserialize_context_factory.h>

BEGIN_NAMESPACE_OPENDAQ

const char* InterfacesKey = "Interfaces";
const char* InterfaceNamesKey = "InterfaceNames";
const char* SourceKey = "Source";
const char* SyncronizationLockedKey = "SyncronizationLocked";

SyncComponentImpl::SyncComponentImpl(const TypeManagerPtr& typeManager)
    : Super()
    , typeManager(typeManager)
{
    Super::addProperty(ObjectProperty(InterfacesKey, PropertyObject()));
    Super::addProperty(ListProperty(InterfaceNamesKey, List<IString>()));
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

ErrCode SyncComponentImpl::getInterfaceNames(IList** interfaceNames)
{
    OPENDAQ_PARAM_NOT_NULL(interfaceNames);
    return daqTry([&]() {
        *interfaceNames = getTypedProperty<IList>(InterfaceNamesKey).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode SyncComponentImpl::addInterface(IPropertyObject* interface)
{
    OPENDAQ_PARAM_NOT_NULL(interface);

    PropertyObjectPtr interfacePtr = interface;

    //TBD: Check if interface inherits from SyncInterfaceBase
    StringPtr className = interfacePtr.getClassName();
    if (className != "SyncInterfaceBase")
    {
        if (typeManager == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "TypeManager is not assigned.");
        }

        TypePtr type;
        ErrCode errCode = typeManager->getType(className, &type);
        if (OPENDAQ_FAILED(errCode) || type == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_INVALID_ARGUMENT, fmt::format("Interface '{}' not found.", className));
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
    err = interfacesPtr->addProperty(ObjectProperty(className, interface));
    if (OPENDAQ_FAILED(err))
        return err;

    return daqTry([&]() {
        ListPtr<IString> interfaceNames = getTypedProperty<IList>(InterfaceNamesKey);
        interfaceNames.pushBack(className);
        return Super::setPropertyValue(String(InterfaceNamesKey), interfaceNames);
    });
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
    err = InterfacesPtr->removeProperty(interfaceName);
    if (OPENDAQ_FAILED(err))
        return err;

    return daqTry([&]() {
        Int selectedSource = 0;
        getSelectedSource(&selectedSource);

        ListPtr<IString> interfaceNames = getTypedProperty<IList>(InterfaceNamesKey);
        for (SizeT i = 0; i < interfaceNames.getCount(); i++)
        {
            Bool equals;
            interfaceNames[i]->equals(interfaceName, &equals);
            if (equals)
            {
                if (selectedSource == Int(i))
                {
                    setSelectedSource(0);
                }
                else if (selectedSource > Int(i))
                {
                    setSelectedSource(selectedSource - 1);
                }
                interfaceNames.removeAt(i);
                break;
            }
        }
        return Super::setPropertyValue(String(InterfaceNamesKey), interfaceNames);
    });
}

ErrCode SyncComponentImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ErrCode SyncComponentImpl::getInterfaceIds(SizeT* idCount, IntfID** ids)
{
    if (idCount == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *idCount = InterfaceIds::Count() + 1;
    if (ids == nullptr)
    {
        return OPENDAQ_SUCCESS;
    }

    **ids = IPropertyObject::Id;
    (*ids)++;

    InterfaceIds::AddInterfaceIds(*ids);
    return OPENDAQ_SUCCESS;
}

ConstCharPtr SyncComponentImpl::SerializeId()
{
    return "SyncComponent";
}

ErrCode SyncComponentImpl::Deserialize(ISerializedObject* serialized,
                                                IBaseObject* context,
                                                IFunction* factoryCallback,
                                                IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_PARAM_NOT_NULL(context); 
    
    return daqTry(
       [&obj, &serialized, &context, &factoryCallback]()
       {
           *obj = Super::DeserializePropertyObject(
                serialized,
                context,
                factoryCallback,
                [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& context, const StringPtr& /*className*/)
                {
                    auto typeManager = context.asPtrOrNull<ITypeManager>(true);
                    const auto sync = createWithImplementation<ISyncComponent, SyncComponentImpl>(typeManager);
                    return sync;
                }).detach();
       });
}

PropertyObjectPtr SyncComponentImpl::createCloneBase()
{
    const auto obj = createWithImplementation<ISyncComponent, SyncComponentImpl>(typeManager);
    return obj;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SyncComponent, ISyncComponent,
    ITypeManager*, typeManager
)

END_NAMESPACE_OPENDAQ