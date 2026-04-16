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
#include <opendaq/update_parameters.h>
#include <opendaq/device_update_options_ptr.h>
#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class UpdateParametersImpl : public GenericPropertyObjectImpl<IUpdateParameters>
{
public:
    using Super = GenericPropertyObjectImpl<IUpdateParameters>;

    UpdateParametersImpl();

    ErrCode INTERFACE_FUNC getDeviceUpdateOptions(IDeviceUpdateOptions** options) override;
    ErrCode INTERFACE_FUNC setDeviceUpdateOptions(IDeviceUpdateOptions* options) override;
    ErrCode INTERFACE_FUNC getConfigurationLoadMode(ConfigurationLoadMode* mode) override;
    ErrCode INTERFACE_FUNC setConfigurationLoadMode(ConfigurationLoadMode mode) override;

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    template <typename T>
    typename InterfaceToSmartPtr<T>::SmartPtr getTypedProperty(const StringPtr& name);
    ErrCode serializeCustomValues(ISerializer* serializer, bool /*forUpdate*/) override;

    DeviceUpdateOptionsPtr deviceOptions;
    ConfigurationLoadMode configLoadMode;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(UpdateParametersImpl)

END_NAMESPACE_OPENDAQ
