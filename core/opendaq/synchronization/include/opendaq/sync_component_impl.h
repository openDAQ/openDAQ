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

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SyncComponentImpl)

END_NAMESPACE_OPENDAQ
