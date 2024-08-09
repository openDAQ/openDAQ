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
#include <opendaq/mirrored_signal_impl.h>

namespace daq::config_protocol
{

DECLARE_OPENDAQ_INTERFACE(IMirroredExternalSignalPrivate, IBaseObject)
{
    virtual void INTERFACE_FUNC assignDomainSignal(const SignalPtr& domainSignal) = 0;
};

class ConfigMirroredExternalSignalImpl final : public MirroredSignalBase<IMirroredExternalSignalPrivate>
{
public:
    using Super = MirroredSignalBase<IMirroredExternalSignalPrivate>;
    explicit ConfigMirroredExternalSignalImpl(const ContextPtr& ctx,
                                              const ComponentPtr& parent,
                                              const StringPtr& remoteGlobalId);

    StringPtr onGetRemoteId() const override;
    Bool onTriggerEvent(const EventPacketPtr& eventPacket) override;

    // IComponent
    ErrCode INTERFACE_FUNC getGlobalId(IString** globalId) override;
    ErrCode INTERFACE_FUNC getLocalId(IString** localId) override;

    // IMirroredExternalSignalPrivate
    void INTERFACE_FUNC assignDomainSignal(const SignalPtr& domainSignal) override;

    // ISerializable
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

    SignalPtr onGetDomainSignal() override;
    DataDescriptorPtr onGetDescriptor() override;

private:
    static StringPtr createLocalId(const StringPtr& remoteGlobalId);

    StringPtr remoteGlobalId;
};

}
