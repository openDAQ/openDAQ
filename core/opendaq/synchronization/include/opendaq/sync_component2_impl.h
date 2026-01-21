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
#include <coretypes/dictobject.h>
#include <coretypes/dictobject_factory.h>
#include <opendaq/component_impl.h>
#include <opendaq/sync_component2.h>
#include <opendaq/sync_component2_internal.h>
#include <opendaq/sync_interface_ptr.h>
#include <opendaq/sync_interface_internal_ptr.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/interface_clock_sync_impl.h>
#include <coretypes/objectptr.h>
#include <coretypes/deserializer.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <fmt/format.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IComponent, typename... Interfaces>
class SyncComponent2Impl;

using SyncComponent2Base = SyncComponent2Impl<>;

template <class Intf, class... Intfs>
class SyncComponent2Impl : public ComponentImpl<Intf, ISyncComponent2, ISyncComponent2Internal, Intfs...>
{
public:
    using Super = ComponentImpl<Intf, ISyncComponent2, ISyncComponent2Internal, Intfs...>;

    SyncComponent2Impl(const ContextPtr& context,
                      const ComponentPtr& parent,
                      const StringPtr& localId,
                      const StringPtr& className = nullptr,
                      const StringPtr& name = nullptr);

    // ISyncComponent2
    ErrCode INTERFACE_FUNC getSelectedSource(ISyncInterface** selectedSource) override;
    ErrCode INTERFACE_FUNC setSelectedSource(IString* selectedSourceName) override;
    ErrCode INTERFACE_FUNC getSourceSynced(Bool* synced) override;
    ErrCode INTERFACE_FUNC getSourceReferenceDomainId(IString** referenceDomainId) override;
    ErrCode INTERFACE_FUNC getInterfaces(IDict** interfaces) override;

    // ISyncComponent2Internal
    ErrCode INTERFACE_FUNC addInterface(ISyncInterface* syncInterface) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    ErrCode INTERFACE_FUNC getInterfaceIds(SizeT* idCount, IntfID** ids) override;

protected:
    void init();
};


template <class Intf, class... Intfs>
SyncComponent2Impl<Intf, Intfs...>::SyncComponent2Impl(const ContextPtr& context,
                                                        const ComponentPtr& parent,
                                                        const StringPtr& localId,
                                                        const StringPtr& className,
                                                        const StringPtr& name)
    : Super(context, parent, localId, className, name)
{
    this->init();
}

