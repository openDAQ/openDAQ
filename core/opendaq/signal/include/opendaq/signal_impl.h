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
#include <opendaq/signal.h>
#include <opendaq/signal_events.h>
#include <opendaq/signal_config.h>
#include <opendaq/context_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/connection_ptr.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/signal_private_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <coretypes/string_ptr.h>
#include <opendaq/utility_sync.h>
#include <opendaq/packet_factory.h>
#include <opendaq/signal_events_ptr.h>
#include <coretypes/validation.h>
#include <opendaq/component_impl.h>
#include <opendaq/input_port_private_ptr.h>

#include <utility>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface, typename... Interfaces>
class SignalBase;

using SignalImpl = SignalBase<ISignalConfig>;

template <typename TInterface, typename... Interfaces>
class SignalBase : public ComponentImpl<TInterface, ISignalEvents, ISignalPrivate, Interfaces...>
{
public:
    using Super = ComponentImpl<TInterface, ISignalEvents, ISignalPrivate, Interfaces...>;
    using Self = SignalBase<TInterface, Interfaces...>;

    SignalBase(const ContextPtr& context,
               const ComponentPtr& parent,
               const StringPtr& localId,
               const StringPtr& className = nullptr,
               ComponentStandardProps propsMode = ComponentStandardProps::Add);
    SignalBase(const ContextPtr& context,
               DataDescriptorPtr descriptor,
               const ComponentPtr& parent,
               const StringPtr& localId,
               const StringPtr& className,
               ComponentStandardProps propsMode = ComponentStandardProps::Add);
    ~SignalBase() override;

    ErrCode INTERFACE_FUNC getPublic(Bool* isPublic) override;
    ErrCode INTERFACE_FUNC setPublic(Bool isPublic) override;

    ErrCode INTERFACE_FUNC getDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getDomainSignal(ISignal** signal) override;
    ErrCode INTERFACE_FUNC getRelatedSignals(IList** signals) override;
    ErrCode INTERFACE_FUNC getConnections(IList** connections) override;
    ErrCode INTERFACE_FUNC getStreamed(Bool* streamed) override;
    ErrCode INTERFACE_FUNC setStreamed(Bool streamed) override;
    ErrCode INTERFACE_FUNC getValue(IDataPacket** packet) override;

    // ISignalConfig
    ErrCode INTERFACE_FUNC setDescriptor(IDataDescriptor* descriptor) override;
    ErrCode INTERFACE_FUNC setDomainSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC setRelatedSignals(IList* signals) override;
    ErrCode INTERFACE_FUNC addRelatedSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC removeRelatedSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC clearRelatedSignals() override;
    ErrCode INTERFACE_FUNC sendPacket(IPacket* packet) override;

    // ISignalEvents
    ErrCode INTERFACE_FUNC listenerConnected(IConnection* connection) override;
    ErrCode INTERFACE_FUNC listenerDisconnected(IConnection* connection) override;
    ErrCode INTERFACE_FUNC domainSignalReferenceSet(ISignal* signal) override;
    ErrCode INTERFACE_FUNC domainSignalReferenceRemoved(ISignal* signal) override;

    // ISignalPrivate
    ErrCode INTERFACE_FUNC clearDomainSignalWithoutNotification() override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj);
protected:
    void serializeCustomObjectValues(const SerializerPtr& serializer) override;
    void updateObject(const SerializedObjectPtr& obj) override;
    int getSerializeFlags() override;

    virtual EventPacketPtr createDataDescriptorChangedEventPacket();
    virtual void onListenedStatusChanged(bool connected);

    void removed() override;

private:
    StringPtr name;
    bool isPublic{};
    DataDescriptorPtr dataDescriptor;
    std::vector<SignalPtr> relatedSignals;
    SignalPtr domainSignal;
    std::vector<ConnectionPtr> connections;
    std::vector<WeakRefPtr<ISignalConfig>> domainSignalReferences;
    DataPacketPtr lastDataPacket;

    void initSignalProperties(ComponentStandardProps propsMode);
    void propertyValueChanged(const PropertyPtr& prop, const BaseObjectPtr& value);
    bool sendPacketInternal(const PacketPtr& packet) const;
};

