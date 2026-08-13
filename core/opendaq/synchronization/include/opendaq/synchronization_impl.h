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

#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/dictobject.h>
#include <coretypes/dictobject_factory.h>
#include <opendaq/synchronization.h>
#include <opendaq/synchronization_internal.h>
#include <opendaq/sync_interface_ptr.h>
#include <opendaq/sync_interface_internal_ptr.h>
#include <opendaq/clock_sync_interface_impl.h>
#include <coretypes/objectptr.h>
#include <coretypes/deserializer.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_impl.h>
#include <fmt/format.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IPropertyObject, typename... Interfaces>
class SynchronizationImpl;

using SynchronizationBase = SynchronizationImpl<>;

template <class Intf, class... Intfs>
class SynchronizationImpl : public GenericPropertyObjectImpl<Intf, ISynchronization, ISynchronizationInternal, Intfs...>
{
public:
    using Super = GenericPropertyObjectImpl<Intf, ISynchronization, ISynchronizationInternal, Intfs...>;
    using Self = SynchronizationImpl<Intf, Intfs...>;

    explicit SynchronizationImpl();

    // ISynchronization
    ErrCode INTERFACE_FUNC getSyncInterfaces(IDict** interfaces) override;
    ErrCode INTERFACE_FUNC setSource(IString* sourceName) override;
    ErrCode INTERFACE_FUNC getSource(ISyncInterface** source) override;
    ErrCode INTERFACE_FUNC getReferenceDomainIds(IList** ids) override;

    // ISynchronizationInternal
    ErrCode INTERFACE_FUNC addInterface(ISyncInterface* syncInterface) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    // IPropertyObjectInternal
    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    explicit SynchronizationImpl(Bool remote);
    SyncInterfacePtr source;
private:
    void onSourceChanged(const StringPtr& sourceName);
};

template <class Intf, class... Intfs>
SynchronizationImpl<Intf, Intfs...>::SynchronizationImpl(Bool remote)
    : Super()
{
    if (!remote)
    {
        source = createWithImplementation<ISyncInterface, ClockSyncInterfaceImpl>();
        source.asPtr<ISyncInterfaceInternal>(true).setAsSource(true);

        auto interfaces = PropertyObject();
        interfaces.addProperty(ObjectProperty(source.getName(), source));
        this->addProperty(ObjectProperty("Interfaces", interfaces));

        const auto souceProperty = StringPropertyBuilder("Source", source.getName())
                                                            .setSelectionValues(EvalValue("%Interfaces:PropertyNames"))
                                                            .setReadOnly(true)
                                                            .build();
        this->addProperty(souceProperty);
        this->objPtr.setPropertyOrder(List<IString>("Interfaces"));

        this->objPtr.getOnPropertyValueWrite("Source") += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr& args)
        { 
           onSourceChanged(args.getValue());
        };
    }
}

template <class Intf, class... Intfs>
SynchronizationImpl<Intf, Intfs...>::SynchronizationImpl()
    : SynchronizationImpl(false)
{
}

template <class Intf, class... Intfs>
void SynchronizationImpl<Intf, Intfs...>::onSourceChanged(const StringPtr& sourceName)
{
    auto lock = this->getRecursiveConfigLock2();
    const PropertyObjectPtr interfacesProperty = this->objPtr.getPropertyValue("Interfaces");
    SyncInterfacePtr newSource = interfacesProperty.getPropertyValue(sourceName);
    SyncInterfacePtr oldSource = source;
    const auto oldSourceMode = oldSource.getMode();

    oldSource.asPtr<ISyncInterfaceInternal>(true).setAsSource(False);

    try
    {
        newSource.asPtr<ISyncInterfaceInternal>(true).setAsSource(True);
        source = newSource.detach();
    }
    catch(...)
    {
        oldSource.asPtr<ISyncInterfaceInternal>(true).setAsSource(True);
        oldSource.asPtr<IPropertyObject>().setPropertyValue("Mode", static_cast<Int>(oldSourceMode));
        throw;
    }
}

template <class Intf, class... Intfs>
ErrCode SynchronizationImpl<Intf, Intfs...>::setSource(IString* sourceName)
{
    const ErrCode errCode = this->setProtectedPropertyValue(String("Source"), sourceName);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class Intf, class... Intfs>
ErrCode SynchronizationImpl<Intf, Intfs...>::getSource(ISyncInterface** selectedSource)
{
    OPENDAQ_PARAM_NOT_NULL(selectedSource);
    auto lock = this->getRecursiveConfigLock2();
    *selectedSource = this->source.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode SynchronizationImpl<Intf, Intfs...>::getReferenceDomainIds(IList** ids)
{
    OPENDAQ_PARAM_NOT_NULL(ids);
    auto idList = List<IString>();

    const PropertyObjectPtr interfacesProperty = this->objPtr.getPropertyValue("Interfaces");
    for (const auto & intefaceProp : interfacesProperty.getAllProperties())
    {
        SyncInterfacePtr interface = intefaceProp.getValue();
        const auto referenceDomainID = interface.getReferenceDomainId();
        if (referenceDomainID.assigned() && referenceDomainID.getLength() != 0)
            idList.pushBack(interface.getReferenceDomainId());
    }

    *ids = idList.detach();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode SynchronizationImpl<Intf, Intfs...>::getSyncInterfaces(IDict** interfaces)
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
ErrCode SynchronizationImpl<Intf, Intfs...>::addInterface(ISyncInterface* syncInterface)
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
ErrCode SynchronizationImpl<Intf, Intfs...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode SynchronizationImpl<Intf, Intfs...>::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_CLONEABLE);
}

template <class Intf, class... Intfs>
ConstCharPtr SynchronizationImpl<Intf, Intfs...>::SerializeId()
{
    return "Synchronization";
}

template <class Intf, class... Intfs>
ErrCode SynchronizationImpl<Intf, Intfs...>::Deserialize(ISerializedObject* serialized,
                                                         IBaseObject* context,
                                                         IFunction* factoryCallback,
                                                         IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);
    const ErrCode errCode = daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        *obj = Super::DeserializePropertyObject(
                   serialized,
                   context,
                   factoryCallback,
                   [](const SerializedObjectPtr&, const BaseObjectPtr&, const StringPtr&)
                   {
                       return createWithImplementation<ISynchronization, SynchronizationImpl<Intf, Intfs...>>().detach();
                   })
                   .detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SynchronizationBase)

END_NAMESPACE_OPENDAQ
