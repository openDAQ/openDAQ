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
#include <opendaq/device_info_impl.h>

namespace daq::config_protocol
{

template <class Impl>
class ConfigClientBaseDeviceInfoImpl;

using ConfigClientDeviceInfoImpl = ConfigClientBaseDeviceInfoImpl<DeviceInfoConfigImpl<IDeviceInfoConfig, IConfigClientObject, IDeserializeComponent>>;

template <class Impl>
class ConfigClientBaseDeviceInfoImpl : public ConfigClientPropertyObjectBaseImpl<Impl>
{
public:
    using Super = ConfigClientPropertyObjectBaseImpl<Impl>;
    using Super::Super;

    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;

    ErrCode INTERFACE_FUNC deserializeValues(ISerializedObject* serializedObject,
                                             IBaseObject* context,
                                             IFunction* callbackFactory) override;
    ErrCode INTERFACE_FUNC complete() override;
    ErrCode INTERFACE_FUNC getDeserializedParameter(IString* parameter, IBaseObject** value) override;


    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:

    template <class Interface, class Implementation>
    static BaseObjectPtr DeserializeDeviceInfo(const SerializedObjectPtr& serialized,
                                               const BaseObjectPtr& context,
                                               const FunctionPtr& factoryCallback);
};

template <class Impl>
ErrCode ConfigClientBaseDeviceInfoImpl<Impl>::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    if (remoteUpdating)
        return Impl::setPropertyValue(propertyName, value);
    return Super::setPropertyValue(propertyName, value);
}

template <class Impl>
ErrCode ConfigClientBaseDeviceInfoImpl<Impl>::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    if (remoteUpdating)
        return Impl::setProtectedPropertyValue(propertyName, value);
    return Super::setProtectedPropertyValue(propertyName, value);
}

template <class Impl>
ErrCode ConfigClientBaseDeviceInfoImpl<Impl>::deserializeValues(ISerializedObject* serializedObject,
                                                                 IBaseObject* context,
                                                                 IFunction* callbackFactory)
{
    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode ConfigClientBaseDeviceInfoImpl<Impl>::complete()
{
    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode ConfigClientBaseDeviceInfoImpl<Impl>::getDeserializedParameter(IString* parameter, IBaseObject** value)
{
    return OPENDAQ_NOTFOUND;
}

template <class Impl>
ErrCode ConfigClientBaseDeviceInfoImpl<Impl>::Deserialize(ISerializedObject* serialized,
                                                          IBaseObject* context,
                                                          IFunction* factoryCallback,
                                                          IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        *obj = DeserializeDeviceInfo<IDeviceInfoConfig, ConfigClientDeviceInfoImpl>(serialized, context, factoryCallback).detach();
    });
}

template <class Impl>
template <class Interface, class Implementation>
BaseObjectPtr ConfigClientBaseDeviceInfoImpl<Impl>::DeserializeDeviceInfo(const SerializedObjectPtr& serialized,
                                                                             const BaseObjectPtr& context,
                                                                             const FunctionPtr& factoryCallback)
{
    return Super::DeserializePropertyObject(
        serialized,
        context,
        factoryCallback,
        [](const SerializedObjectPtr& serialized, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className) -> PropertyObjectPtr
        {
            const auto ctx = deserializeContext.asPtr<IConfigProtocolDeserializeContext>();
            return createWithImplementation<Interface, Implementation>(ctx->getClientComm(),
                                                                       ctx->getRemoteGlobalId());
        });
}

}
