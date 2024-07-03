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
#include <coretypes/type_manager_ptr.h>
#include <opendaq/component_impl.h>
#include <opendaq/sync_component.h>
#include <opendaq/sync_component_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class SyncComponentImpl : public GenericPropertyObjectImpl<ISyncComponent>
{
public:
    using Super = GenericPropertyObjectImpl<ISyncComponent>;

    explicit SyncComponentImpl(const TypeManagerPtr& typeManager);

    //ISyncComponent
    ErrCode INTERFACE_FUNC getSyncLocked(Bool* synchronizationLocked) override;
    ErrCode INTERFACE_FUNC setSyncLocked(Bool synchronizationLocked) override;

    ErrCode INTERFACE_FUNC getSelectedSource(Int* selectedSource) override;
    ErrCode INTERFACE_FUNC setSelectedSource(Int selectedSource) override;

    ErrCode INTERFACE_FUNC getInterfaces(IList** interfaces) override;
    ErrCode INTERFACE_FUNC getInterfaceNames(IList** interfaceNames) override;
    ErrCode INTERFACE_FUNC addInterface(IPropertyObject* interface) override;
    ErrCode INTERFACE_FUNC removeInterface(IString* interfaceName) override;

    ErrCode INTERFACE_FUNC getInterfaceIds(SizeT* idCount, IntfID** ids) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    PropertyObjectPtr createCloneBase() override;

private:
    template <typename T>
    typename InterfaceToSmartPtr<T>::SmartPtr getTypedProperty(const StringPtr& name);

    TypeManagerPtr typeManager;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SyncComponentImpl)

END_NAMESPACE_OPENDAQ