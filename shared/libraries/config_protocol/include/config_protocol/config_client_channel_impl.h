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
#include <config_protocol/config_client_function_block_impl.h>
#include <opendaq/channel_impl.h>

namespace daq::config_protocol
{

class ConfigClientChannelImpl : public ConfigClientBaseFunctionBlockImpl<ChannelImpl<IConfigClientObject>>
{
public:
    using Super = ConfigClientBaseFunctionBlockImpl<ChannelImpl<IConfigClientObject>>;

    ConfigClientChannelImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                            const std::string& remoteGlobalId,
                            const FunctionBlockTypePtr& type,
                            const ContextPtr& ctx,
                            const ComponentPtr& parent,
                            const StringPtr& localId);

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

inline ConfigClientChannelImpl::ConfigClientChannelImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                        const std::string& remoteGlobalId,
                                                        const FunctionBlockTypePtr& type,
                                                        const ContextPtr& ctx,
                                                        const ComponentPtr& parent,
                                                        const StringPtr& localId)
    : Super(configProtocolClientComm, remoteGlobalId, type, ctx, parent, localId)
{
}

inline ErrCode ConfigClientChannelImpl::Deserialize(ISerializedObject* serialized,
    IBaseObject* context,
    IFunction* factoryCallback,
    IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializeComponent(serialized,
                                               context,
                                               factoryCallback,
                                               [](const SerializedObjectPtr& serialized,
                                                  const ComponentDeserializeContextPtr& deserializeContext,
                                                  const StringPtr& className)
                                               {
                                                   const auto configDeserializeContext =
                                                       deserializeContext.asPtr<IConfigProtocolDeserializeContext>();

                                                   const auto typeId = serialized.readString("typeId");

                                                   const auto fbType = FunctionBlockType(typeId, typeId, "", nullptr);

                                                   return createWithImplementation<IChannel, ConfigClientChannelImpl>(
                                                       configDeserializeContext->getClientComm(),
                                                       configDeserializeContext->getRemoteGlobalId(),
                                                       fbType,
                                                       deserializeContext.getContext(),
                                                       deserializeContext.getParent(),
                                                       deserializeContext.getLocalId());
                                               })
                       .detach();
        });
}

}
