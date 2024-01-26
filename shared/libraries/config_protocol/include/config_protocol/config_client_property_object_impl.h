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
#include <config_protocol/config_client_function_impl.h>
#include <config_protocol/config_client_procedure_impl.h>
#include <config_protocol/config_client_object.h>
#include <opendaq/deserialize_component_ptr.h>
#include <config_protocol/config_protocol_deserialize_context.h>

namespace daq::config_protocol
{

template <typename Impl>
class ConfigClientPropertyObjectBaseImpl;

class ConfigClientStandalonePropertyObjectImpl;

using ConfigClientPropertyObjectImpl = ConfigClientPropertyObjectBaseImpl<ConfigClientStandalonePropertyObjectImpl>;

template <class Impl>
class ConfigClientPropertyObjectBaseImpl : public ConfigClientObjectImpl, public Impl
{
public:
    template <class ... Args>
    explicit ConfigClientPropertyObjectBaseImpl(
        const ConfigProtocolClientCommPtr& configProtocolClientComm,
        const std::string& remoteGlobalId,
        const Args& ... args);

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

    ErrCode INTERFACE_FUNC complete() override;

    ErrCode INTERFACE_FUNC getRemoteGlobalId(IString** remoteGlobalId) override;

private:
    bool deserializationComplete;

    BaseObjectPtr getValueFromServer(const StringPtr& propName, bool& setValue);
    BaseObjectPtr getFullPropName(const std::string& propName) const;

    void checkCanSetPropertyValue(const StringPtr& propName);
};

class ConfigClientStandalonePropertyObjectImpl : public GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>
{
public:
    using Super = GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>;

