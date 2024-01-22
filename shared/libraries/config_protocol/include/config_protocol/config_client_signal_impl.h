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
#include <config_protocol/config_client_component_impl.h>
#include <opendaq/mirrored_signal_impl.h>

namespace daq::config_protocol
{

class ConfigClientSignalImpl : public ConfigClientComponentBaseImpl<MirroredSignalBase<IConfigClientObject>>
{
public:
    using Super = ConfigClientComponentBaseImpl<MirroredSignalBase<IConfigClientObject>>;

    ConfigClientSignalImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                           const std::string& remoteGlobalId,
                           const ContextPtr& ctx,
                           const ComponentPtr& parent,
                           const StringPtr& localId);

    StringPtr onGetRemoteId() const override;
    Bool onTriggerEvent(EventPacketPtr eventPacket) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};


inline ConfigClientSignalImpl::ConfigClientSignalImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                      const std::string& remoteGlobalId,
                                                      const ContextPtr& ctx,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId)
{
}

inline StringPtr ConfigClientSignalImpl::onGetRemoteId() const
{
    return String(remoteGlobalId).detach();
}

inline Bool ConfigClientSignalImpl::onTriggerEvent(EventPacketPtr eventPacket)
{
    // TODO
    return True;
}

inline ErrCode ConfigClientSignalImpl::Deserialize(ISerializedObject* serialized,
                                                   IBaseObject* context,
                                                   IFunction* factoryCallback,
                                                   IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = DeserializeConfigComponent<ISignal, ConfigClientSignalImpl>(serialized, context, factoryCallback).detach();
        });
}


}
