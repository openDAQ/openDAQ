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
#include <coreobjects/property_object_impl.h>
#include <config_protocol/config_client_object_impl.h>

namespace daq::config_protocol
{

template <typename Impl>
class ConfigClientPropertyObjectBaseImpl;

using ConfigClientPropertyObjectImpl = ConfigClientPropertyObjectBaseImpl<PropertyObjectImpl>;

template <class Impl>
class ConfigClientPropertyObjectBaseImpl : public ConfigClientObjectImpl, public Impl
{
public:
    template <class ... Args>
    explicit ConfigClientPropertyObjectBaseImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm, const SerializedObjectPtr& serializedObject, const Args& ... args);

    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC getPropertyValue(IString* propertyName, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC getPropertySelectionValue(IString* propertyName, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC clearPropertyValue(IString* propertyName) override;
    ErrCode INTERFACE_FUNC getProperty(IString* propertyName, IProperty** value) override;
    ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;
    ErrCode INTERFACE_FUNC removeProperty(IString* propertyName) override;
    ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IString* propertyName, IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnPropertyValueRead(IString* propertyName, IEvent** event) override;
    ErrCode INTERFACE_FUNC getVisibleProperties(IList** properties) override;
    ErrCode INTERFACE_FUNC hasProperty(IString* propertyName, Bool* hasProperty) override;
    ErrCode INTERFACE_FUNC getAllProperties(IList** properties) override;
    ErrCode INTERFACE_FUNC setPropertyOrder(IList* orderedPropertyNames) override;

private:
    void buildPropertyObject(const SerializedObjectPtr& serializedObject);
};

template <class Impl>
template <class ... Args>
ConfigClientPropertyObjectBaseImpl<Impl>::ConfigClientPropertyObjectBaseImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                                             const SerializedObjectPtr& serializedObject,
                                                                             const Args& ... args)
    : ConfigClientObjectImpl(configProtocolClientComm)
    , Impl(args ...)
{
    buildPropertyObject(serializedObject);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getPropertyValue(IString* propertyName, IBaseObject** value)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getPropertySelectionValue(IString* propertyName, IBaseObject** value)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::clearPropertyValue(IString* propertyName)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getProperty(IString* propertyName, IProperty** value)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::addProperty(IProperty* property)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::removeProperty(IString* propertyName)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getOnPropertyValueWrite(IString* propertyName, IEvent** event)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getOnPropertyValueRead(IString* propertyName, IEvent** event)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getVisibleProperties(IList** properties)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::hasProperty(IString* propertyName, Bool* hasProperty)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getAllProperties(IList** properties)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setPropertyOrder(IList* orderedPropertyNames)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::buildPropertyObject(const SerializedObjectPtr& serializedObject)
{
    // TODO
}

}
