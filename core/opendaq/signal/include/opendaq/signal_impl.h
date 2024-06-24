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
#include <opendaq/packet_factory.h>
#include <opendaq/signal_events_ptr.h>
#include <coretypes/validation.h>
#include <opendaq/component_impl.h>
#include <opendaq/input_port_private_ptr.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

// https://developercommunity.visualstudio.com/t/inline-static-destructors-are-called-multiple-time/1157794
#ifdef _MSC_VER
#if _MSC_VER <= 1927
#define WORKAROUND_MEMBER_INLINE_VARIABLE
#endif
#endif

#define SIGNAL_AVAILABLE_ATTRIBUTES {"Public", "DomainSignal", "RelatedSignals"}

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
    ErrCode INTERFACE_FUNC getLastValue(IBaseObject** value) override;

    // ISignalConfig
    ErrCode INTERFACE_FUNC setDescriptor(IDataDescriptor* descriptor) override;
    ErrCode INTERFACE_FUNC setDomainSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC setRelatedSignals(IList* signals) override;
    ErrCode INTERFACE_FUNC addRelatedSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC removeRelatedSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC clearRelatedSignals() override;
    ErrCode INTERFACE_FUNC sendPacket(IPacket* packet) override;
    ErrCode INTERFACE_FUNC sendPackets(IList* packets) override;
    ErrCode INTERFACE_FUNC sendPacketAndStealRef(IPacket* packet) override;
    ErrCode INTERFACE_FUNC sendPacketsAndStealRef(IList* packet) override;

    // ISignalEvents
    ErrCode INTERFACE_FUNC listenerConnected(IConnection* connection) override;
    ErrCode INTERFACE_FUNC listenerDisconnected(IConnection* connection) override;
    ErrCode INTERFACE_FUNC domainSignalReferenceSet(ISignal* signal) override;
    ErrCode INTERFACE_FUNC domainSignalReferenceRemoved(ISignal* signal) override;

    // ISignalPrivate
    ErrCode INTERFACE_FUNC clearDomainSignalWithoutNotification() override;
    ErrCode INTERFACE_FUNC enableKeepLastValue(Bool enabled) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
protected:
    void visibleChanged() override;

    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override;
    void updateObject(const SerializedObjectPtr& obj) override;
    int getSerializeFlags() override;

    virtual EventPacketPtr createDataDescriptorChangedEventPacket();
    virtual void onListenedStatusChanged(bool listened);
    virtual SignalPtr onGetDomainSignal();
    virtual DataDescriptorPtr onGetDescriptor();

    void removed() override;
    BaseObjectPtr getDeserializedParameter(const StringPtr& parameter) override;
    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

    ErrCode lockAllAttributesInternal() override;
    
#ifdef WORKAROUND_MEMBER_INLINE_VARIABLE
    static std::unordered_set<std::string> signalAvailableAttributes;
#else
    inline static std::unordered_set<std::string> signalAvailableAttributes = SIGNAL_AVAILABLE_ATTRIBUTES;
#endif

    DataDescriptorPtr dataDescriptor;
    StringPtr deserializedDomainSignalId;
    DataPacketPtr lastDataPacket;

private:
    bool isPublic{};
    std::vector<SignalPtr> relatedSignals;
    SignalPtr domainSignal;
    std::vector<ConnectionPtr> connections;
    std::vector<ConnectionPtr> remoteConnections;
    std::vector<WeakRefPtr<ISignalConfig>> domainSignalReferences;
    bool keepLastPacket;
    bool keepLastValue;

    bool sendPacketInternal(const PacketPtr& packet, bool ignoreActive = false) const;
    bool sendPacketInternal(PacketPtr&& packet, bool ignoreActive = false) const;
    void triggerRelatedSignalsChanged();
    void disconnectInputPort(const ConnectionPtr& connection);
    void clearConnections(std::vector<ConnectionPtr>& connections);
    void setKeepLastPacket();
    TypePtr addToTypeManagerRecursively(const TypeManagerPtr& typeManager,
                                        const DataDescriptorPtr& descriptor) const;
    void buildTempConnections(std::vector<ConnectionPtr>& tempConnections);
    void checkKeepLastPacket(const PacketPtr& packet);
    void enqueuePacketToConnections(const PacketPtr& packet, const std::vector<ConnectionPtr>& tempConnections);
    void enqueuePacketToConnections(PacketPtr&& packet, const std::vector<ConnectionPtr>& tempConnections);
    void enqueuePacketsToConnections(const ListPtr<IPacket>& packets, const std::vector<ConnectionPtr>& tempConnections);
    void enqueuePacketsToConnections(ListPtr<IPacket>&& packets, const std::vector<ConnectionPtr>& tempConnections);

    template <class Packet>
    bool keepLastPacketAndEnqueue(Packet&& packet);

    template <class ListOfPackets>
    bool keepLastPacketAndEnqueueMultiple(ListOfPackets&& packets);
};

