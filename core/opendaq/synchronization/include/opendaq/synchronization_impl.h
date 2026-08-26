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
class GenericSynchronizationImpl : public GenericPropertyObjectImpl<TInterface, ISynchronization, Interfaces...>
{
public:
    using Super = GenericPropertyObjectImpl<TInterface, ISynchronization, Interfaces...>;

    explicit GenericSynchronizationImpl(const TypeManagerPtr& manager);

    // ISynchronization
    ErrCode INTERFACE_FUNC getSyncInterfaces(IDict** interfaces) override;
    ErrCode INTERFACE_FUNC setSource(IString* sourceName) override;
    ErrCode INTERFACE_FUNC getSource(ISyncInterface** source) override;
    ErrCode INTERFACE_FUNC getReferenceDomainIds(IList** ids) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
};

class SynchronizationImpl : public GenericSynchronizationImpl<IPropertyObject, ISynchronizationInternal>
{
public:
    using Super = GenericSynchronizationImpl<IPropertyObject, ISynchronizationInternal>;

    explicit SynchronizationImpl(const TypeManagerPtr& manager);

    // ISynchronization
    ErrCode INTERFACE_FUNC getSource(ISyncInterface** source) override;

    // ISynchronizationInternal
    ErrCode INTERFACE_FUNC addInterface(ISyncInterface* syncInterface) override;

    // IPropertyObjectInternal
    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

protected:
    SyncInterfacePtr source;

private:
    void onSourceChanged(const StringPtr& sourceName);
    void onSourceClockTypeChanged(PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args);
    void notifySourceClockTypeChanged(const StringPtr& clockType);
    void subscribeSourceClockType(const SyncInterfacePtr& syncInterface);
    void unsubscribeSourceClockType(const SyncInterfacePtr& syncInterface);
};

template <typename TInterface, typename... Interfaces>
GenericSynchronizationImpl<TInterface, Interfaces...>::GenericSynchronizationImpl(const TypeManagerPtr& manager)
    : Super(manager, "")
{
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSynchronizationImpl<TInterface, Interfaces...>::setSource(IString* sourceName)
{
    const ErrCode errCode = this->setPropertyValue(String("Source"), sourceName);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSynchronizationImpl<TInterface, Interfaces...>::getSource(ISyncInterface** selectedSource)
{
    OPENDAQ_PARAM_NOT_NULL(selectedSource);
   
    return daqTry([&]
    {
        auto lock = this->getRecursiveConfigLock2();
        StringPtr sourceName = this->objPtr.getPropertyValue("Source");
        PropertyObjectPtr interfaces = this->objPtr.getPropertyValue("Interfaces");
        *selectedSource = interfaces.getPropertyValue(sourceName).template as<ISyncInterface>();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSynchronizationImpl<TInterface, Interfaces...>::getReferenceDomainIds(IList** ids)
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

template <typename TInterface, typename... Interfaces>
ErrCode GenericSynchronizationImpl<TInterface, Interfaces...>::getSyncInterfaces(IDict** interfaces)
{
    OPENDAQ_PARAM_NOT_NULL(interfaces);
    return daqTry([&]
    {
        auto interfacesDict = Dict<IString, ISyncInterface>();

        auto lock = this->getRecursiveConfigLock2();
        const PropertyObjectPtr interfacesProperty = this->objPtr.getPropertyValue("Interfaces");

        for (const auto& prop : interfacesProperty.getAllProperties())
            interfacesDict.set(prop.getName(), prop.getValue());

        *interfaces = interfacesDict.detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericSynchronizationImpl<TInterface, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);
    *id = "Synchronization";
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
