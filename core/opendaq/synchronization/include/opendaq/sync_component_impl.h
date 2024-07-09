/*
 * Copyright 2022-2024 Blueberry d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/listobject.h>
#include <coreobjects/property_object.h>
#include <opendaq/component_impl.h>
#include <opendaq/sync_component_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename MainInterface, typename ... Interfaces>
class GenericSyncComponentImpl : public GenericPropertyObjectImpl<MainInterface, Interfaces...>
{
public:
    using Super = GenericPropertyObjectImpl<MainInterface, Interfaces...>;

    
    explicit GenericSyncComponentImpl(const TypeManagerPtr& manager, const StringPtr& className, const ProcedurePtr& triggerCoreEvent = nullptr);
    explicit GenericSyncComponentImpl(const TypeManagerPtr& manager);

    //ISyncComponent
    virtual ErrCode INTERFACE_FUNC getSyncLocked(Bool* synchronizationLocked) override;
    virtual ErrCode INTERFACE_FUNC setSyncLocked(Bool synchronizationLocked) override;

    virtual ErrCode INTERFACE_FUNC getSelectedSource(Int* selectedSource) override;
    virtual ErrCode INTERFACE_FUNC setSelectedSource(Int selectedSource) override;

    virtual ErrCode INTERFACE_FUNC getInterfaces(IList** interfaces) override;
    virtual ErrCode INTERFACE_FUNC getInterfaceNames(IList** interfaceNames) override;
    virtual ErrCode INTERFACE_FUNC addInterface(IPropertyObject* interface) override;
    virtual ErrCode INTERFACE_FUNC removeInterface(IString* interfaceName) override;

    virtual ErrCode INTERFACE_FUNC getInterfaceIds(SizeT* idCount, IntfID** ids) override;

    // ISerializable
    virtual ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    PropertyObjectPtr createCloneBase() override;

private:
    template <typename T>
    typename InterfaceToSmartPtr<T>::SmartPtr getTypedProperty(const StringPtr& name);

};

using SyncComponentImpl = GenericSyncComponentImpl<ISyncComponent>;

template <typename MainInterface, typename ... Interfaces>
GenericSyncComponentImpl<MainInterface, Interfaces...>::GenericSyncComponentImpl(const TypeManagerPtr& manager, const StringPtr& className, const ProcedurePtr& triggerCoreEvent)
    : Super(manager, className, triggerCoreEvent)
{
    Super::addProperty(ObjectProperty("Interfaces", PropertyObject()));
    Super::addProperty(ListProperty("InterfaceNames", List<IString>()));
    Super::addProperty(SelectionProperty("Source", EvalValue("$InterfaceNames"), 0));
    Super::addProperty(BoolProperty("SyncronizationLocked", false));
}

template <typename MainInterface, typename ... Interfaces>
GenericSyncComponentImpl<MainInterface, Interfaces...>::GenericSyncComponentImpl(const TypeManagerPtr& manager)
    : GenericSyncComponentImpl(manager, nullptr, nullptr)
{
}

template <typename MainInterface, typename ... Interfaces>
template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr GenericSyncComponentImpl<MainInterface, Interfaces...>::getTypedProperty(const StringPtr& name)
{
    return objPtr.getPropertyValue(name).template asPtr<T>();
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::getSyncLocked(Bool* synchronizationLocked)
{
    return daqTry([&]() {
        *synchronizationLocked = getTypedProperty<IBoolean>("SyncronizationLocked");
        return OPENDAQ_SUCCESS;
    });
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::setSyncLocked(Bool synchronizationLocked)
{
    return Super::setPropertyValue(String("SyncronizationLocked"), BooleanPtr(synchronizationLocked));
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::getSelectedSource(Int* selectedSource)
{
    return daqTry([&]() {
        *selectedSource = getTypedProperty<IInteger>("Source");
        return OPENDAQ_SUCCESS;
    });
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::setSelectedSource(Int selectedSource)
{
    return Super::setPropertyValue(String("Source"), Integer(selectedSource));
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::getInterfaces(IList** interfaces)
{
    OPENDAQ_PARAM_NOT_NULL(interfaces);
    ListPtr<IPropertyObject> interfacesList = List<IPropertyObject>();

    BaseObjectPtr Interfaces;
    StringPtr str = "Interfaces";
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

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::getInterfaceNames(IList** interfaceNames)
{
    OPENDAQ_PARAM_NOT_NULL(interfaceNames);
    return daqTry([&]() {
        *interfaceNames = getTypedProperty<IList>("InterfaceNames").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::addInterface(IPropertyObject* interface)
{
    OPENDAQ_PARAM_NOT_NULL(interface);

    PropertyObjectPtr interfacePtr = interface;

    //TBD: Check if interface inherits from SyncInterfaceBase
    StringPtr className = interfacePtr.getClassName();
    if (className != "SyncInterfaceBase")
    {
        auto typeManager = this->manager.getRef();
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
    StringPtr str = "Interfaces";
    ErrCode err = this->getPropertyValue(str, &Interfaces);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto interfacesPtr = Interfaces.asPtr<IPropertyObject>(true);
    err = interfacesPtr->addProperty(ObjectProperty(className, interface));
    if (OPENDAQ_FAILED(err))
        return err;

    return daqTry([&]() {
        ListPtr<IString> interfaceNames = getTypedProperty<IList>("InterfaceNames");
        interfaceNames.pushBack(className);
        return Super::setPropertyValue(String("InterfaceNames"), interfaceNames);
    });
}


template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::removeInterface(IString* interfaceName)
{
    OPENDAQ_PARAM_NOT_NULL(interfaceName);

    BaseObjectPtr Interfaces;
    StringPtr str = "Interfaces";
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

        ListPtr<IString> interfaceNames = getTypedProperty<IList>("InterfaceNames");
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
        return Super::setPropertyValue(String("InterfaceNames"), interfaceNames);
    });
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::getInterfaceIds(SizeT* idCount, IntfID** ids)
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

template <typename MainInterface, typename ... Interfaces>
ConstCharPtr GenericSyncComponentImpl<MainInterface, Interfaces...>::SerializeId()
{
    return "SyncComponent";
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::Deserialize(ISerializedObject* serialized,
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
                    auto manager = context.asPtrOrNull<ITypeManager>(true);
                    const auto sync = createWithImplementation<ISyncComponent, GenericSyncComponentImpl<MainInterface, Interfaces...>>(manager);
                    return sync;
                }).detach();
       });
}

template <typename MainInterface, typename ... Interfaces>
PropertyObjectPtr GenericSyncComponentImpl<MainInterface, Interfaces...>::createCloneBase()
{
    const auto obj = createWithImplementation<ISyncComponent, GenericSyncComponentImpl<MainInterface, Interfaces...>>(manager);
    return obj;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SyncComponentImpl)

END_NAMESPACE_OPENDAQ