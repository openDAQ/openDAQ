/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/context_ptr.h>
#include <opendaq/component_impl.h>
#include <opendaq/sync_component.h>
#include <opendaq/sync_component_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

const char* Interfaces = "interfaces";
const char* SyncronizationLocked = "SyncronizationLocked";
const char* Source = "Source";

class SyncComponentImpl : public GenericPropertyObjectImpl<ISyncComponent>
{
public:
    using Super = GenericPropertyObjectImpl<ISyncComponent>;

    explicit SyncComponentImpl();

    //ISyncComponent
    ErrCode INTERFACE_FUNC test() override;
    ErrCode INTERFACE_FUNC getSyncLocked(Bool* SyncronizationLocked) override;
    ErrCode INTERFACE_FUNC getSelectedSource(IString** selectedSource) override;
    //ErrCode INTERFACE_FUNC setSyncLocked(Bool syncronizationLocked) override;
    //ErrCode INTERFACE_FUNC setSelectedSource(IString* selectedSource) override;
    ErrCode INTERFACE_FUNC getInterfaces(IList** interfaces) override;
    ErrCode INTERFACE_FUNC addInterface(IPropertyObject* interface) override;
    ErrCode INTERFACE_FUNC removeInterface(IString* interfaceName) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    ListPtr<IPropertyObject> interfaces;

private:
    template <typename T>
    typename InterfaceToSmartPtr<T>::SmartPtr getTypedProperty(const StringPtr& name);
};

SyncComponentImpl::SyncComponentImpl()
    : Super()
{
    Super::addProperty(ObjectProperty(Interfaces, PropertyObject()));
    Super::addProperty(BoolProperty(SyncronizationLocked, false));
    Super::addProperty(SelectionProperty(Source, List<IString>("Interface1", "Interface2", "Interface3"), 0));
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr SyncComponentImpl::getTypedProperty(const StringPtr& name)
{
    return objPtr.getPropertyValue(name).template asPtr<T>();
}

ErrCode SyncComponentImpl::test()
{
    return OPENDAQ_SUCCESS;
}

ErrCode SyncComponentImpl::getSyncLocked(Bool* syncLocked)
{
    return daqTry([&]() {
        *syncLocked = getTypedProperty<IBoolean>(SyncronizationLocked);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode SyncComponentImpl::getSelectedSource(IString** selectedSource)
{
    return daqTry([&]() {
        *selectedSource = getTypedProperty<IString>(Source).detach();
        return OPENDAQ_SUCCESS;
    });
}


//ErrCode INTERFACE_FUNC setSyncLocked(Bool syncronizationLocked)
//{
//    return daqTry([&]() {
//        checkErrorInfo(Super::setPropertyValue(String(SyncronizationLocked), syncronizationLocked));
//        return OPENDAQ_SUCCESS;
//    });
//}
//
//ErrCode INTERFACE_FUNC setSelectedSource(IString* selectedSource)
//{
//    OPENDAQ_PARAM_NOT_NULL(selectedSource);
//    return daqTry([&]() {
//        checkErrorInfo(Super::setPropertyValue(String(Source), selectedSource));
//        return OPENDAQ_SUCCESS;
//    });
//}

ErrCode SyncComponentImpl::getInterfaces(IList** interfaces)
{
    OPENDAQ_PARAM_NOT_NULL(interfaces);
    ListPtr<IPropertyObject> interfacesList = List<IPropertyObject>();

    BaseObjectPtr Interfaces;
    StringPtr str = "interfaces";
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

    //TBD: Check if interface inherits from SyncInterfaceBase

    BaseObjectPtr Interfaces;
    StringPtr str = "interfaces";
    ErrCode err = this->getPropertyValue(str, &Interfaces);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto InterfacesPtr = Interfaces.asPtr<IPropertyObject>();
    for (const auto& prop : InterfacesPtr.getAllProperties())
    {
        if (prop.getValueType() != ctObject)
            continue;

        auto interfaceProperty = InterfacesPtr.getPropertyValue(prop.getName());
        //check for duplicates of the interface here
    }

    InterfacesPtr.addProperty(ObjectProperty(interface));
    return OPENDAQ_SUCCESS;
}


ErrCode SyncComponentImpl::removeInterface(IString* interfaceName)
{
    OPENDAQ_PARAM_NOT_NULL(interfaceName);

    BaseObjectPtr Interfaces;
    StringPtr str = "interfaces";
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

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializePropertyObject(
                    serialized,
                    context,
                    factoryCallback,
                       [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
                       {
                           const auto sync = createWithImplementation<ISyncComponent, SyncComponentImpl>();
                           return sync;
                       }).detach();
        });
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SyncComponentImpl)

END_NAMESPACE_OPENDAQ
