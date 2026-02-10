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

class ConfigClientDeviceInfoImpl : public ConfigClientPropertyObjectBaseImpl<DeviceInfoConfigImpl<IDeviceInfoConfig, IConfigClientObject, IDeserializeComponent>>
{
public:
    using Super = ConfigClientPropertyObjectBaseImpl;
    using Super::Super;

    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;

    ErrCode INTERFACE_FUNC deserializeValues(ISerializedObject* serializedObject,
                                             IBaseObject* context,
                                             IFunction* callbackFactory) override;
    ErrCode INTERFACE_FUNC getDeserializedParameter(IString* parameter, IBaseObject** value) override;


    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:

    ErrCode setValueInternal(IString* propertyName, IBaseObject* value) override;
};

// TODO: Move to inline definitions to cpp
inline ErrCode ConfigClientDeviceInfoImpl::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    if (this->remoteUpdating)
        return Super::setPropertyValue(propertyName, value);

    const ErrCode errCode = Super::setPropertyValue(propertyName, value);
    if (errCode == OPENDAQ_ERR_NOTFOUND)
    {
        daqClearErrorInfo();
        return Super::setPropertyValue(propertyName, value);
    }
    return errCode;
}

inline ErrCode ConfigClientDeviceInfoImpl::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);
    if (this->remoteUpdating)
        return Super::setProtectedPropertyValue(propertyName, value);

    PropertyPtr property;
    ErrCode errCode = Super::getProperty(propertyName, &property);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    
    if (property.getReadOnly())
        return Super::setProtectedPropertyValue(propertyName, value);

    return Super::setProtectedPropertyValue(propertyName, value);
}

inline ErrCode ConfigClientDeviceInfoImpl::setValueInternal(IString* propertyName, IBaseObject* value)
{
    return this->setPropertyValue(propertyName, value);
}

inline ErrCode ConfigClientDeviceInfoImpl::addProperty(IProperty* property)
{
    return Super::addProperty(property);
}

inline ErrCode ConfigClientDeviceInfoImpl::deserializeValues(ISerializedObject* serializedObject,
                                                                 IBaseObject* context,
                                                                 IFunction* callbackFactory)
{
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientDeviceInfoImpl::getDeserializedParameter(IString* parameter, IBaseObject** value)
{
    return OPENDAQ_NOTFOUND;
}

inline ErrCode ConfigClientDeviceInfoImpl::Deserialize(ISerializedObject* serialized,
                                                          IBaseObject* context,
                                                          IFunction* factoryCallback,
                                                          IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    const ErrCode errCode = daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        ComponentDeserializeContextPtr deserializeContextPtr = ComponentDeserializeContextPtr::Borrow(context);
        const auto ctx = deserializeContextPtr.asPtr<IConfigProtocolDeserializeContext>();
        PropertyObjectPtr propObj = createWithImplementation<IDeviceInfoConfig, ConfigClientDeviceInfoImpl>(ctx->getClientComm(), ctx->getRemoteGlobalId());

        GenericPropertyObjectImpl::DeserializePropertyOrder(serialized, context, nullptr, propObj);

        GenericPropertyObjectImpl::DeserializeLocalProperties(serialized, context, factoryCallback, propObj);

        // Do not create client objects for nested property objects (eg. active client connection info)
        GenericPropertyObjectImpl::DeserializePropertyValues(serialized, context, nullptr, propObj);
        
        const auto deserializeComponent = propObj.asPtr<IDeserializeComponent>(true);
        deserializeComponent.complete();

        *obj = propObj.detach();
    });

    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}
}
