/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <opendaq/sync_component_private.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename MainInterface, typename ... Interfaces>
class GenericSyncComponentImpl : public ComponentImpl<ISyncComponentPrivate, MainInterface, Interfaces...>
{
public:
    using Super = ComponentImpl<ISyncComponentPrivate, MainInterface, Interfaces...>;

    GenericSyncComponentImpl(const ContextPtr& context,
                             const ComponentPtr& parent,
                             const StringPtr& localId,
                             const StringPtr& className = nullptr,
                             const StringPtr& name = nullptr);

    // ISyncComponent
    ErrCode INTERFACE_FUNC getSyncLocked(Bool* synchronizationLocked) override;
    ErrCode INTERFACE_FUNC setSyncLocked(Bool synchronizationLocked) override;

    ErrCode INTERFACE_FUNC getSelectedSource(Int* selectedSource) override;
    virtual ErrCode INTERFACE_FUNC setSelectedSource(Int selectedSource) override;

    ErrCode INTERFACE_FUNC getInterfaces(IDict** interfaces) override;
    ErrCode INTERFACE_FUNC addInterface(IPropertyObject* syncInterface) override;
    ErrCode INTERFACE_FUNC removeInterface(IString* syncInterfaceName) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    template <typename T>
    typename InterfaceToSmartPtr<T>::SmartPtr getTypedProperty(const StringPtr& name);

    ErrCode checkClassNameIsSyncInterface(const StringPtr& className, const TypeManagerPtr& manager) const;

};

using SyncComponentImpl = GenericSyncComponentImpl<ISyncComponent>;


template <typename MainInterface, typename ... Interfaces>
GenericSyncComponentImpl<MainInterface, Interfaces...>::GenericSyncComponentImpl(const ContextPtr& context,
                             const ComponentPtr& parent,
                             const StringPtr& localId,
                             const StringPtr& className,
                             const StringPtr& name)
    : Super(context, parent, localId, nullptr, "Synchronization")
{
    Super::addProperty(ObjectProperty("Interfaces", PropertyObject()));
    Super::addProperty(SelectionProperty("Source", EvalValue("%Interfaces:PropertyNames"), 0));
    Super::addProperty(BoolProperty("SynchronizationLocked", false));
}