    ErrCode INTERFACE_FUNC deserializeValues(ISerializedObject* serializedObject,
                                             IBaseObject* context,
                                             IFunction* callbackFactory) override;
    ErrCode INTERFACE_FUNC complete() override;
    ErrCode INTERFACE_FUNC getDeserializedParameter(IString* parameter, IBaseObject** value) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

template <class Impl>
template <class ... Args>
ConfigClientPropertyObjectBaseImpl<Impl>::ConfigClientPropertyObjectBaseImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                                             const std::string& remoteGlobalId,
                                                                             const Args& ... args)
    : ConfigClientObjectImpl(configProtocolClientComm, remoteGlobalId)
    , Impl(args ...)
    , deserializationComplete(false)
{
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    const auto propertyNamePtr = StringPtr::Borrow(propertyName);
    const auto valuePtr = BaseObjectPtr::Borrow(value);
    return daqTry(
        [this, &propertyNamePtr, &valuePtr]()
        {
            const auto fullPropName = getFullPropName(propertyNamePtr);
            checkCanSetPropertyValue(fullPropName);
            clientComm->setPropertyValue(remoteGlobalId, fullPropName, valuePtr);
        });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    if (!deserializationComplete)
        return Impl::setProtectedPropertyValue(propertyName, value);

    const auto propertyNamePtr = StringPtr::Borrow(propertyName);
    const auto valuePtr = BaseObjectPtr::Borrow(value);
    return daqTry([this, &propertyNamePtr, &valuePtr]()
    {
            const auto fullPropName = getFullPropName(propertyNamePtr);
            checkCanSetPropertyValue(fullPropName);
            clientComm->setProtectedPropertyValue(remoteGlobalId, fullPropName, valuePtr);
    });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getPropertyValue(IString* propertyName, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);
    OPENDAQ_PARAM_NOT_NULL(value);

    auto propertyNamePtr = StringPtr::Borrow(propertyName);

    return daqTry(
        [this, &propertyNamePtr, &value]()
        {
            if (clientComm->getConnected())
            {
                bool setValue;
                auto v = getValueFromServer(propertyNamePtr, setValue);

                if (setValue)
                    Impl::setPropertyValue(propertyNamePtr, v);
                *value = v.detach();
                return OPENDAQ_SUCCESS;
            }

            return Impl::getPropertyValue(propertyNamePtr, value);
        });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getPropertySelectionValue(IString* propertyName, IBaseObject** value)
{
    return Impl::getPropertySelectionValue(propertyName, value);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::clearPropertyValue(IString* propertyName)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    const auto propertyNamePtr = StringPtr::Borrow(propertyName);
    return daqTry([this, &propertyNamePtr]()
    {
        clientComm->clearPropertyValue(remoteGlobalId, propertyNamePtr);
    });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getProperty(IString* propertyName, IProperty** value)
{
    return Impl::getProperty(propertyName, value);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::addProperty(IProperty* property)
{
    if (!deserializationComplete)
        return Impl::addProperty(property);

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
    return Impl::getOnPropertyValueWrite(propertyName, event);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getOnPropertyValueRead(IString* propertyName, IEvent** event)
{
    return Impl::getOnPropertyValueRead(propertyName, event);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getVisibleProperties(IList** properties)
{
    return Impl::getVisibleProperties(properties);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::hasProperty(IString* propertyName, Bool* hasProperty)
{
    return Impl::hasProperty(propertyName, hasProperty);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getAllProperties(IList** properties)
{
    return Impl::getAllProperties(properties);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setPropertyOrder(IList* orderedPropertyNames)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::complete()
{
    deserializationComplete = true;
    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getRemoteGlobalId(IString** remoteGlobalId)
{
    OPENDAQ_PARAM_NOT_NULL(remoteGlobalId);

    *remoteGlobalId = String(this->remoteGlobalId).detach();
    return OPENDAQ_SUCCESS;
}

template <class Impl>
BaseObjectPtr ConfigClientPropertyObjectBaseImpl<Impl>::getValueFromServer(const StringPtr& propName, bool& setValue)
{
    const auto prop = Impl::getUnboundProperty(propName);
    setValue = false;
    switch (const auto vt = prop.getValueType())
    {
        case ctProc:
            return createWithImplementation<IProcedure, ConfigClientProcedureImpl>(clientComm, remoteGlobalId, this->path, propName);
        case ctFunc:
            return createWithImplementation<IFunction, ConfigClientFunctionImpl>(clientComm, remoteGlobalId, this->path, propName);
        case ctObject:
        {
            BaseObjectPtr obj;
            checkErrorInfo(Impl::getPropertyValue(propName, &obj));
            return obj;
        }
        default:
        {
            setValue = true;
            return clientComm->getPropertyValue(remoteGlobalId, this->getFullPropName(propName));
        }
    }
}

template <class Impl>
BaseObjectPtr ConfigClientPropertyObjectBaseImpl<Impl>::getFullPropName(const std::string& propName) const
{
    auto fullPropName = propName;
    if (this->path.assigned() && this->path != "")
        fullPropName = this->path.toStdString() + "." + fullPropName;
    return fullPropName;
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::checkCanSetPropertyValue(const StringPtr& propName)
{
    const auto prop = Impl::getUnboundProperty(propName);
    switch (const auto vt = prop.getValueType())
    {
        case ctProc:
        case ctFunc:
            throw InvalidOperationException("Cannot set remote function property");
        default:
            return;
    }
}

inline ErrCode ConfigClientStandalonePropertyObjectImpl::deserializeValues(ISerializedObject* serializedObject,
                                                                           IBaseObject* context,
                                                                           IFunction* callbackFactory)
{
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientStandalonePropertyObjectImpl::complete()
{
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientStandalonePropertyObjectImpl::getDeserializedParameter(IString* parameter, IBaseObject** value)
{
    return OPENDAQ_NOTFOUND;
}

inline ErrCode ConfigClientStandalonePropertyObjectImpl::Deserialize(ISerializedObject* serialized,
    IBaseObject* context,
    IFunction* factoryCallback,
    IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            const auto serializedPtr = SerializedObjectPtr::Borrow(serialized);
            if (!serializedPtr.assigned())
                throw ArgumentNullException("Serialized object not assigned");

            const auto contextPtr = BaseObjectPtr::Borrow(context);
            if (!contextPtr.assigned())
                throw ArgumentNullException("Deserialization context not assigned");

            const auto componentDeserializeContext = contextPtr.asPtrOrNull<IComponentDeserializeContext>(true);
            if (!componentDeserializeContext.assigned())
                throw InvalidParameterException("Invalid deserialization context");

            const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

            PropertyObjectPtr propObj = Super::DeserializePropertyObject(
                serializedPtr,
                contextPtr,
                factoryCallbackPtr,
                [&componentDeserializeContext, &factoryCallback](
                    const SerializedObjectPtr& serialized, const BaseObjectPtr& context, const StringPtr& className)
                {
                    const auto ctx = componentDeserializeContext.asPtr<IConfigProtocolDeserializeContext>();

                    PropertyObjectPtr propObj = createWithImplementation<IPropertyObject, ConfigClientPropertyObjectImpl>(
                        ctx->getClientComm(), ctx->getRemoteGlobalId());

                    return propObj;
                });

            const auto deserializeComponent = propObj.asPtr<IDeserializeComponent>(true);
            deserializeComponent.complete();

            *obj = propObj.detach();
        });
}

}