#ifdef WORKAROUND_MEMBER_INLINE_VARIABLE
template <typename TInterface, typename... Interfaces>
std::unordered_set<std::string> SignalBase<TInterface, Interfaces...>::signalAvailableAttributes = SIGNAL_AVAILABLE_ATTRIBUTES;
#endif

template <typename TInterface, typename... Interfaces>
SignalBase<TInterface, Interfaces...>::SignalBase(const ContextPtr& context,
                                      DataDescriptorPtr descriptor,
                                      const ComponentPtr& parent,
                                      const StringPtr& localId,
                                      const StringPtr& className)
    : Super(context, parent, localId, className)
    , dataDescriptor(std::move(descriptor))
    , isPublic(true)
    , keepLastValue(true)
{
    setKeepLastPacket();
}

template <typename TInterface, typename... Interfaces>
SignalBase<TInterface, Interfaces...>::~SignalBase()
{
    if (domainSignal.assigned())
        domainSignal.asPtr<ISignalEvents>().domainSignalReferenceRemoved(this->template borrowPtr<SignalPtr>());
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
    if (this->frozen)
        return OPENDAQ_ERR_FROZEN;

    {
        std::scoped_lock lock(this->sync);

        if (this->lockedAttributes.count("Public"))
        {
            if (this->context.assigned() && this->context.getLogger().assigned())
            {
                const auto loggerComponent = this->context.getLogger().getOrAddComponent("Component");
                StringPtr descObj;
                this->getName(&descObj);
                LOG_I("Active attribute of {} is locked", descObj);
            }

            return OPENDAQ_IGNORED;
        }

        this->isPublic = isPublic;
        setKeepLastPacket();
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                CoreEventId::AttributeChanged, Dict<IString, IBaseObject>({{"AttributeName", "Public"}, {"Public", this->isPublic}}));
        
        this->triggerCoreEvent(args);
    }
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getDescriptor(IDataDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    std::scoped_lock lock(this->sync);
    
    DataDescriptorPtr dataDescriptorPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetDescriptor, dataDescriptorPtr);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    *descriptor = dataDescriptorPtr.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
