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
#include <opendaq/clock_sync_interface_impl.h>
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
    using Self = SyncComponent2Impl<Intf, Intfs...>;

    SyncComponent2Impl(const ContextPtr& context,
                      const ComponentPtr& parent,
                      const StringPtr& localId,
                      Bool registerEvents = False);

    // ISyncComponent2
    ErrCode INTERFACE_FUNC getSelectedSource(ISyncInterface** selectedSource) override;
    ErrCode INTERFACE_FUNC getSourceSynced(Bool* synced) override;
    ErrCode INTERFACE_FUNC getSourceReferenceDomainId(IString** referenceDomainId) override;
    ErrCode INTERFACE_FUNC getInterfaces(IDict** interfaces) override;

    // ISyncComponent2Internal
    ErrCode INTERFACE_FUNC addInterface(ISyncInterface* syncInterface) override;


    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void init(bool registerEvents);
    void setSelectedSource(const SyncInterfacePtr& newSource);
    void onInterfaceModeChanged(const PropertyObjectPtr& objPtr, const PropertyValueEventArgsPtr& eventArgs);

    SyncInterfacePtr source;
};


template <class Intf, class... Intfs>
SyncComponent2Impl<Intf, Intfs...>::SyncComponent2Impl(const ContextPtr& context,
                                                        const ComponentPtr& parent,
                                                        const StringPtr& localId,
                                                        Bool registerEvents)
    : Super(context, parent, localId, nullptr, nullptr)
{
    this->init(registerEvents == True);
}

template <class Intf, class... Intfs>
void SyncComponent2Impl<Intf, Intfs...>::init(bool registerEvents)
{
    source = createWithImplementation<ISyncInterface, ClockSyncInterfaceImpl>();
    source.asPtr<IPropertyObject>(true).setPropertyValue("Mode", "Input");

    auto interfaces = PropertyObject();
    interfaces.addProperty(ObjectProperty(source.getName(), source));
    Super::addProperty(ObjectProperty("Interfaces", interfaces));

    const auto souceProperty = StringPropertyBuilder("Source", source.getName())
                                                        .setSelectionValues(EvalValue("%Interfaces:PropertyNames"))
                                                        .setReadOnly(true)
                                                        .build();
    Super::addProperty(souceProperty);
    this->objPtr.setPropertyOrder(List<IString>("Interfaces"));

    if (registerEvents)
    {
        source.asPtr<IPropertyObject>(true).getOnPropertyValueWrite("Mode") += [this](const PropertyObjectPtr& objPtr, const PropertyValueEventArgsPtr& eventArgs)
        {
            if (source == objPtr)
            {
                const StringPtr mode = eventArgs.getValue();
                if (mode == "Off")
                    eventArgs.setValue("Input");
            }
            else
            {
                const StringPtr mode = eventArgs.getValue();
                if (mode == "Input")
                {
                    const auto oldSource = source;
                    setSelectedSource(objPtr);
                    oldSource.template asPtr<ISyncInterfaceInternal>(true).deactivateAsSource();
                }
            }
        };
    }
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::getSelectedSource(ISyncInterface** selectedSource)
{
    OPENDAQ_PARAM_NOT_NULL(selectedSource);
    auto lock = this->getRecursiveConfigLock();
    *selectedSource = this->source.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
void SyncComponent2Impl<Intf, Intfs...>::setSelectedSource(const SyncInterfacePtr& newSource)
{
    this->template borrowPtr<PropertyObjectProtectedPtr>().setProtectedPropertyValue("Source", newSource.getName());
    source = newSource;
}

template <class Intf, class... Intfs>
void SyncComponent2Impl<Intf, Intfs...>::onInterfaceModeChanged(const PropertyObjectPtr& objPtr, const PropertyValueEventArgsPtr& eventArgs)
{
    if (source == objPtr)
    {
        const StringPtr mode = eventArgs.getValue();
        if (mode == "Output" || mode == "Off")
        {
            const PropertyObjectPtr interfacesProperty = this->objPtr.getPropertyValue("Interfaces");
            const PropertyObjectPtr clockSyncInterfaceProperty = interfacesProperty.getPropertyValue("ClockSyncInterface");
            clockSyncInterfaceProperty.setPropertyValue("Mode", "Input");
        }
    }
    else
    {
        const StringPtr mode = eventArgs.getValue();
        if (mode == "Input" || mode == "Auto")
        {
            const auto oldSource = source;
            setSelectedSource(objPtr);
            oldSource.template asPtr<ISyncInterfaceInternal>(true).deactivateAsSource();
        }
    }
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::getSourceSynced(Bool* synced)
{
    OPENDAQ_PARAM_NOT_NULL(synced);
    auto lock = this->getRecursiveConfigLock();

    SyncInterfacePtr source;
    OPENDAQ_RETURN_IF_FAILED(this->getSelectedSource(&source));
    OPENDAQ_RETURN_IF_FAILED(source->getSynced(synced));
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode SyncComponent2Impl<Intf, Intfs...>::getSourceReferenceDomainId(IString** referenceDomainId)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainId);
    auto lock = this->getRecursiveConfigLock();

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
        const PropertyObjectPtr interfaceProperty = interfacePtr;

        const PropertyObjectPtr interfacesProperty = this->objPtr.getPropertyValue("Interfaces");

        interfacesProperty.addProperty(ObjectProperty(interfacePtr.getName(), interfaceProperty));
        interfaceProperty.getOnPropertyValueWrite("Mode") += [this](const PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args)
        {
            onInterfaceModeChanged(obj, args);
        };
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
                    false).detach();
            }).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SyncComponent2Base)

END_NAMESPACE_OPENDAQ
