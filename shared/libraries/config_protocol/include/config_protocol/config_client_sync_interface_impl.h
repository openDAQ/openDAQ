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

class ConfigClientSyncInterfaceImpl : public ConfigClientPropertyObjectBaseImpl<SyncInterfaceBaseImpl<ISyncInterface, IConfigClientObject, IDeserializeComponent>>
{
public:
    using Impl = SyncInterfaceBaseImpl<ISyncInterface, IConfigClientObject, IDeserializeComponent>;
    using Super = ConfigClientPropertyObjectBaseImpl<Impl>;

    ConfigClientSyncInterfaceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                  const std::string& remoteGlobalId);

    // IDeserializeComponent
    ErrCode INTERFACE_FUNC deserializeValues(ISerializedObject* serializedObject, IBaseObject* context, IFunction* callbackFactory) override;
    ErrCode INTERFACE_FUNC complete() override;
    ErrCode INTERFACE_FUNC getDeserializedParameter(IString* parameter, IBaseObject** value) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;
};

inline ConfigClientSyncInterfaceImpl::ConfigClientSyncInterfaceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                                     const std::string& remoteGlobalId)
    : Super(configProtocolClientComm, remoteGlobalId)
{
}

inline ErrCode ConfigClientSyncInterfaceImpl::deserializeValues(ISerializedObject* serializedObject,
                                                                 IBaseObject* context,
                                                                 IFunction* callbackFactory)
{
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientSyncInterfaceImpl::complete()
{
    return Super::complete();
}

inline ErrCode ConfigClientSyncInterfaceImpl::getDeserializedParameter(IString* parameter, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(parameter);
    OPENDAQ_PARAM_NOT_NULL(value);
    return OPENDAQ_NOTFOUND;
}

inline ErrCode ConfigClientSyncInterfaceImpl::Deserialize(ISerializedObject* serialized,
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
                return createWithImplementation<ISyncInterface, ConfigClientSyncInterfaceImpl>(
                    configDeserializeContext->getClientComm(),
                    configDeserializeContext->getRemoteGlobalId());
            });

        const auto deserializeComponent = propObj.asPtr<IDeserializeComponent>(true);
        deserializeComponent.complete();

        *obj = propObj.detach();
        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

inline void ConfigClientSyncInterfaceImpl::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
{
    Super::handleRemoteCoreObjectInternal(sender, args);
}

} // namespace daq::config_protocol
