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
               DataDescriptorPtr descriptor,
               const ComponentPtr& parent,
               const StringPtr& localId,
               const StringPtr& className = nullptr);

    ~SignalBase() override;

    ErrCode INTERFACE_FUNC getPublic(Bool* isPublic) override;
    ErrCode INTERFACE_FUNC setPublic(Bool isPublic) override;

    ErrCode INTERFACE_FUNC getDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getDomainSignal(ISignal** signal) override;
    ErrCode INTERFACE_FUNC getRelatedSignals(IList** signals) override;
    ErrCode INTERFACE_FUNC getConnections(IList** connections) override;
    ErrCode INTERFACE_FUNC getStreamed(Bool* streamed) override;
    ErrCode INTERFACE_FUNC setStreamed(Bool streamed) override;
    
    // IComponent
    ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC setDescription(IString* description) override;
    ErrCode INTERFACE_FUNC setVisible(Bool visible) override;

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

    void propertyValueChanged(const PropertyPtr& prop, const BaseObjectPtr& value);
    void sendAattributeChangedPacket(const StringPtr& attributeName, const BaseObjectPtr& value);
    bool sendPacketInternal(const PacketPtr& packet) const;
    void triggerRelatedSignalsChanged();
};

template <typename TInterface, typename... Interfaces>
SignalBase<TInterface, Interfaces...>::SignalBase(const ContextPtr& context,
                                      DataDescriptorPtr descriptor,
                                      const ComponentPtr& parent,
                                      const StringPtr& localId,
                                      const StringPtr& className)
    : Super(context, parent, localId, className)
    , isPublic(true)
    , dataDescriptor(std::move(descriptor))
{
}

template <typename TInterface, typename... Interfaces>
SignalBase<TInterface, Interfaces...>::~SignalBase()
{
    if (domainSignal.assigned())
        domainSignal.asPtr<ISignalEvents>().domainSignalReferenceRemoved(this->template borrowPtr<SignalPtr>());
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::propertyValueChanged(const PropertyPtr& prop, const BaseObjectPtr& value)
{
    const auto packet = PropertyChangedEventPacket(prop.getName(), value);
    std::scoped_lock lock(this->sync);

    static_cast<void>(sendPacketInternal(packet));
}

template <typename TInterface, typename ... Interfaces>
void SignalBase<TInterface, Interfaces...>::sendAattributeChangedPacket(const StringPtr& attributeName, const BaseObjectPtr& value)
{
    const auto packet = AttributeChangedEventPacket(attributeName, value);
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
    {
        std::scoped_lock lock(this->sync);

        this->isPublic = isPublic;
        sendAattributeChangedPacket("Public", isPublic);
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                core_event_ids::AttributeChanged, Dict<IString, IBaseObject>({{"AttributeName", "Public"}, {"Public", this->isPublic}}));
        
        this->triggerCoreEvent(args);
    }
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

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                core_event_ids::DataDescriptorChanged,
                Dict<IString, IBaseObject>({{"DataDescriptor", dataDescriptor}}));
        
        this->triggerCoreEvent(args);
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
    {
        std::scoped_lock lock(this->sync);

        if (signal == domainSignal)
            return OPENDAQ_IGNORED;

        if (domainSignal.assigned())
            domainSignal.asPtr<ISignalEvents>().domainSignalReferenceRemoved(this->template borrowPtr<SignalPtr>());

        domainSignal = signal;

        if (domainSignal.assigned())
            domainSignal.asPtr<ISignalEvents>().domainSignalReferenceSet(this->template borrowPtr<SignalPtr>());

        // TODO: Enable this once supported in streaming
        // sendAattributeChangedPacket("DomainSignal", domainSignal);
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                core_event_ids::AttributeChanged,
                Dict<IString, IBaseObject>({{"AttributeName", "DomainSignal"}, {"DomainSignal", domainSignal}}));
        
        this->triggerCoreEvent(args);
    }

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

    {
        std::scoped_lock lock(this->sync);

        const auto signalsPtr = ListPtr<ISignal>::Borrow(signals);
        relatedSignals.clear();
        for (const auto& sig : signalsPtr)
            relatedSignals.push_back(sig);

        // TODO: Enable this once supported in streaming
        // sendAattributeChangedPacket("RelatedSignals", relatedSignals);
    }

    triggerRelatedSignalsChanged();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::addRelatedSignal(ISignal* signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    auto signalPtr = ObjectPtr(signal);

    {
        std::scoped_lock lock(this->sync);
        const auto it = std::find(relatedSignals.begin(), relatedSignals.end(), signalPtr);
        if (it != relatedSignals.end())
            return OPENDAQ_ERR_DUPLICATEITEM;

        relatedSignals.push_back(std::move(signalPtr));

        // TODO: Enable this once supported in streaming
        // sendAattributeChangedPacket("RelatedSignals", relatedSignals);
    }

    triggerRelatedSignalsChanged();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::removeRelatedSignal(ISignal* signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    const auto signalPtr = ObjectPtr<ISignal>::Borrow(signal);

    {
        std::scoped_lock lock(this->sync);
        auto it = std::find(relatedSignals.begin(), relatedSignals.end(), signalPtr);
        if (it == relatedSignals.end())
            return OPENDAQ_ERR_NOTFOUND;

        relatedSignals.erase(it);

        
        // TODO: Enable this once supported in streaming
        // sendAattributeChangedPacket("RelatedSignals", relatedSignals);
    }

    triggerRelatedSignalsChanged();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::clearRelatedSignals()
{
    {
        std::scoped_lock lock(this->sync);
        relatedSignals.clear();
        
        // TODO: Enable this once supported in streaming
        // sendAattributeChangedPacket("RelatedSignals", relatedSignals);
    }
    
    triggerRelatedSignalsChanged();
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
        return OPENDAQ_SUCCESS;

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

template <typename TInterface, typename ... Interfaces>
void SignalBase<TInterface, Interfaces...>::triggerRelatedSignalsChanged()
{
    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        ListPtr<ISignal> sigs = List<ISignal>();
        for (const auto& sig : relatedSignals)
            sigs.pushBack(sig);

        const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                core_event_ids::AttributeChanged,
                Dict<IString, IBaseObject>({{"AttributeName", "RelatedSignals"}, {"RelatedSignals", sigs}}));
        
        this->triggerCoreEvent(args);
    }
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

template <typename TInterface, typename ... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setActive(Bool active)
{
    const ErrCode err = Super::setActive(active);
    if (OPENDAQ_FAILED(err) || err == OPENDAQ_IGNORED)
        return err;
    
    std::scoped_lock lock(this->sync);
    sendAattributeChangedPacket("Active", active);
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setName(IString* name)
{
    const ErrCode err = Super::setName(name);
    if (OPENDAQ_FAILED(err) || err == OPENDAQ_IGNORED)
        return err;
    
    std::scoped_lock lock(this->sync);
    sendAattributeChangedPacket("Name", name);
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setDescription(IString* description)
{
    const ErrCode err = Super::setDescription(description);
    if (OPENDAQ_FAILED(err) || err == OPENDAQ_IGNORED)
        return err;
    
    std::scoped_lock lock(this->sync);
    sendAattributeChangedPacket("Description", description);
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setVisible(Bool visible)
{
    const ErrCode err = Super::setVisible(visible);
    if (OPENDAQ_FAILED(err) || err == OPENDAQ_IGNORED)
        return err;
    
    std::scoped_lock lock(this->sync);
    sendAattributeChangedPacket("Visible", visible);
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
