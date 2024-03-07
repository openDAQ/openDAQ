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

DECLARE_OPENDAQ_INTERFACE(IConfigClientSignalPrivate, IBaseObject)
{
    virtual void INTERFACE_FUNC assignDomainSignal(const SignalPtr& domainSignal) = 0;
};

class ConfigClientSignalImpl : public ConfigClientComponentBaseImpl<MirroredSignalBase<IConfigClientObject, IConfigClientSignalPrivate>>
{
public:
    using Super = ConfigClientComponentBaseImpl<MirroredSignalBase<IConfigClientObject, IConfigClientSignalPrivate>>;

    ConfigClientSignalImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                           const std::string& remoteGlobalId,
                           const ContextPtr& ctx,
                           const ComponentPtr& parent,
                           const StringPtr& localId,
                           const StringPtr& className = nullptr);

    // IConfigClientSignalPrivate
    void INTERFACE_FUNC assignDomainSignal(const SignalPtr& domainSignal) override;

    StringPtr onGetRemoteId() const override;
    Bool onTriggerEvent(EventPacketPtr eventPacket) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;
    void onRemoteUpdate(const SerializedObjectPtr& serialized) override;

private:
    void descriptorChanged(const CoreEventArgsPtr& args);
    void attributeChanged(const CoreEventArgsPtr& args);
};


inline ConfigClientSignalImpl::ConfigClientSignalImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                      const std::string& remoteGlobalId,
                                                      const ContextPtr& ctx,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId,
                                                      const StringPtr& className)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId, className)
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
        case CoreEventId::TypeAdded:
        case CoreEventId::TypeRemoved:
        default:
            break;
    }

    Super::handleRemoteCoreObjectInternal(sender, args);
}

inline void ConfigClientSignalImpl::onRemoteUpdate(const SerializedObjectPtr& serialized)
{
    ConfigClientComponentBaseImpl::onRemoteUpdate(serialized);

    if (serialized.hasKey("domainSignalId"))
        deserializedDomainSignalId = serialized.readString("domainSignalId");
    else
        deserializedDomainSignalId.release();

    if (serialized.hasKey("dataDescriptor"))
        this->dataDescriptor = serialized.readObject("dataDescriptor");
}

inline void ConfigClientSignalImpl::descriptorChanged(const CoreEventArgsPtr& args)
{
    this->dataDescriptor = args.getParameters().get("DataDescriptor");
    if (!this->coreEventMuted && this->coreEvent.assigned())
        this->triggerCoreEvent(args);
}

inline void ConfigClientSignalImpl::assignDomainSignal(const SignalPtr& domainSignal)
{
    const bool relock = this->lockedAttributes.erase("DomainSignal");
    const bool unmuteCoreEvent = !this->coreEventMuted;

    this->coreEventMuted = true;

    checkErrorInfo(SignalBase<IMirroredSignalConfig, IMirroredSignalPrivate, IConfigClientObject, IConfigClientSignalPrivate>::setDomainSignal(domainSignal));

    if (relock)
        this->lockedAttributes.insert("DomainSignal");

    if (unmuteCoreEvent)
        this->coreEventMuted = false;
}

inline void ConfigClientSignalImpl::attributeChanged(const CoreEventArgsPtr& args)
{
    const std::string attrName = args.getParameters().get("AttributeName");
    const bool relock = this->lockedAttributes.erase(attrName);

    if (attrName == "RelatedSignals")
    {
        const ListPtr<ISignal> relatedSignals = args.getParameters().get("RelatedSignals");
        checkErrorInfo(SignalBase<IMirroredSignalConfig, IMirroredSignalPrivate, IConfigClientObject, IConfigClientSignalPrivate>::setRelatedSignals(relatedSignals));
    }
    else if (attrName == "DomainSignal")
    {
        const SignalPtr domainSignal = args.getParameters().get("DomainSignal");
        checkErrorInfo(SignalBase<IMirroredSignalConfig, IMirroredSignalPrivate, IConfigClientObject, IConfigClientSignalPrivate>::setDomainSignal(domainSignal));
    }

    if (relock)
        this->lockedAttributes.insert(attrName);
}
}
