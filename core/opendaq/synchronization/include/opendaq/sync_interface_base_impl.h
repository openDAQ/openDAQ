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
#include <array>

BEGIN_NAMESPACE_OPENDAQ

inline constexpr std::array<const char*, 4> SyncModeNames{"Off", "Input", "Output", "Auto"};

inline SyncMode syncModeFromName(const StringPtr& name)
{
    for (size_t i = 0; i < SyncModeNames.size(); ++i)
        if (name == SyncModeNames[i])
            return static_cast<SyncMode>(i);
    DAQ_THROW_EXCEPTION(NotFoundException, "Unknown sync mode");
}

template <typename TInterface = IPropertyObject, typename... Interfaces>
class GenericSyncInterfaceImpl : public GenericPropertyObjectImpl<TInterface, ISyncInterface, Interfaces...>
{
public:
    using Super = GenericPropertyObjectImpl<TInterface, ISyncInterface, Interfaces...>;

    GenericSyncInterfaceImpl(const TypeManagerPtr& manager);

    // ISyncInterface
    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) override;
    ErrCode INTERFACE_FUNC setMode(SyncMode mode) override;
    ErrCode INTERFACE_FUNC getMode(SyncMode* sourceMode) override;
    ErrCode INTERFACE_FUNC getAvailableModes(IDict** availableModes) override;
    ErrCode INTERFACE_FUNC getSourceSupported(Bool* canBeSource) override;
    ErrCode INTERFACE_FUNC getConfig(IPropertyObject** config) override;
    ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** syncStatus) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

private:
    void triggerCoreEvent(const CoreEventArgsPtr& args);

protected:
    // Written by the server object and by the client mirror, so a gateway passes them on
    ErrCode serializeCustomValues(ISerializer* serializer, bool forUpdate) override;

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
    ErrCode INTERFACE_FUNC getSyncType(IString** syncType) override = 0;

    // ISyncInterfaceInternal
    ErrCode INTERFACE_FUNC setAsSource(Bool source) override;
    ErrCode INTERFACE_FUNC sourceChanged(ISyncInterface* source) override;

    // IPropertyObjectInternal
    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

protected:
    explicit SyncInterfaceBaseImpl(const TypeManagerPtr& manager,
                                   const StringPtr& id,
                                   const std::vector<SyncMode>& availableModes,
                                   const PropertyObjectPtr& config = PropertyObject());

    virtual void onModeChanged(SyncMode mode);

    void setReferenceDomainId(const StringPtr& referenceDomainId);
    void setSyncSourceStatus(SyncSourceStatus status, const StringPtr& message = "");
    void setSyncRoleStatus(SyncRoleStatus status, const StringPtr& message = "");

    const PropertyObjectPtr config;
    TypeManagerPtr manager;

private:

    void initAvailiableModes(const std::vector<SyncMode>& availableModes);
    void initProperties(const StringPtr& id);
    void initSynchronizationStatus();

    ListPtr<IString> sourceModes;
    ListPtr<IString> outputModes;
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
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);
    return daqTry([&]
    {
        *id = this->objPtr.getPropertyValue("Id").template as<IString>();
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::setMode(SyncMode mode)
{
    const auto index = static_cast<size_t>(mode);
    if (index >= SyncModeNames.size())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, "Unknown sync mode");
    const ErrCode errCode = this->setPropertyValue(String("Mode"), String(SyncModeNames[index]));
    OPENDAQ_RETURN_IF_FAILED(errCode, "Failed to set sync interface mode");
    return errCode;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getMode(SyncMode* sourceMode)
{
    OPENDAQ_PARAM_NOT_NULL(sourceMode);
    return daqTry([&]
    {
        *sourceMode = syncModeFromName(this->objPtr.getPropertyValue("Mode"));
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getAvailableModes(IDict** availableModes)
{
    OPENDAQ_PARAM_NOT_NULL(availableModes);
    return daqTry([&]
    {
        auto modes = Dict<IInteger, IString>();
        for (const StringPtr& name : ListPtr<IString>(this->objPtr.getPropertyValue("ModeSelection")))
            modes.set(static_cast<Int>(syncModeFromName(name)), name);
        *availableModes = modes.detach();
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getSourceSupported(Bool* canBeSource)
{
    OPENDAQ_PARAM_NOT_NULL(canBeSource);

    BaseObjectPtr objPtr;
    OPENDAQ_RETURN_IF_FAILED(this->getPropertyValue(String("SourceSupported"), &objPtr));

    Bool value = False;
    const BooleanPtr canBeSourceValue = objPtr.asPtrOrNull<IBoolean>();
    if (canBeSourceValue.assigned())
        OPENDAQ_RETURN_IF_FAILED(canBeSourceValue->getValue(&value));

    *canBeSource = value;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getReferenceDomainId(IString** referenceDomainId)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainId);
    BaseObjectPtr objPtr;
    OPENDAQ_RETURN_IF_FAILED(this->getPropertyValue(String("ReferenceDomainId"), &objPtr));
    *referenceDomainId = objPtr.asOrNull<IString>();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::getConfig(IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);
    BaseObjectPtr objPtr;
    OPENDAQ_RETURN_IF_FAILED(this->getPropertyValue(String("Config"), &objPtr));
    *config = objPtr.asOrNull<IPropertyObject>();
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
ErrCode GenericSyncInterfaceImpl<TInterface, Interfaces...>::serializeCustomValues(ISerializer* serializer, bool forUpdate)
{
    OPENDAQ_RETURN_IF_FAILED(Super::serializeCustomValues(serializer, forUpdate));
    if (forUpdate)
        return OPENDAQ_SUCCESS;

    serializer->key("SyncStatus");
    OPENDAQ_RETURN_IF_FAILED(statusContainer.template asPtr<ISerializable>(true)->serialize(serializer));

    StringPtr syncType;
    OPENDAQ_RETURN_IF_FAILED(this->getSyncType(&syncType));
    serializer->key("SyncType");
    serializer->writeString(syncType.getCharPtr(), syncType.getLength());
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