template <typename MainInterface, typename ... Interfaces>
template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr GenericSyncComponentImpl<MainInterface, Interfaces...>::getTypedProperty(const StringPtr& name)
{
    return this->objPtr.getPropertyValue(name).template asPtr<T>();
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::getSyncLocked(Bool* synchronizationLocked)
{
    OPENDAQ_PARAM_NOT_NULL(synchronizationLocked);
    return daqTry([&] 
    {
        *synchronizationLocked = getTypedProperty<IBoolean>("SynchronizationLocked");
        return OPENDAQ_SUCCESS;
    });
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::setSyncLocked(Bool synchronizationLocked)
{
    return Super::setPropertyValue(String("SynchronizationLocked"), BooleanPtr(synchronizationLocked));
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::getSelectedSource(Int* selectedSource)
{
    OPENDAQ_PARAM_NOT_NULL(selectedSource);
    return daqTry([&]
    {
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
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::getInterfaces(IDict** interfaces)
{
    OPENDAQ_PARAM_NOT_NULL(interfaces);
    auto interfacesDict = Dict<IString, IPropertyObject>();

    BaseObjectPtr interfacesValue;
    StringPtr str = "Interfaces";
    ErrCode err = this->getPropertyValue(str, &interfacesValue);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto InterfacesPtr = interfacesValue.asPtr<IPropertyObject>(true);
    for (const auto& prop : InterfacesPtr.getAllProperties())
    {
        if (prop.getValueType() == ctObject)
        {
            StringPtr name = prop.getName();
            BaseObjectPtr interfaceProperty;
            err = InterfacesPtr->getPropertyValue(name, &interfaceProperty);
            if (OPENDAQ_FAILED(err))
                return err;

            interfacesDict.set(name, interfaceProperty);
        }
    }

    *interfaces = interfacesDict.detach();
    return OPENDAQ_SUCCESS;
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::checkClassNameIsSyncInterface(const StringPtr& className, const TypeManagerPtr& manager) const
{
    if (!className.assigned())
        return this->makeErrorInfo(OPENDAQ_ERR_INVALID_ARGUMENT, "Interface name does not inherit from SyncInterfaceBase.");

    TypePtr type;
    ErrCode errCode = manager->getType(className, &type);
    if (OPENDAQ_FAILED(errCode) || type == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_INVALID_ARGUMENT, fmt::format("Interface '{}' is not registered in type manager.", className));

    if (auto objectClass = type.asPtrOrNull<IPropertyObjectClass>(true); objectClass.assigned())
    {
        auto parentName = objectClass.getParentName();
        if (!parentName.assigned())
        {
            return this->makeErrorInfo(OPENDAQ_ERR_INVALID_ARGUMENT, fmt::format("Interface '{}' does not inherit from 'SyncInterfaceBase'.", className));
        }
        if (parentName == "SyncInterfaceBase")
        {
            return OPENDAQ_SUCCESS;
        }
        return checkClassNameIsSyncInterface(parentName, manager);
    }

    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_ARGUMENT, fmt::format("Interface '{}' is not IPropertyObjectClass", className));
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::addInterface(IPropertyObject* syncInterface)
{
    OPENDAQ_PARAM_NOT_NULL(syncInterface);

    PropertyObjectPtr interfacePtr = syncInterface;

    StringPtr className = interfacePtr.getClassName();
    if (className == nullptr)
    {
        return this->makeErrorInfo(OPENDAQ_ERR_INVALID_ARGUMENT, "Interface name is not assigned.");
    }

    if (className == "SyncInterfaceBase")
    {
        return this->makeErrorInfo(OPENDAQ_ERR_INVALID_ARGUMENT, "Allowed adding property objects which inherits from 'SyncInterfaceBase', but not 'SyncInterfaceBase' itself.");
    }

    auto typeManager = this->context.getTypeManager();
    if (typeManager == nullptr)
    {
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "TypeManager is not assigned.");
    }

    ErrCode errCode = checkClassNameIsSyncInterface(className, typeManager);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    BaseObjectPtr interfacesValue;
    StringPtr str = "Interfaces";
    errCode = this->getPropertyValue(str, &interfacesValue);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    const auto interfacesPtr = interfacesValue.asPtr<IPropertyObject>(true);
    return interfacesPtr->addProperty(ObjectProperty(className, syncInterface));
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::removeInterface(IString* interfaceName)
{
    OPENDAQ_PARAM_NOT_NULL(interfaceName);

    BaseObjectPtr interfacesValue;
    StringPtr str = "Interfaces";
    ErrCode err = this->getPropertyValue(str, &interfacesValue);
    if (OPENDAQ_FAILED(err))
        return err;

    Int selectedSource = 0;
    getSelectedSource(&selectedSource);

    const auto InterfacesPtr = interfacesValue.asPtr<IPropertyObject>(true);
    Int idx = 0;
    err = OPENDAQ_ERR_NOTFOUND;
    for (const auto& prop : InterfacesPtr.getAllProperties())
    {
        Bool equals;
        prop.getName()->equals(interfaceName, &equals);
        if (equals)
        {
            err = InterfacesPtr->removeProperty(interfaceName);
            if (OPENDAQ_FAILED(err))
                return err;

            if (selectedSource == idx)
            {
                setSelectedSource(0);
            }
            else if (selectedSource > idx)
            {
                setSelectedSource(selectedSource - 1);
            }
            break;
        }
        idx++;
    }
    return err;
}

template <typename MainInterface, typename ... Interfaces>
ErrCode GenericSyncComponentImpl<MainInterface, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);
    *id = SerializeId();
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

    return daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        *obj = Super::DeserializeComponent(
            serialized,
            context,
            factoryCallback, 
            [](const SerializedObjectPtr&, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className)
            {
                return createWithImplementation<ISyncComponent, GenericSyncComponentImpl>(
                    deserializeContext.getContext(),
                    deserializeContext.getParent(),
                    deserializeContext.getLocalId(),
                    className);
            }).detach();
    });
}


OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SyncComponentImpl)

END_NAMESPACE_OPENDAQ
