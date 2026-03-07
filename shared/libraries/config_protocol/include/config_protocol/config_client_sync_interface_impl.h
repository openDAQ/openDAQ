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
#include <config_protocol/config_client_property_object_impl.h>
#include <opendaq/sync_interface_base_impl.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>
#include <opendaq/component_deserialize_context_ptr.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/function_ptr.h>
#include <opendaq/deserialize_component.h>

namespace daq::config_protocol
{

class ConfigClientSyncInterfaceImpl : public ConfigClientPropertyObjectBaseImpl<SyncInterfaceBaseImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>>
{
public:
    using Impl = SyncInterfaceBaseImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>;
    using Super = ConfigClientPropertyObjectBaseImpl<Impl>;

    ConfigClientSyncInterfaceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                  const std::string& remoteGlobalId);

    // IPropertyObject
    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC clearPropertyValue(IString* propertyName) override;
    ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;
    ErrCode INTERFACE_FUNC removeProperty(IString* propertyName) override;

    ErrCode INTERFACE_FUNC beginUpdate() override;
    ErrCode INTERFACE_FUNC endUpdate() override;

    // ISyncInterfaceInternal
    ErrCode INTERFACE_FUNC setAsSource(Bool isSource) override;

    // IDeserializeComponent
    ErrCode INTERFACE_FUNC deserializeValues(ISerializedObject* serializedObject, IBaseObject* context, IFunction* callbackFactory) override;
    ErrCode INTERFACE_FUNC complete() override;
    ErrCode INTERFACE_FUNC getDeserializedParameter(IString* parameter, IBaseObject** value) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

} // namespace daq::config_protocol
