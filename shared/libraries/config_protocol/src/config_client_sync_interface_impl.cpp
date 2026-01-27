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

#include <config_protocol/config_client_sync_interface_impl.h>
#include <opendaq/sync_interface.h>
#include <coretypes/objectptr.h>
#include <opendaq/component_deserialize_context_ptr.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/function_ptr.h>

namespace daq::config_protocol
{

ConfigClientSyncInterfaceImpl::ConfigClientSyncInterfaceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                             const std::string& remoteGlobalId)
    : Super(configProtocolClientComm, remoteGlobalId)
{
}

ErrCode ConfigClientSyncInterfaceImpl::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    if (remoteUpdating)
        return Impl::setPropertyValue(propertyName, value);
    return Super::setPropertyValue(propertyName, value);
}

ErrCode ConfigClientSyncInterfaceImpl::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    if (remoteUpdating)
        return Impl::setProtectedPropertyValue(propertyName, value);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED, "Setting protected values is not allowed on client sync interface");
}

ErrCode ConfigClientSyncInterfaceImpl::clearPropertyValue(IString* propertyName)
{
    if (remoteUpdating)
        return Impl::clearPropertyValue(propertyName);
    return Super::clearPropertyValue(propertyName);
}

ErrCode ConfigClientSyncInterfaceImpl::addProperty(IProperty* property)
{
    if (remoteUpdating)
        return Impl::addProperty(property);
    return Super::addProperty(property);
}

ErrCode ConfigClientSyncInterfaceImpl::removeProperty(IString* propertyName)
{
    if (remoteUpdating)
        return Impl::removeProperty(propertyName);
    return Super::removeProperty(propertyName);
}

ErrCode ConfigClientSyncInterfaceImpl::beginUpdate()
{
    if (remoteUpdating)
        return Impl::beginUpdate();
    return Super::beginUpdate();
}

ErrCode ConfigClientSyncInterfaceImpl::endUpdate()
{
    if (remoteUpdating)
        return Impl::endUpdate();
    return Super::endUpdate();
}

ErrCode ConfigClientSyncInterfaceImpl::setAsSource(Bool isSource)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Setting as source is not supported on client sync interface");
}

ErrCode ConfigClientSyncInterfaceImpl::deserializeValues(ISerializedObject* serializedObject,
                                                          IBaseObject* context,
                                                          IFunction* callbackFactory)
{
    return OPENDAQ_SUCCESS;
}

ErrCode ConfigClientSyncInterfaceImpl::complete()
{
    return Super::complete();
}

ErrCode ConfigClientSyncInterfaceImpl::getDeserializedParameter(IString* parameter, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(parameter);
    OPENDAQ_PARAM_NOT_NULL(value);
    return OPENDAQ_NOTFOUND;
}

ErrCode ConfigClientSyncInterfaceImpl::Deserialize(ISerializedObject* serialized,
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
            [&configDeserializeContext](const SerializedObjectPtr& serialized, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className)
            {
                auto obj = createWithImplementation<ISyncInterface, ConfigClientSyncInterfaceImpl>(
                    configDeserializeContext->getClientComm(),
                    configDeserializeContext->getRemoteGlobalId());
                obj.as<IConfigClientObject>(true)->setRemoteUpdating(true);
                return obj;
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
