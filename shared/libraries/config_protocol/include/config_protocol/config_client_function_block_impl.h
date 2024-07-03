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
#include <opendaq/function_block_impl.h>

namespace daq::config_protocol
{

template <class Impl>
class ConfigClientBaseFunctionBlockImpl;

using ConfigClientFunctionBlockImpl = ConfigClientBaseFunctionBlockImpl<FunctionBlockImpl<IFunctionBlock, IConfigClientObject>>;

template <class Impl>
class ConfigClientBaseFunctionBlockImpl : public ConfigClientComponentBaseImpl<Impl>
{
public:
    using Super = ConfigClientBaseFunctionBlockImpl<FunctionBlockImpl<IFunctionBlock, IConfigClientObject>>;

    ConfigClientBaseFunctionBlockImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                      const std::string& remoteGlobalId,
                                      const FunctionBlockTypePtr& functionBlockType,
                                      const ContextPtr& ctx,
                                      const ComponentPtr& parent,
                                      const StringPtr& localId,
                                      const StringPtr& className = nullptr);

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void onRemoteUpdate(const SerializedObjectPtr& serialized) override;
};

template <class Impl>
ConfigClientBaseFunctionBlockImpl<Impl>::ConfigClientBaseFunctionBlockImpl(
    const ConfigProtocolClientCommPtr& configProtocolClientComm,
    const std::string& remoteGlobalId,
    const FunctionBlockTypePtr& type,
    const ContextPtr& ctx,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const StringPtr& className)
    : ConfigClientComponentBaseImpl<Impl>(configProtocolClientComm, remoteGlobalId, type, ctx, parent, localId, className)
{
}

template <class Impl>
ErrCode ConfigClientBaseFunctionBlockImpl<Impl>::Deserialize(ISerializedObject* serialized,
    IBaseObject* context,
    IFunction* factoryCallback,
    IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializeComponent(
                       serialized,
                       context,
                       factoryCallback,
                       [](const SerializedObjectPtr& serialized,
                          const ComponentDeserializeContextPtr& deserializeContext,
                          const StringPtr& className)
                       {
                            const auto configDeserializeContext = deserializeContext.asPtr<IConfigProtocolDeserializeContext>();

                            const auto typeId = serialized.readString("typeId");

                            const auto fbType = FunctionBlockType(typeId, typeId, "", nullptr);

                            return createWithImplementation<IFunctionBlock, ConfigClientFunctionBlockImpl>(
                                configDeserializeContext->getClientComm(),
                                configDeserializeContext->getRemoteGlobalId(),
                                fbType,
                                deserializeContext.getContext(),
                                deserializeContext.getParent(),
                                deserializeContext.getLocalId(),
                                className);
                       })
                       .detach();
        });
}

template <class Impl>
void ConfigClientBaseFunctionBlockImpl<Impl>::onRemoteUpdate(const SerializedObjectPtr& serialized)
{
    ConfigClientComponentBaseImpl<Impl>::onRemoteUpdate(serialized);

    for (const auto& comp : this->components)
    {
        const auto id = comp.getLocalId();
        if (serialized.hasKey(id))
        {
            const auto serObj = serialized.readSerializedObject(id);
            comp.template asPtr<IConfigClientObject>()->remoteUpdate(serObj);
        }
    }
}
}