template <class Intf, class... Intfs>
void SyncComponent2Impl<Intf, Intfs...>::init()
{
    auto selectedSource = createWithImplementation<ISyncInterface, InterfaceClockSyncImpl>();
    selectedSource.asPtr<ISyncInterfaceInternal>(true).setAsSource(true);

    auto interfaces = PropertyObject();
    interfaces.addProperty(ObjectProperty(selectedSource.getName(), selectedSource));
    Super::addProperty(ObjectProperty("Interfaces", interfaces));

    Super::addProperty(SelectionProperty("Source", EvalValue("%Interfaces:PropertyNames"), 0));
    this->objPtr.getOnPropertyValueWrite("Source") += [this](const PropertyObjectPtr& objPtr, const PropertyValueEventArgsPtr& eventArgs)
    {
        StringPtr sourceName = objPtr.getPropertySelectionValue("Source");
        checkErrorInfo(this->setSelectedSource(sourceName));
    };
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::getSelectedSource(ISyncInterface** selectedSource)
{
    OPENDAQ_PARAM_NOT_NULL(selectedSource);
    return daqTry([&]
    {
        StringPtr sourceName = this->objPtr.getPropertySelectionValue("Source");
        *selectedSource = this->objPtr.getPropertyValue(sourceName).template as<ISyncInterface>();
        return OPENDAQ_SUCCESS;
    });
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::setSelectedSource(IString* selectedSourceName)
{
    OPENDAQ_PARAM_NOT_NULL(selectedSourceName);
    
    return daqTry([&]
    {
        const StringPtr sourceName = this->objPtr.getPropertySelectionValue("Source");   
        const StringPtr candidateSourceName = StringPtr::Borrow(selectedSourceName);

        if (sourceName == candidateSourceName)
            return OPENDAQ_SUCCESS;

        const PropertyObjectPtr interfaces = this->objPtr.getPropertyValue("Interfaces");

        if (!interfaces.hasProperty(sourceName))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format("Interface '{}' not found in interfaces", sourceName));

        
        SyncInterfacePtr currentSource = this->objPtr.getPropertyValue(sourceName);
        if (auto sourceInternal = currentSource.asPtrOrNull<ISyncInterfaceInternal>(true); sourceInternal.assigned())
            sourceInternal.setAsSource(false);

        SyncInterfacePtr newSource = interfaces.getPropertyValue(candidateSourceName);
        if (auto sourceInternal = newSource.asPtrOrNull<ISyncInterfaceInternal>(true); sourceInternal.assigned())
            sourceInternal.setAsSource(true);

        const auto sourceProperty = this->objPtr.getProperty("Source");
        for (const auto& [index, name] : sourceProperty.getSelectionValues().template asPtr<IDict, DictPtr<IInteger, IString>>(true))
        {
            if (name == sourceName)
            {
                OPENDAQ_RETURN_IF_FAILED(this->setPropertyValue(String("Source"), index));
                return OPENDAQ_SUCCESS;
            }
        }

        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format("Index for interface {} not found", sourceName));
    });
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::getSourceSynced(Bool* synced)
{
    OPENDAQ_PARAM_NOT_NULL(synced);

    SyncInterfacePtr source;
    OPENDAQ_RETURN_IF_FAILED(this->getSelectedSource(&source));
    OPENDAQ_RETURN_IF_FAILED(source->getSynced(synced));
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::getSourceReferenceDomainId(IString** referenceDomainId)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainId);

    SyncInterfacePtr source;
    OPENDAQ_RETURN_IF_FAILED(this->getSelectedSource(&source));
    OPENDAQ_RETURN_IF_FAILED(source->getReferenceDomainId(referenceDomainId));
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::getInterfaces(IDict** interfaces)
{
    OPENDAQ_PARAM_NOT_NULL(interfaces);
    return daqTry([&]
    {
        auto interfacesDict = Dict<IString, ISyncInterface>();
        const PropertyObjectPtr interfacesProperty = this->objPtr.getPropertyValue("Interfaces");

        for (const auto& prop : interfacesProperty.getAllProperties())
            interfacesDict.set(prop.getName(), prop.getValue());

        *interfaces = interfacesDict.detach();
        return OPENDAQ_SUCCESS;
    });
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::addInterface(ISyncInterface* syncInterface)
{
    OPENDAQ_PARAM_NOT_NULL(syncInterface);
    
    return daqTry([&]
    {
        const SyncInterfacePtr interfacePtr = SyncInterfacePtr::Borrow(syncInterface);
        const PropertyObjectPtr interfacesProperty = this->objPtr.getPropertyValue("Interfaces");
        interfacesProperty.addProperty(ObjectProperty(interfacePtr.getName(), interfacePtr));
    });
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ConstCharPtr SyncComponent2Impl<Intf, Intfs...>::SerializeId()
{
    return "SyncComponent2";
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::Deserialize(ISerializedObject* serialized,
                                                         IBaseObject* context,
                                                         IFunction* factoryCallback,
                                                         IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);
    const ErrCode errCode = daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        *obj = Super::DeserializeComponent(
            serialized,
            context,
            factoryCallback,
            [](const SerializedObjectPtr&, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className)
            {
                return createWithImplementation<ISyncComponent2, SyncComponent2Impl<Intf, Intfs...>>(
                    deserializeContext.getContext(),
                    deserializeContext.getParent(),
                    deserializeContext.getLocalId(),
                    className).detach();
            }).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::getInterfaceIds(SizeT* idCount, IntfID** ids)
{
    OPENDAQ_PARAM_NOT_NULL(idCount);

    using InterfaceIdsType = typename Super::InterfaceIds;
    *idCount = InterfaceIdsType::Count() + 1;
    if (ids == nullptr)
    {
        return OPENDAQ_SUCCESS;
    }

    **ids = IPropertyObject::Id;
    (*ids)++;

    InterfaceIdsType::AddInterfaceIds(*ids);
    return OPENDAQ_SUCCESS;
}


OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SyncComponent2Base)

END_NAMESPACE_OPENDAQ
