/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <config_protocol/config_client_component_impl.h>
#include <opendaq/sync_component_impl.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>

namespace daq::config_protocol
{

template <class Impl>
class ConfigClientBaseSyncComponentImpl;

using ConfigClientSyncComponentImpl = ConfigClientBaseSyncComponentImpl<GenericSyncComponentImpl<ISyncComponent, IConfigClientObject>>;

template <class Impl>
class ConfigClientBaseSyncComponentImpl : public ConfigClientComponentBaseImpl<Impl>
{
public:

    using Super = ConfigClientComponentBaseImpl<Impl>;
    ConfigClientBaseSyncComponentImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                               const std::string& remoteGlobalId,
                               const ContextPtr& ctx,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const StringPtr& className = nullptr);

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    ErrCode INTERFACE_FUNC setSelectedSource(Int selectedSource) override
    {
        return Super::setPropertyValue(String("Source"), Integer(selectedSource));
    }

protected:
    template <class Interface, class Implementation>
    static BaseObjectPtr DeserializeSyncComponent(const SerializedObjectPtr& serialized,
                                                 const BaseObjectPtr& context,
                                                 const FunctionPtr& factoryCallback);

    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;
};

template <class Impl>
ConfigClientBaseSyncComponentImpl<Impl>::ConfigClientBaseSyncComponentImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                             const std::string& remoteGlobalId,
                                                             const ContextPtr& ctx,
                                                             const ComponentPtr& parent,
                                                             const StringPtr& localId,
                                                             const StringPtr& className)

    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId, className)
{
}

template <class Impl>
ErrCode ConfigClientBaseSyncComponentImpl<Impl>::Deserialize(ISerializedObject* serialized,
    IBaseObject* context,
    IFunction* factoryCallback,
    IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = DeserializeSyncComponent<ISyncComponent, ConfigClientSyncComponentImpl>(serialized, context, factoryCallback).detach();
        });
}

template <class Impl>
template <class Interface, class Implementation>
BaseObjectPtr ConfigClientBaseSyncComponentImpl<Impl>::DeserializeSyncComponent(
    const SerializedObjectPtr& serialized,
    const BaseObjectPtr& context,
    const FunctionPtr& factoryCallback)
{
    return Impl::DeserializeComponent(
        serialized,
        context,
        factoryCallback,
        [](const SerializedObjectPtr& serialized, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className)
        {
            const auto ctx = deserializeContext.asPtr<IConfigProtocolDeserializeContext>();
            return createWithImplementation<Interface, Implementation>(ctx->getClientComm(),
                                                                           ctx->getRemoteGlobalId(),
                                                                           deserializeContext.getContext(),
                                                                           deserializeContext.getParent(),
                                                                           deserializeContext.getLocalId(),
                                                                           className);
        });
}

template <class Impl>
void ConfigClientBaseSyncComponentImpl<Impl>::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
{
    ConfigClientPropertyObjectBaseImpl<Impl>::handleRemoteCoreObjectInternal(sender, args);
}
} // namespace daq::config_protocol
