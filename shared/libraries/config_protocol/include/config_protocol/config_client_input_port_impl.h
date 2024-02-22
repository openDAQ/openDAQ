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
#include <opendaq/input_port_impl.h>

namespace daq::config_protocol
{

class ConfigClientInputPortImpl : public ConfigClientComponentBaseImpl<GenericInputPortImpl<IConfigClientObject>>
{
public:
    using Super = ConfigClientComponentBaseImpl<GenericInputPortImpl<IConfigClientObject>>;

    ConfigClientInputPortImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                              const std::string& remoteGlobalId,
                              const ContextPtr& ctx,
                              const ComponentPtr& parent,
                              const StringPtr& localId);

    ErrCode INTERFACE_FUNC connect(ISignal* signal) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;
};

inline ConfigClientInputPortImpl::ConfigClientInputPortImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                            const std::string& remoteGlobalId,
                                                            const ContextPtr& ctx,
                                                            const ComponentPtr& parent,
                                                            const StringPtr& localId)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId)
{
}

inline ErrCode ConfigClientInputPortImpl::connect(ISignal* signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    return daqTry([this, &signal] {
        const auto signalPtr = SignalPtr::Borrow(signal);

        const auto configObject = signalPtr.asPtrOrNull<IConfigClientObject>(true);
        if (!configObject.assigned())
            throw InvalidParameterException("Not a remote signal");

        StringPtr signalRemoteGlobalId;
        checkErrorInfo(configObject->getRemoteGlobalId(&signalRemoteGlobalId));

        auto params = ParamsDict({{"SignalId", signalRemoteGlobalId}});
        clientComm->sendComponentCommand(remoteGlobalId, "ConnectSignal", params, nullptr);
    });
}

inline ErrCode ConfigClientInputPortImpl::Deserialize(ISerializedObject* serialized,
                                                      IBaseObject* context,
                                                      IFunction* factoryCallback,
                                                      IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = DeserializeConfigComponent<IInputPortConfig, ConfigClientInputPortImpl>(serialized, context, factoryCallback).detach();
        });
}

inline void ConfigClientInputPortImpl::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
{
    switch (static_cast<CoreEventId>(args.getEventId()))
    {
        case CoreEventId::SignalConnected:
        case CoreEventId::SignalDisconnected:
            if (!this->coreEventMuted && this->coreEvent.assigned())
                this->triggerCoreEvent(args);
            break;
        case CoreEventId::ComponentUpdateEnd:
        case CoreEventId::AttributeChanged:
        case CoreEventId::TagsChanged:
        case CoreEventId::PropertyValueChanged:
        case CoreEventId::PropertyObjectUpdateEnd:
        case CoreEventId::PropertyAdded:
        case CoreEventId::PropertyRemoved:
        case CoreEventId::DataDescriptorChanged:
        case CoreEventId::ComponentAdded:
        case CoreEventId::ComponentRemoved:
        case CoreEventId::StatusChanged:
        case CoreEventId::TypeAdded:
        case CoreEventId::TypeRemoved:
        default:
            break;
    }

    ConfigClientComponentBaseImpl<GenericInputPortImpl<IConfigClientObject>>::handleRemoteCoreObjectInternal(sender, args);
}
}
