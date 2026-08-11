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

#include <config_protocol/config_client_synchronization_impl.h>
#include <opendaq/synchronization.h>
#include <opendaq/component_deserialize_context_ptr.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/objectptr.h>

namespace daq::config_protocol
{

ConfigClientSynchronizationImpl::ConfigClientSynchronizationImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,const std::string& remoteGlobalId)
    : Super(configProtocolClientComm, remoteGlobalId, True)
{
}

ErrCode ConfigClientSynchronizationImpl::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    if (this->remoteUpdating)
        return Impl::setPropertyValue(propertyName, value);
    return Super::setPropertyValue(propertyName, value);
}

ErrCode ConfigClientSynchronizationImpl::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    if (this->remoteUpdating)
        return Impl::setProtectedPropertyValue(propertyName, value);
    return Super::setProtectedPropertyValue(propertyName, value);
}

ErrCode ConfigClientSynchronizationImpl::clearPropertyValue(IString* propertyName)
{
    if (this->remoteUpdating)
        return Impl::clearPropertyValue(propertyName);
    return Super::clearPropertyValue(propertyName);
}

ErrCode ConfigClientSynchronizationImpl::addProperty(IProperty* property)
{
    if (this->remoteUpdating)
        return Impl::addProperty(property);
    return Super::addProperty(property);
}

ErrCode ConfigClientSynchronizationImpl::removeProperty(IString* propertyName)
{
    if (this->remoteUpdating)
        return Impl::removeProperty(propertyName);
    return Super::removeProperty(propertyName);
}

ErrCode ConfigClientSynchronizationImpl::beginUpdate()
{
    if (this->remoteUpdating)
        return Impl::beginUpdate();
    return Super::beginUpdate();
}

ErrCode ConfigClientSynchronizationImpl::endUpdate()
{
    if (this->remoteUpdating)
        return Impl::endUpdate();
    return Super::endUpdate();
}

ErrCode ConfigClientSynchronizationImpl::getSelectedSource(ISyncInterface** selectedSource)
{
    OPENDAQ_PARAM_NOT_NULL(selectedSource);
    return daqTry([&]
    {
        StringPtr sourceName = this->objPtr.getPropertyValue("Source");
        PropertyObjectPtr interfaces = this->objPtr.getPropertyValue("Interfaces");
        *selectedSource = interfaces.getPropertyValue(sourceName).template as<ISyncInterface>();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConfigClientSynchronizationImpl::addInterface(ISyncInterface* syncInterface)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Adding interfaces is not supported on the client side");
}

ErrCode ConfigClientSynchronizationImpl::deserializeValues(ISerializedObject* /*serializedObject*/,
                                                                   IBaseObject* /*context*/,
                                                                   IFunction* /*callbackFactory*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode ConfigClientSynchronizationImpl::getDeserializedParameter(IString* parameter, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(parameter);
    OPENDAQ_PARAM_NOT_NULL(value);
    return OPENDAQ_NOTFOUND;
}

ErrCode ConfigClientSynchronizationImpl::Deserialize(ISerializedObject* serialized,
                                                              IBaseObject* context,
                                                              IFunction* factoryCallback,
                                                              IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_PARAM_NOT_NULL(context);

    const ErrCode errCode = daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        const auto contextPtr = BaseObjectPtr::Borrow(context);
        if (!contextPtr.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Deserialization context not assigned");

        const auto componentDeserializeContext = contextPtr.asPtrOrNull<IComponentDeserializeContext>(true);
        if (!componentDeserializeContext.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Invalid deserialization context");

        const auto configDeserializeContext = componentDeserializeContext.asPtr<IConfigProtocolDeserializeContext>();

        const auto serializedPtr = SerializedObjectPtr::Borrow(serialized);
        const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

        PropertyObjectPtr propObj = Super::DeserializePropertyObject(
            serializedPtr,
            contextPtr,
            factoryCallbackPtr,
            [&configDeserializeContext](const SerializedObjectPtr&, const BaseObjectPtr&, const StringPtr&)
            {
                auto syncComponent = createWithImplementation<ISynchronization, ConfigClientSynchronizationImpl>(
                    configDeserializeContext->getClientComm(),
                    configDeserializeContext->getRemoteGlobalId());
                syncComponent.as<IConfigClientObject>(true)->setRemoteUpdating(true);
                return syncComponent;
            });

        propObj.as<IConfigClientObject>(true)->setRemoteUpdating(false);
        const auto deserializeComponent = propObj.asPtr<IDeserializeComponent>(true);
        deserializeComponent.complete();

        *obj = propObj.detach();
        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

} // namespace daq::config_protocol
