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

protected:
    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;

private:
    void descriptorChanged(const CoreEventArgsPtr& args);
    void attributeChanged(const CoreEventArgsPtr& args);
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

inline void ConfigClientSignalImpl::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
{
    switch (static_cast<CoreEventId>(args.getEventId()))
    {
        case CoreEventId::DataDescriptorChanged:
            descriptorChanged(args);
            break;
        case CoreEventId::AttributeChanged:
            attributeChanged(args);
            break;
        case CoreEventId::SignalConnected:
        case CoreEventId::SignalDisconnected:
        case CoreEventId::ComponentUpdateEnd:
        case CoreEventId::TagsChanged:
        case CoreEventId::PropertyValueChanged:
        case CoreEventId::PropertyObjectUpdateEnd:
        case CoreEventId::PropertyAdded:
        case CoreEventId::PropertyRemoved:
        case CoreEventId::ComponentAdded:
        case CoreEventId::ComponentRemoved:
        case CoreEventId::StatusChanged:
        default:
            break;
    }

    ConfigClientComponentBaseImpl<MirroredSignalBase<IConfigClientObject>>::handleRemoteCoreObjectInternal(sender, args);
}

inline void ConfigClientSignalImpl::descriptorChanged(const CoreEventArgsPtr& args)
{
    this->dataDescriptor = args.getParameters().get("DataDescriptor");
    if (!this->coreEventMuted && this->coreEvent.assigned())
        this->triggerCoreEvent(args);
}

inline void ConfigClientSignalImpl::attributeChanged(const CoreEventArgsPtr& args)
{
    const std::string attrName = args.getParameters().get("AttributeName");
    const bool relock = this->lockedAttributes.erase(attrName);

    if (attrName == "RelatedSignals")
    {
        const ListPtr<ISignal> relatedSignals = args.getParameters().get("RelatedSignals");
        checkErrorInfo(SignalBase<IMirroredSignalConfig, IMirroredSignalPrivate, IConfigClientObject>::setRelatedSignals(relatedSignals));
    }
    else if (attrName == "DomainSignal")
    {
        const SignalPtr domainSignal = args.getParameters().get("DomainSignal");
        checkErrorInfo(SignalBase<IMirroredSignalConfig, IMirroredSignalPrivate, IConfigClientObject>::setDomainSignal(domainSignal));
    }

    if (relock)
        this->lockedAttributes.insert(attrName);
}
}