EventPacketPtr SignalBase<TInterface, Interfaces...>::createDataDescriptorChangedEventPacket()
{
    const SignalPtr domainSignalObj = onGetDomainSignal();

    DataDescriptorPtr domainDataDescriptor;
    if (domainSignalObj.assigned())
        domainDataDescriptor = domainSignalObj.getDescriptor();

    const DataDescriptorPtr dataDescriptorObj = onGetDescriptor();

    EventPacketPtr packet = DataDescriptorChangedEventPacket(dataDescriptorObj, domainDataDescriptor);
    return packet;
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::onListenedStatusChanged(bool /*listened*/)
{
}

template <typename TInterface, typename... Interfaces>
inline TypePtr SignalBase<TInterface, Interfaces...>::addToTypeManagerRecursively(const TypeManagerPtr& typeManager,
                                                                                  const DataDescriptorPtr& descriptor) const
{
    const auto name = descriptor.getName();
    if (!name.assigned())
        throw NotAssignedException{"Name of data descriptor not assigned."};

    const auto fields = descriptor.getStructFields();
    auto fieldNames = List<IString>();
    auto fieldTypes = List<IType>();

    if (fields.assigned())
    {
        for (auto const& field : fields)
        {
            const auto dimensions = field.getDimensions();

            if (!dimensions.assigned())
                throw NotAssignedException{"Dimensions of data descriptor not assigned."};

            const auto dimensionCount = dimensions.getCount();

            if (dimensionCount > 1)
                throw NotSupportedException{"getLastValue on signals with dimensions supports only up to one dimension."};

            TypePtr type;

            switch (field.getSampleType())
            {
                case SampleType::Float32:
                case SampleType::Float64:
                    type = SimpleType(CoreType::ctFloat);
                    break;
                case SampleType::Int8:
                case SampleType::UInt8:
                case SampleType::Int16:
                case SampleType::UInt16:
                case SampleType::Int32:
                case SampleType::UInt32:
                case SampleType::Int64:
                case SampleType::UInt64:
                    type = SimpleType(CoreType::ctInt);
                    break;
                case SampleType::ComplexFloat32:
                case SampleType::ComplexFloat64:
                    type = SimpleType(CoreType::ctComplexNumber);
                    break;
                case SampleType::Struct:
                    // Recursion
                    type = addToTypeManagerRecursively(typeManager, field);
                    break;
                default:
                    type = SimpleType(CoreType::ctUndefined);
            }

            // Handle list
            if (dimensionCount == 1)
                type = SimpleType(CoreType::ctList);

            fieldNames.pushBack(field.getName());
            fieldTypes.pushBack(type);
        }
    }

    const auto structType = StructType(name, fieldNames, fieldTypes);

    try
    {
        typeManager.addType(structType);
    }
    catch (const std::exception& e)
    {
        const auto loggerComponent = this->context.getLogger().getOrAddComponent("Signal");
        LOG_W("Couldn't add type {} to type manager: {}", structType.getName(), e.what());
    }
    catch (...)
    {
        const auto loggerComponent = this->context.getLogger().getOrAddComponent("Signal");
        LOG_W("Couldn't add type {} to type manager!", structType.getName());
    }

    return structType;
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
        success = sendPacketInternal(packet, true);
        if (success)
        {
            for (const auto& signal : domainSignalReferences)
            {
                const SignalConfigPtr signalPtr = signal.getRef();
                if (signalPtr.assigned())
                    valueSignalsOfDomainSignal.push_back(std::move(signalPtr));
            }
            try {
                if (dataDescriptor.getSampleType() == SampleType::Struct)
                {
                    auto typeManager = this->context.getTypeManager();
                    addToTypeManagerRecursively(typeManager, dataDescriptor);
                }
            }
            catch (const std::exception& e)
            {
                const auto loggerComponent = this->context.getLogger().getOrAddComponent("Signal");
                LOG_W("There was an exception in setDescriptor method: {}", e.what());
            }
            catch (...)
            {
                const auto loggerComponent = this->context.getLogger().getOrAddComponent("Signal");
                LOG_W("There was an exception in setDescriptor method!");
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
            CoreEventId::DataDescriptorChanged, Dict<IString, IBaseObject>({{"DataDescriptor", dataDescriptor}}));

        this->triggerCoreEvent(args);
    }

    return success ? OPENDAQ_SUCCESS : OPENDAQ_IGNORED;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getDomainSignal(ISignal** signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    std::scoped_lock lock(this->sync);

    SignalPtr signalPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetDomainSignal, signalPtr);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    *signal = signalPtr.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setDomainSignal(ISignal* signal)
{
    {
        std::scoped_lock lock(this->sync);
        
        if (this->lockedAttributes.count("DomainSignal"))
        {
            if (this->context.assigned() && this->context.getLogger().assigned())
            {
                const auto loggerComponent = this->context.getLogger().getOrAddComponent("Component");
                StringPtr descObj;
                this->getName(&descObj);
                LOG_I("Domain Signal attribute of {} is locked", descObj);
            }

            return OPENDAQ_IGNORED;
        }

        if (signal == domainSignal)
            return OPENDAQ_IGNORED;

        if (domainSignal.assigned())
            domainSignal.asPtr<ISignalEvents>().domainSignalReferenceRemoved(this->template borrowPtr<SignalPtr>());

        domainSignal = signal;

        if (domainSignal.assigned())
            domainSignal.asPtr<ISignalEvents>().domainSignalReferenceSet(this->template borrowPtr<SignalPtr>());
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                CoreEventId::AttributeChanged,
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

        if (this->lockedAttributes.count("RelatedSignals"))
        {
            if (this->context.assigned() && this->context.getLogger().assigned())
            {
                const auto loggerComponent = this->context.getLogger().getOrAddComponent("Component");
                StringPtr descObj;
                this->getName(&descObj);
                LOG_I("Related Signals attribute of {} is locked", descObj);
            }

            return OPENDAQ_IGNORED;
        }

        const auto signalsPtr = ListPtr<ISignal>::Borrow(signals);
        relatedSignals.clear();
        for (const auto& sig : signalsPtr)
            relatedSignals.push_back(sig);
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

        if (this->lockedAttributes.count("RelatedSignals"))
        {
            if (this->context.assigned() && this->context.getLogger().assigned())
            {
                const auto loggerComponent = this->context.getLogger().getOrAddComponent("Component");
                StringPtr descObj;
                this->getName(&descObj);
                LOG_I("Related Signals attribute of {} is locked", descObj);
            }

            return OPENDAQ_IGNORED;
        }

        const auto it = std::find(relatedSignals.begin(), relatedSignals.end(), signalPtr);
        if (it != relatedSignals.end())
            return OPENDAQ_ERR_DUPLICATEITEM;

        relatedSignals.push_back(std::move(signalPtr));
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

        if (this->lockedAttributes.count("RelatedSignals"))
        {
            if (this->context.assigned() && this->context.getLogger().assigned())
            {
                const auto loggerComponent = this->context.getLogger().getOrAddComponent("Component");
                StringPtr descObj;
                this->getName(&descObj);
                LOG_I("Related Signals attribute of {} is locked", descObj);
            }

            return OPENDAQ_IGNORED;
        }

        auto it = std::find(relatedSignals.begin(), relatedSignals.end(), signalPtr);
        if (it == relatedSignals.end())
            return OPENDAQ_ERR_NOTFOUND;

        relatedSignals.erase(it);
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
    }
    
    triggerRelatedSignalsChanged();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getConnections(IList** connections)
{
    OPENDAQ_PARAM_NOT_NULL(connections);

    std::scoped_lock lock(this->sync);

    auto connectionsPtr = List<IConnection>();
    for (const auto& conn : this->connections)
        connectionsPtr.pushBack(conn);
    for (const auto& conn : this->remoteConnections)
        connectionsPtr.pushBack(conn);

    *connections = connectionsPtr.detach();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::buildTempConnections(std::vector<ConnectionPtr>& tempConnections)
{
    tempConnections.reserve(connections.size());
    for (const auto& connection : connections)
        tempConnections.push_back(connection);
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::checkKeepLastPacket(const PacketPtr& packet)
{
    if (keepLastPacket)
    {
        auto dataPacket = packet.asPtrOrNull<IDataPacket>();
        if (dataPacket.assigned() && dataPacket.getSampleCount() > 0)
            lastDataPacket = std::move(dataPacket);
    }
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::enqueuePacketToConnections(const PacketPtr& packet, const std::vector<ConnectionPtr>& tempConnections)
{
    for (const auto& connection : tempConnections)
        connection.enqueue(packet);
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::enqueuePacketToConnections(PacketPtr&& packet, const std::vector<ConnectionPtr>& tempConnections)
{
    if (tempConnections.empty())
        return;

    auto startIt = tempConnections.begin();
    const auto endIt = std::prev(tempConnections.end());

    while (startIt != endIt)
        startIt++->enqueue(packet);

    startIt->enqueue(std::move(packet));
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::enqueuePacketsToConnections(
    const ListPtr<IPacket>& packets,
    const std::vector<ConnectionPtr>& tempConnections)
{
    for (const auto& connection : tempConnections)
        connection.enqueueMultiple(packets);
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::enqueuePacketsToConnections(
    ListPtr<IPacket>&& packets,
    const std::vector<ConnectionPtr>& tempConnections)
{
    if (tempConnections.empty())
        return;

    auto startIt = tempConnections.begin();
    const auto endIt = std::prev(tempConnections.end());

    while (startIt != endIt)
        startIt++->enqueueMultiple(packets);

    (*startIt)->enqueueMultipleAndStealRef(packets.detach());
}

template <typename TInterface, typename... Interfaces>
template <class Packet>
bool SignalBase<TInterface, Interfaces...>::keepLastPacketAndEnqueue(Packet&& packet)
{
    std::vector<ConnectionPtr> tempConnections;

    {
        std::scoped_lock lock(this->sync);

        if (!this->active)
            return false;

        checkKeepLastPacket(packet);
        buildTempConnections(tempConnections);
    }

    enqueuePacketToConnections(std::forward<Packet>(packet), tempConnections);

    return true;
}

template <typename TInterface, typename... Interfaces>
template <class ListOfPackets>
bool SignalBase<TInterface, Interfaces...>::keepLastPacketAndEnqueueMultiple(ListOfPackets&& packets)
{
    std::vector<ConnectionPtr> tempConnections;

    {
        size_t cnt = packets.getCount();

        std::scoped_lock lock(this->sync);

        if (!this->active || cnt == 0)
            return false;

        checkKeepLastPacket(packets[cnt - 1]);
        buildTempConnections(tempConnections);
    }

    enqueuePacketsToConnections(std::forward<ListOfPackets>(packets), tempConnections);

    return true;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::sendPacket(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    const auto packetPtr = PacketPtr::Borrow(packet);

    return daqTry(
        [this, &packetPtr]
        {
            if (!keepLastPacketAndEnqueue(packetPtr))
                return OPENDAQ_IGNORED;

            return OPENDAQ_SUCCESS;
        });
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::sendPacketAndStealRef(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    auto packetPtr = PacketPtr::Adopt(packet);

    return daqTry(
        [this, packet = std::move(packetPtr)] () mutable
        {
            if (!keepLastPacketAndEnqueue(std::move(packet)))
                return OPENDAQ_IGNORED;

            return OPENDAQ_SUCCESS;
        });
}

template <typename TInterface, typename... Interfaces>
ErrCode INTERFACE_FUNC SignalBase<TInterface, Interfaces...>::sendPackets(IList* packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    const auto packetsPtr = ListPtr<IPacket>::Borrow(packets);

    return daqTry([&packetsPtr, this]
        {
            if (!keepLastPacketAndEnqueueMultiple(packetsPtr))
                return OPENDAQ_IGNORED;

            return OPENDAQ_SUCCESS;
        });    
}

template <typename TInterface, typename... Interfaces>
ErrCode INTERFACE_FUNC SignalBase<TInterface, Interfaces...>::sendPacketsAndStealRef(IList* packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    auto packetsPtr = ListPtr<IPacket>::Adopt(packets);

    return daqTry(
        [this, packets = std::move(packetsPtr)] () mutable
        {
            if (!keepLastPacketAndEnqueueMultiple(std::move(packets)))
                return OPENDAQ_IGNORED;

            return OPENDAQ_SUCCESS;
        });
}

template <typename TInterface, typename... Interfaces>
bool SignalBase<TInterface, Interfaces...>::sendPacketInternal(const PacketPtr& packet, bool ignoreActive) const
{
    if (!ignoreActive && !this->active)
        return false;

    for (auto& connection : connections)
        connection.enqueue(packet);

    return true;
}

template <typename TInterface, typename... Interfaces>
bool SignalBase<TInterface, Interfaces...>::sendPacketInternal(PacketPtr&& packet, bool ignoreActive) const
{
    if (!ignoreActive && !this->active)
        return false;

    if (connections.empty())
        return true;

    auto startIt = connections.begin();
    const auto endIt = std::prev(connections.end());

    while (startIt != endIt)
        startIt++->enqueue(packet);

    startIt->enqueue(std::move(packet));

    return true;
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::triggerRelatedSignalsChanged()
{
    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        ListPtr<ISignal> sigs = List<ISignal>();
        for (const auto& sig : relatedSignals)
            sigs.pushBack(sig);

        const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                CoreEventId::AttributeChanged,
                Dict<IString, IBaseObject>({{"AttributeName", "RelatedSignals"}, {"RelatedSignals", sigs}}));
        
        this->triggerCoreEvent(args);
    }
}

template <typename TInterface, typename... Interfaces>
SignalPtr SignalBase<TInterface, Interfaces...>::onGetDomainSignal()
{
    return domainSignal;
}

template <typename TInterface, typename ... Interfaces>
DataDescriptorPtr SignalBase<TInterface, Interfaces...>::onGetDescriptor()
{
    return dataDescriptor;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::listenerConnected(IConnection* connection)
{
    OPENDAQ_PARAM_NOT_NULL(connection);

    const auto connectionPtr = ConnectionPtr::Borrow(connection);

    std::scoped_lock lock(this->sync);

    if (connectionPtr.isRemote())
    {
        const auto it = std::find(remoteConnections.begin(), remoteConnections.end(), connectionPtr);
        if (it != remoteConnections.end())
            return OPENDAQ_ERR_DUPLICATEITEM;

        remoteConnections.push_back(connectionPtr);
        return OPENDAQ_SUCCESS;
    }

    const auto it = std::find(connections.begin(), connections.end(), connectionPtr);
    if (it != connections.end())
        return OPENDAQ_ERR_DUPLICATEITEM;
    
    const auto packet = createDataDescriptorChangedEventPacket();

    if (connections.empty())
    {
        const ErrCode errCode = wrapHandler(this, &Self::onListenedStatusChanged, true);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    connections.push_back(connectionPtr);

    connectionPtr.enqueueOnThisThread(packet);

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::listenerDisconnected(IConnection* connection)
{
    OPENDAQ_PARAM_NOT_NULL(connection);

    const auto connectionPtr = ObjectPtr<IConnection>::Borrow(connection);

    std::scoped_lock lock(this->sync);

    if (connectionPtr.isRemote())
    {
        const auto it = std::find(remoteConnections.begin(), remoteConnections.end(), connectionPtr);
        if (it == remoteConnections.end())
            return OPENDAQ_ERR_NOTFOUND;

        remoteConnections.erase(it);
        return OPENDAQ_SUCCESS;
    }

    const auto it = std::find(connections.begin(), connections.end(), connectionPtr);
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
ErrCode SignalBase<TInterface, Interfaces...>::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);
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
                           return createWithImplementation<ISignalConfig, SignalImpl>(
                               deserializeContext.getContext(), nullptr, deserializeContext.getParent(), deserializeContext.getLocalId(), className);
                       }).detach();
        });
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate)
{
    if (domainSignal.assigned())
    {
        serializer.key("DomainSignalId");
        const auto domainSignalGlobalId = domainSignal.getGlobalId();
        serializer.writeString(domainSignalGlobalId);
    }

    if (dataDescriptor.assigned())
    {
        serializer.key("DataDescriptor");
        dataDescriptor.serialize(serializer);
    }

    serializer.key("Public");
    serializer.writeBool(isPublic);

    Super::serializeCustomObjectValues(serializer, forUpdate);
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::updateObject(const SerializedObjectPtr& obj)
{
    if (obj.hasKey("Public"))
        isPublic = obj.readBool("Public");

    Super::updateObject(obj);
}


template <typename TInterface, typename... Interfaces>
int SignalBase<TInterface, Interfaces...>::getSerializeFlags()
{
    return ComponentSerializeFlag_SerializeActiveProp;
}

template <typename TInterface, typename ... Interfaces>
void SignalBase<TInterface, Interfaces...>::disconnectInputPort(const ConnectionPtr& connection)
{
    const auto inputPort = connection.getInputPort();
    if (inputPort.assigned())
    {
        const auto inputPortPrivate = inputPort.template asPtrOrNull<IInputPortPrivate>(true);
        if (inputPortPrivate.assigned())
            inputPortPrivate.disconnectWithoutSignalNotification();
    }
}

template <typename TInterface, typename ... Interfaces>
void SignalBase<TInterface, Interfaces...>::clearConnections(std::vector<ConnectionPtr>& connections)
{
    for (auto& connection : connections)
        disconnectInputPort(connection);
    connections.clear();
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::removed()
{
    clearConnections(connections);
    clearConnections(remoteConnections);

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
BaseObjectPtr SignalBase<TInterface, Interfaces...>::getDeserializedParameter(const StringPtr& parameter)
{
    if (parameter == "DomainSignalId")
        return deserializedDomainSignalId;

    throw NotFoundException();
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                                                    const BaseObjectPtr& context,
                                                                    const FunctionPtr& factoryCallback)
{
    Super::deserializeCustomObjectValues(serializedObject, context, factoryCallback);
    if (serializedObject.hasKey("DomainSignalId"))
        deserializedDomainSignalId = serializedObject.readString("DomainSignalId");
    if (serializedObject.hasKey("DataDescriptor"))
        dataDescriptor = serializedObject.readObject("DataDescriptor", context, factoryCallback);
    if (serializedObject.hasKey("Public"))
        isPublic = serializedObject.readBool("Public");
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
ErrCode SignalBase<TInterface, Interfaces...>::lockAllAttributesInternal()
{
    for (const auto& str : this->signalAvailableAttributes)
        this->lockedAttributes.insert(str);

    return Super::lockAllAttributesInternal();
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::enableKeepLastValue(Bool enabled)
{
    std::scoped_lock lock(this->sync);
    keepLastValue = enabled;

    setKeepLastPacket();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::setKeepLastPacket()
{
    keepLastPacket = keepLastValue && isPublic && this->visible;

    if (!keepLastPacket)
        lastDataPacket = nullptr;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getLastValue(IBaseObject ** value)
{
    OPENDAQ_PARAM_NOT_NULL(value);
    std::scoped_lock lock(this->sync);

    if (!lastDataPacket.assigned() || lastDataPacket.getSampleCount() == 0)
        return OPENDAQ_IGNORED;

    auto typeManager = this->context.getTypeManager();
    return lastDataPacket->getLastValue(value, typeManager);
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::visibleChanged()
{
    setKeepLastPacket();
}


OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SignalImpl)

END_NAMESPACE_OPENDAQ
