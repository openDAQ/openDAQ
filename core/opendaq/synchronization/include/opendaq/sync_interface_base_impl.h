/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#include <opendaq/sync_interface_ptr.h>
#include <opendaq/component_status_container_ptr.h>
#include <opendaq/sync_interface_internal.h>
#include <opendaq/component_status_container_impl.h>
#include <coreobjects/property_object_impl.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/eval_value_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/objectptr.h>
#include <coretypes/deserializer.h>

BEGIN_NAMESPACE_OPENDAQ

inline ObjectPtr<IInteger> Integer(SyncMode mode)
{
   return Integer(static_cast<Int>(mode));
}

template <typename TInterface = IPropertyObject, typename... Interfaces>
class GenericSyncInterfaceImpl : public GenericPropertyObjectImpl<TInterface, ISyncInterface, Interfaces...>
{
public:
    using Super = GenericPropertyObjectImpl<TInterface, ISyncInterface, Interfaces...>;

    GenericSyncInterfaceImpl(const TypeManagerPtr& manager);

    // ISyncInterface
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) override;
    ErrCode INTERFACE_FUNC setMode(SyncMode mode) override;
    ErrCode INTERFACE_FUNC getMode(SyncMode* sourceMode) override;
    ErrCode INTERFACE_FUNC getAvailableModes(IDict** availableModes) override;
    ErrCode INTERFACE_FUNC getStatus(IPropertyObject** status) override;
    ErrCode INTERFACE_FUNC getConfiguration(IPropertyObject** configuration) override;
    ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** syncStatus) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

private:
    void triggerCoreEvent(const CoreEventArgsPtr& args);

protected:
    ComponentStatusContainerPtr statusContainer;
};

#ifdef _MSC_VER
    #pragma warning(push)
    // SyncInterfaceBaseImpl's data members and base class don't have dll-interface, but they are never
    // accessed outside of this DLL directly - only through the exported ABI (ISyncInterface, ...) methods.
    #pragma warning(disable : 4251)
    #pragma warning(disable : 4275)
#endif

class PUBLIC_EXPORT SyncInterfaceBaseImpl : public GenericSyncInterfaceImpl<IPropertyObject, ISyncInterfaceInternal>
{
public:
    using Super = GenericSyncInterfaceImpl<IPropertyObject, ISyncInterfaceInternal>;

    // ISyncInterface
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) override;
    ErrCode INTERFACE_FUNC getAvailableModes(IDict** availableModes) override;
    ErrCode INTERFACE_FUNC getStatus(IPropertyObject** status) override;
    ErrCode INTERFACE_FUNC getConfiguration(IPropertyObject** configuration) override;

    // ISyncInterfaceInternal
    ErrCode INTERFACE_FUNC setAsSource(Bool source) override;
    ErrCode INTERFACE_FUNC sourceChanged(ISyncInterface* source) override;
    ErrCode INTERFACE_FUNC getClockType(IString** clockType) override;

    // IPropertyObjectInternal
    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

    // Serialization
    ErrCode serializeCustomValues(ISerializer* serializer, bool forUpdate) override;

protected:
    explicit SyncInterfaceBaseImpl(const TypeManagerPtr& manager,
                                   const StringPtr& name,
                                   const std::vector<SyncMode>& availableModes);

    virtual void onConfigurationChanged(const StringPtr& name, const BaseObjectPtr& value);

    void setReferenceDomainId(const StringPtr& referenceDomainId);
    void setSyncSourceStatus(SyncSourceStatus status, const StringPtr& message = "");
    void setSyncRoleStatus(SyncRoleStatus status, const StringPtr& message = "");
   
    PropertyObjectPtr configuration;
    PropertyObjectPtr status;
    TypeManagerPtr manager;

private:

    void initAvailiableModes(const std::vector<SyncMode>& availableModes);
    void initProperties();
    void initSynchronizationStatus();

    const StringPtr name;
    Bool isSource = False;
    StringPtr referenceDomainId;
    DictPtr<IInteger, IString> sourceModes;
    DictPtr<IInteger, IString> outputModes;
};

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

template <typename TInterface, typename... Interfaces>
GenericSyncInterfaceImpl<TInterface, Interfaces...>::GenericSyncInterfaceImpl(const TypeManagerPtr& manager)
    : Super(manager, "")
{
    statusContainer = createWithImplementation<IComponentStatusContainer, ComponentStatusContainerImpl>(
        [this](const CoreEventArgsPtr& args)
        {
            triggerCoreEvent(args);
        });
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    return daqTry([&]
    {
        *name = this->objPtr.getPropertyValue("Name").template as<IString>();
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::setMode(SyncMode mode)
{
    const ErrCode errCode = this->setPropertyValue(String("Configuration.Mode"), Integer(mode));
    OPENDAQ_RETURN_IF_FAILED(errCode, "Failed to set sync interface mode");
    return errCode;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getMode(SyncMode* sourceMode)
{
    OPENDAQ_PARAM_NOT_NULL(sourceMode);

    BaseObjectPtr objPtr;
    OPENDAQ_RETURN_IF_FAILED(this->getPropertyValue(String("Configuration.Mode"), &objPtr));

    Int value = 0;
    const IntegerPtr modeValue = objPtr.asPtrOrNull<IInteger>();
    if (modeValue.assigned())
        OPENDAQ_RETURN_IF_FAILED(modeValue->getValue(&value));

    *sourceMode = static_cast<SyncMode>(value);
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getAvailableModes(IDict** availableModes)
{
    OPENDAQ_PARAM_NOT_NULL(availableModes);
    BaseObjectPtr objPtr;
    OPENDAQ_RETURN_IF_FAILED(this->getPropertyValue(String("Configuration.ModeOptions"), &objPtr));
    *availableModes = objPtr.asOrNull<IDict>();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getReferenceDomainId(IString** referenceDomainId)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainId);
    BaseObjectPtr objPtr;
    OPENDAQ_RETURN_IF_FAILED(this->getPropertyValue(String("Status.ReferenceDomainId"), &objPtr));
    *referenceDomainId = objPtr.asOrNull<IString>();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getStatus(IPropertyObject** status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    BaseObjectPtr objPtr;
    OPENDAQ_RETURN_IF_FAILED(this->getPropertyValue(String("Status"), &objPtr));
    *status = objPtr.asOrNull<IPropertyObject>();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getConfiguration(IPropertyObject** configuration)
{
    OPENDAQ_PARAM_NOT_NULL(configuration);
    BaseObjectPtr objPtr;
    OPENDAQ_RETURN_IF_FAILED(this->getPropertyValue(String("Configuration"), &objPtr));
    *configuration = objPtr.asOrNull<IPropertyObject>();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getStatusContainer(IComponentStatusContainer** syncStatus)
{
    OPENDAQ_PARAM_NOT_NULL(syncStatus);
    *syncStatus = statusContainer.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);
    *id = "SyncInterface";
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
void GenericSyncInterfaceImpl<TInterface, Interfaces...>::triggerCoreEvent(const CoreEventArgsPtr& args)
{
    if (!args.assigned())
        return;

    if (this->coreEventMuted)
        return;

    ProcedurePtr coreEventTrigger;
    checkErrorInfo(this->getCoreEventTrigger(&coreEventTrigger));

    if (!coreEventTrigger.assigned())
        return;

    args.getParameters().set("Path", this->getPath());
    coreEventTrigger(args);
}

END_NAMESPACE_OPENDAQ