template <typename TInterface, typename... Interfaces>
SignalBase<TInterface, Interfaces...>::SignalBase(const ContextPtr& context,
                                      const ComponentPtr& parent,
                                      const StringPtr& localId,
                                      const StringPtr& className,
                                      const ComponentStandardProps propsMode)
    : SignalBase<TInterface, Interfaces...>(context, nullptr, parent, localId, className, propsMode)
{
}

template <typename TInterface, typename... Interfaces>
SignalBase<TInterface, Interfaces...>::SignalBase(const ContextPtr& context,
                                      DataDescriptorPtr descriptor,
                                      const ComponentPtr& parent,
                                      const StringPtr& localId,
                                      const StringPtr& className,
                                      const ComponentStandardProps propsMode)
    : Super(context, parent, localId, className, propsMode)
    , isPublic(true)
    , dataDescriptor(std::move(descriptor))
{
    initSignalProperties(propsMode);
}

template <typename TInterface, typename... Interfaces>
SignalBase<TInterface, Interfaces...>::~SignalBase()
{
    if (domainSignal.assigned())
        domainSignal.asPtr<ISignalEvents>().domainSignalReferenceRemoved(this->template borrowPtr<SignalPtr>());
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::initSignalProperties(const ComponentStandardProps propsMode)
{
    if (propsMode == ComponentStandardProps::Skip)
        return;

    auto objPtr = this->template borrowPtr<ComponentPtr>();

    if (objPtr.hasProperty("Name"))
    {
        objPtr.getOnPropertyValueWrite("Name") +=
            [this](PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args) { propertyValueChanged(args.getProperty(), args.getValue()); };
    }

    if (objPtr.hasProperty("Description"))
    {
        objPtr.getOnPropertyValueWrite("Description") +=
            [this](PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args) { propertyValueChanged(args.getProperty(), args.getValue()); };
    }
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::propertyValueChanged(const PropertyPtr& prop, const BaseObjectPtr& value)
{
    const auto packet = PropertyChangedEventPacket(prop.getName(), value);
    std::scoped_lock lock(this->sync);

    static_cast<void>(sendPacketInternal(packet));
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getPublic(Bool* isPublic)
{
    OPENDAQ_PARAM_NOT_NULL(isPublic);

    std::scoped_lock lock(this->sync);

    *isPublic = this->isPublic;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setPublic(Bool isPublic)
{
    std::scoped_lock lock(this->sync);

    this->isPublic = isPublic;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getDescriptor(IDataDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    std::scoped_lock lock(this->sync);

    *descriptor = dataDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
EventPacketPtr SignalBase<TInterface, Interfaces...>::createDataDescriptorChangedEventPacket()
{
    DataDescriptorPtr domainDataDescriptor;
    if (domainSignal.assigned())
        domainDataDescriptor = domainSignal.getDescriptor();

    EventPacketPtr packet = DataDescriptorChangedEventPacket(dataDescriptor, domainDataDescriptor);
    return packet;
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::onListenedStatusChanged(bool /*listened*/)
{
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setDescriptor(IDataDescriptor* descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    std::vector<SignalConfigPtr> valueSignalsOfDomainSignal;
    bool success;

    {
        std::scoped_lock lock(this->sync);

        dataDescriptor = descriptor;
        const auto packet = DataDescriptorChangedEventPacket(descriptor, nullptr);

        // Should this return a failure error code or execute all sendPacket calls and return one of the errors?
        success = sendPacketInternal(packet);
        if (success)
        {
            for (const auto& signal : domainSignalReferences)
            {
                const SignalConfigPtr signalPtr = signal.getRef();
                if (signalPtr.assigned())
                    valueSignalsOfDomainSignal.push_back(std::move(signalPtr));
            }
        }
    }

    if (!valueSignalsOfDomainSignal.empty())
    {
        const EventPacketPtr domainChangedPacket = DataDescriptorChangedEventPacket(nullptr, descriptor);
        for (const auto& sig : valueSignalsOfDomainSignal)
        {
            const auto err = sig->sendPacket(domainChangedPacket);
            success &= err == OPENDAQ_SUCCESS;
        }
    }

    return success
        ? OPENDAQ_SUCCESS
        : OPENDAQ_IGNORED;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getDomainSignal(ISignal** signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    std::scoped_lock lock(this->sync);

    *signal = domainSignal.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setDomainSignal(ISignal* signal)
{
    std::scoped_lock lock(this->sync);

    if (signal == domainSignal)
        return  OPENDAQ_IGNORED;

    if (domainSignal.assigned())
        domainSignal.asPtr<ISignalEvents>().domainSignalReferenceRemoved(this->template borrowPtr<SignalPtr>());

    domainSignal = signal;

    if (domainSignal.assigned())
        domainSignal.asPtr<ISignalEvents>().domainSignalReferenceSet(this->template borrowPtr<SignalPtr>());

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getRelatedSignals(IList** signals)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    std::scoped_lock lock(this->sync);

    ListPtr<ISignal> signalsPtr{relatedSignals};
    *signals = signalsPtr.detach();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setRelatedSignals(IList* signals)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    std::scoped_lock lock(this->sync);

    const auto signalsPtr = ListPtr<ISignal>::Borrow(signals);
    relatedSignals.clear();
    for (const auto& sig : signalsPtr)
        relatedSignals.push_back(sig);

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::addRelatedSignal(ISignal* signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    auto signalPtr = ObjectPtr(signal);

    std::scoped_lock lock(this->sync);
    const auto it = std::find(relatedSignals.begin(), relatedSignals.end(), signalPtr);
    if (it != relatedSignals.end())
        return OPENDAQ_ERR_DUPLICATEITEM;

    relatedSignals.push_back(std::move(signalPtr));
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::removeRelatedSignal(ISignal* signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    const auto signalPtr = ObjectPtr<ISignal>::Borrow(signal);

    std::scoped_lock lock(this->sync);
    auto it = std::find(relatedSignals.begin(), relatedSignals.end(), signalPtr);
    if (it == relatedSignals.end())
        return OPENDAQ_ERR_NOTFOUND;

    relatedSignals.erase(it);
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::clearRelatedSignals()
{
    std::scoped_lock lock(this->sync);
    relatedSignals.clear();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getConnections(IList** connections)
{
    OPENDAQ_PARAM_NOT_NULL(connections);

    std::scoped_lock lock(this->sync);

    ListPtr<IConnection> connectionsPtr{this->connections};
    *connections = connectionsPtr.detach();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::sendPacket(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    const auto packetPtr = PacketPtr::Borrow(packet);

    std::scoped_lock lock(this->sync);

    if (sendPacketInternal(packetPtr))
    {
        const auto dataPacket = packetPtr.asPtrOrNull<IDataPacket>();
        if (dataPacket.assigned() && dataPacket.getSampleCount())
            lastDataPacket = dataPacket;
        return OPENDAQ_SUCCESS;
    }

    return  OPENDAQ_IGNORED;
}

template <typename TInterface, typename... Interfaces>
bool SignalBase<TInterface, Interfaces...>::sendPacketInternal(const PacketPtr& packet) const
{
    if (!this->active)
        return false;

    for (auto& connection : connections)
        connection.enqueue(packet);

    return true;
}

#include <opendaq/event_packet_params.h>

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::listenerConnected(IConnection* connection)
{
    OPENDAQ_PARAM_NOT_NULL(connection);

    const auto connectionPtr = ConnectionPtr::Borrow(connection);

    std::scoped_lock lock(this->sync);
    auto it = std::find(connections.begin(), connections.end(), connectionPtr);
    if (it != connections.end())
        return OPENDAQ_ERR_DUPLICATEITEM;

    if (connections.empty())
    {
        const ErrCode errCode = wrapHandler(this, &Self::onListenedStatusChanged, true);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    connections.push_back(connectionPtr);

    const auto packet = createDataDescriptorChangedEventPacket();
    connectionPtr.enqueueOnThisThread(packet);

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::listenerDisconnected(IConnection* connection)
{
    OPENDAQ_PARAM_NOT_NULL(connection);

    const auto connectionPtr = ObjectPtr<IConnection>::Borrow(connection);

    std::scoped_lock lock(this->sync);
    auto it = std::find(connections.begin(), connections.end(), connectionPtr);
    if (it == connections.end())
        return OPENDAQ_ERR_NOTFOUND;

    connections.erase(it);

    if (connections.empty())
    {
        const ErrCode errCode = wrapHandler(this, &Self::onListenedStatusChanged, false);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::domainSignalReferenceSet(ISignal* signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    const auto signalPtr = SignalPtr::Borrow(signal).asPtrOrNull<ISignalConfig>(true);
    if (!signalPtr.assigned())
        return this->makeErrorInfo(OPENDAQ_ERR_NOINTERFACE, "Signal does not implement ISignalConfig interface.");

    std::scoped_lock lock(this->sync);
    for (const auto& refSignal : domainSignalReferences)
    {
        if (refSignal.getRef() == signalPtr)
            return OPENDAQ_ERR_DUPLICATEITEM;
    }

    domainSignalReferences.push_back(WeakRefPtr<ISignalConfig>(signal));
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::domainSignalReferenceRemoved(ISignal* signal)
{
    std::scoped_lock lock(this->sync);

    const auto signalPtr = SignalPtr::Borrow(signal).asPtrOrNull<ISignalConfig>(true);
    if (!signalPtr.assigned())
        return this->makeErrorInfo(OPENDAQ_ERR_NOINTERFACE, "Signal does not implement ISignalConfig interface.");

    for (auto it = begin(domainSignalReferences); it != end(domainSignalReferences); ++it)
    {
        auto sig = it->getRef();
        if (sig.assigned() && signalPtr == sig)
        {
            domainSignalReferences.erase(it);
            return OPENDAQ_SUCCESS;
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::clearDomainSignalWithoutNotification()
{
    std::scoped_lock lock(this->sync);

    domainSignal = nullptr;

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode INTERFACE_FUNC SignalBase<TInterface, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ConstCharPtr SignalBase<TInterface, Interfaces...>::SerializeId()
{
    return "Signal";
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::serializeCustomObjectValues(const SerializerPtr& serializer)
{
    if (domainSignal.assigned())
    {
        serializer->key("domainSignalId");
        auto domainSignalGlobalId = this->getRelativeGlobalId(domainSignal.getGlobalId());
        serializer->writeString(domainSignalGlobalId.c_str(), domainSignalGlobalId.size());
    }

    Super::serializeCustomObjectValues(serializer);
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::updateObject(const SerializedObjectPtr& obj)
{
    Super::updateObject(obj);
}


template <typename TInterface, typename... Interfaces>
int SignalBase<TInterface, Interfaces...>::getSerializeFlags()
{
    return ComponentSerializeFlag_SerializeActiveProp;
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::removed()
{
    for (auto& connection : connections)
    {
        auto inputPort = connection.getInputPort();
        if (inputPort.assigned())
        {
            auto inputPortPrivate = inputPort.template asPtrOrNull<IInputPortPrivate>(true);
            if (inputPortPrivate.assigned())
                inputPortPrivate.disconnectWithoutSignalNotification();
        }
    }

    connections.clear();

    for (auto it = begin(domainSignalReferences); it != end(domainSignalReferences); ++it)
    {
        auto sig = it->getRef();
        if (sig.assigned())
        {
            auto sigPrivate = sig.template asPtrOrNull<ISignalPrivate>(true);
            if (sigPrivate.assigned())
                sigPrivate.clearDomainSignalWithoutNotification();
        }

    }

    domainSignalReferences.clear();
    relatedSignals.clear();
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getStreamed(Bool* streamed)
{
    OPENDAQ_PARAM_NOT_NULL(streamed);

    *streamed = False;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setStreamed(Bool streamed)
{
    return OPENDAQ_IGNORED;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getValue(IDataPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);
    std::scoped_lock lock(this->sync);

    if (lastDataPacket.assigned() && lastDataPacket.getSampleCount() > 1)
    {
        auto dataPacket = DataPacket(lastDataPacket.getDataDescriptor(), 1);

        auto memSize = lastDataPacket.getSampleMemSize();
        auto idx = lastDataPacket.getSampleCount() - 1;
        int8_t* src = static_cast<int8_t*>(lastDataPacket.getData()) + memSize * idx;
        int8_t* dst = static_cast<int8_t*>(dataPacket.getData());
        memcpy(dst, src, memSize);

        lastDataPacket = dataPacket;
    }

    *packet = lastDataPacket.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
