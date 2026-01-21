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

using ConfigClientSyncComponent2Impl = ConfigClientBaseSyncComponent2Impl<SyncComponent2Impl<IComponent, IConfigClientObject>>;

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

} // namespace daq::config_protocol
