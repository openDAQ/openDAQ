/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/channel.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/tags_factory.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename... Interfaces>
class ChannelImpl;

using Channel = ChannelImpl<>;

template <typename... Interfaces>
class ChannelImpl : public FunctionBlockImpl<IChannel, Interfaces...>
{
public:
    using Self = ChannelImpl<Interfaces...>;
    using Super = FunctionBlockImpl<IChannel, Interfaces...>;

    explicit ChannelImpl(const FunctionBlockTypePtr& fbType,
                         const ContextPtr& context,
                         const ComponentPtr& parent,
                         const StringPtr& localId,
                         const StringPtr& className = nullptr);

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

template <typename... Interfaces>
ChannelImpl<Interfaces...>::ChannelImpl(const FunctionBlockTypePtr& fbType,
                                        const ContextPtr& context,
                                        const ComponentPtr& parent,
                                        const StringPtr& localId,
                                        const StringPtr& className)
    : FunctionBlockImpl<IChannel, Interfaces...>(fbType, context, parent, localId, className)
{
}

template <typename... Interfaces>
ErrCode INTERFACE_FUNC ChannelImpl<Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ConstCharPtr ChannelImpl<Interfaces...>::SerializeId()
{
    return "Channel";
}

template <typename... Interfaces>
ErrCode ChannelImpl<Interfaces...>::Deserialize(ISerializedObject* serialized,
                                                IBaseObject* context,
                                                IFunction* factoryCallback,
                                                IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super:: template DeserializeFunctionBlock<Channel>(serialized,
                                                            context,
                                                            factoryCallback).detach();
        });
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(Channel)

END_NAMESPACE_OPENDAQ
