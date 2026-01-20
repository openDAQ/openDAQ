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
#include <config_protocol/config_client_component_impl.h>
#include <opendaq/sync_component2_impl.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>

namespace daq::config_protocol
{

template <class Impl>
class ConfigClientBaseSyncComponent2Impl;

using ConfigClientSyncComponent2Impl = ConfigClientBaseSyncComponent2Impl<SyncComponent2Impl<ISyncComponent2, IConfigClientObject>>;

template <class Impl>
class ConfigClientBaseSyncComponent2Impl : public ConfigClientComponentBaseImpl<Impl>
{
public:

    using Super = ConfigClientComponentBaseImpl<Impl>;
    using Super::Super;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    template <class Interface, class Implementation>
    static BaseObjectPtr DeserializeSyncComponent2(const SerializedObjectPtr& serialized,
                                                    const BaseObjectPtr& context,
                                                    const FunctionPtr& factoryCallback);

    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;
};

template <class Impl>
ErrCode ConfigClientBaseSyncComponent2Impl<Impl>::Deserialize(ISerializedObject* serialized,
                                                               IBaseObject* context,
                                                               IFunction* factoryCallback,
                                                               IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    const ErrCode errCode = daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        *obj = DeserializeSyncComponent2<ISyncComponent2, ConfigClientSyncComponent2Impl>(serialized, context, factoryCallback).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class Impl>
template <class Interface, class Implementation>
BaseObjectPtr ConfigClientBaseSyncComponent2Impl<Impl>::DeserializeSyncComponent2(const SerializedObjectPtr& serialized,
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
void ConfigClientBaseSyncComponent2Impl<Impl>::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
{
    ConfigClientPropertyObjectBaseImpl<Impl>::handleRemoteCoreObjectInternal(sender, args);
}

} // namespace daq::config_protocol
