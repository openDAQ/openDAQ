/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <coretypes/string_ptr.h>
#include <coretypes/validation.h>
#include <opendaq/component_impl.h>
#include <opendaq/connection_internal.h>
#include <opendaq/connection_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_packet_impl.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/input_port_private_ptr.h>
#include <opendaq/last_value_cache.h>
#include <opendaq/packet_factory.h>
#include <opendaq/signal.h>
#include <opendaq/signal_config.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/signal_errors.h>
#include <opendaq/signal_events.h>
#include <opendaq/signal_events_ptr.h>
#include <opendaq/signal_exceptions.h>
#include <opendaq/signal_private_ptr.h>
#include <atomic>
#include <limits>
#include <thread>
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

namespace details
{

/*
 * Immutable snapshot of a signal's connections, published with an atomic pointer so the
 * (single) producer thread can fan packets out without taking any lock. Entries hold
 * strong references, so a pinned snapshot keeps every contained connection alive for the
 * duration of a send even if it is disconnected concurrently.
 */
struct ConnectionsSnapshot
{
    struct Entry
    {
        ConnectionPtr connection;               // strong ref
        IConnectionInternal* producerPath{};    // borrowed; single-producer fast path (may be null)
    };

    std::vector<Entry> entries;
    mutable std::atomic<int> refCount{1};       // 1 = the publication reference

    void addRef() const noexcept
    {
        refCount.fetch_add(1, std::memory_order_relaxed);
    }

    void release() const noexcept
    {
        if (refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete this;
    }
};

struct RetiredPacket
{
    IPacket* packet;
    RetiredPacket* next;
};

}

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
    ErrCode INTERFACE_FUNC getLastValueWithTimestamp(IBaseObject** value, IBaseObject** timestamp) override;

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
    ErrCode INTERFACE_FUNC setLastValue(IBaseObject* lastValue) override;

    // ISignalEvents
    ErrCode INTERFACE_FUNC listenerConnected(IConnection* connection) override;
    ErrCode INTERFACE_FUNC listenerConnectedScheduled(IConnection* connection) override;
    ErrCode INTERFACE_FUNC listenerDisconnected(IConnection* connection) override;
    ErrCode INTERFACE_FUNC domainSignalReferenceSet(ISignal* signal) override;
    ErrCode INTERFACE_FUNC domainSignalReferenceRemoved(ISignal* signal) override;

    // ISignalPrivate
    ErrCode INTERFACE_FUNC clearDomainSignalWithoutNotification() override;
    ErrCode INTERFACE_FUNC enableKeepLastValue(Bool enabled) override;
    ErrCode INTERFACE_FUNC getSignalSerializeId(IString** serializeId) override;
    ErrCode INTERFACE_FUNC getKeepLastValue(Bool* keepLastValue) override;
    ErrCode INTERFACE_FUNC sendPacketRecursiveLock(IPacket* packet) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
protected:
    void visibleChanged() override;

    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override;
    void updateObject(const SerializedObjectPtr& obj, const BaseObjectPtr& context) override;

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
    LastValueCache lastValueCache;

private:
    bool isPublic{};
    std::vector<SignalPtr> relatedSignals;
    SignalPtr domainSignal;
    std::vector<ConnectionPtr> connections;
    std::vector<ConnectionPtr> remoteConnections;
    std::vector<WeakRefPtr<ISignalConfig>> domainSignalReferences;
    bool keepLastPacket;
    bool keepLastValue;

    // Lock-free data path state. `connections` above stays the canonical list, owned and
    // mutated only under the config lock; the snapshot is a derived immutable copy that the
    // single producer thread pins per send. See pinConnectionsSnapshot for the protocol.
    std::atomic<details::ConnectionsSnapshot*> connectionsSnapshot;
    mutable std::atomic<int> snapshotGate{0};

    // Last-value support: the producer publishes the latest data packet into an atomic slot
    // (no raw-byte copy on the hot path anymore); getLastValue* lazily feeds lastValueCache
    // from it under the config lock. Retired packets that a concurrent reader may still be
    // pinning are parked on a lock-free list and reclaimed on the config path.
    std::atomic<IPacket*> lastPacketSlot{nullptr};
    std::atomic<std::uint64_t> lastPacketSeq{0};
    mutable std::atomic<int> lastValueGate{0};
    std::atomic<details::RetiredPacket*> retiredPackets{nullptr};
    std::atomic<bool> keepLastPacketAtomic{false};
    std::uint64_t lastValueCachedSeq{std::numeric_limits<std::uint64_t>::max()};  // config-lock guarded

    ErrCode listenerConnectedInternal(IConnection* connection, bool schedule);
    ErrCode sendPacketInner(IPacket* packet, bool recursiveLock);
    bool sendPacketInternal(const PacketPtr& packet, bool ignoreActive = false) const;
    bool sendPacketInternal(PacketPtr&& packet, bool ignoreActive = false) const;
    void triggerRelatedSignalsChanged();
    void disconnectInputPort(const ConnectionPtr& connection);
    void clearConnections(std::vector<ConnectionPtr>& connections);
    void setKeepLastPacket();
    TypePtr addToTypeManagerRecursively(const TypeManagerPtr& typeManager,
                                        const DataDescriptorPtr& descriptor) const;

    details::ConnectionsSnapshot* pinConnectionsSnapshot() const;
    void publishConnectionsSnapshot();
    template <class Packet>
    void cacheLastPacketOnSend(const Packet& packet);
    void retireOrReleaseSlotPacket(IPacket* packet);
    PacketPtr pinLastPacket() const;
    void drainRetiredPackets();
    void clearLastPacketSlot();
    void refreshLastValueCacheFromSlot();

    void enqueuePacketToConnections(const PacketPtr& packet, const details::ConnectionsSnapshot& snapshot);
    void enqueuePacketToConnections(PacketPtr&& packet, const details::ConnectionsSnapshot& snapshot);
    void enqueuePacketsToConnections(const ListPtr<IPacket>& packets, const details::ConnectionsSnapshot& snapshot);
    void enqueuePacketsToConnections(ListPtr<IPacket>&& packets, const details::ConnectionsSnapshot& snapshot);

    template <class Packet>
    bool keepLastPacketAndEnqueue(Packet&& packet, bool recursiveLock = false);

    template <class ListOfPackets>
    bool keepLastPacketAndEnqueueMultiple(ListOfPackets&& packets);

    void setLastValueFromPacket(const DataPacketPtr& packet = nullptr);
    ErrCode getLastValueImpl(IBaseObject** value);
    ErrCode getLastTimestampImpl(IBaseObject** timestamp);
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
    , connectionsSnapshot(new details::ConnectionsSnapshot())
{
    if (dataDescriptor.assigned() && dataDescriptor.getSampleType() == SampleType::Null)
        DAQ_THROW_EXCEPTION(InvalidSampleTypeException, "SampleType \"Null\" is reserved for \"DATA_DESCRIPTOR_CHANGED\" event packet.");
    setKeepLastPacket();

    if (dataDescriptor.assigned() && dataDescriptor.getSampleType() == SampleType::Struct)
    {
        auto typeManager = this->context.getTypeManager();
        addToTypeManagerRecursively(typeManager, dataDescriptor);
    }
}

template <typename TInterface, typename... Interfaces>
SignalBase<TInterface, Interfaces...>::~SignalBase()
{
    if (domainSignal.assigned())
        domainSignal.asPtr<ISignalEvents>().domainSignalReferenceRemoved(this->template borrowPtr<SignalPtr>());

    // no producer or reader can be active anymore: refcount reached zero
    if (auto* snapshot = connectionsSnapshot.exchange(nullptr, std::memory_order_acquire))
        snapshot->release();
    if (IPacket* packet = lastPacketSlot.exchange(nullptr, std::memory_order_acquire))
        packet->releaseRef();
    auto* retired = retiredPackets.exchange(nullptr, std::memory_order_acquire);
    while (retired)
    {
        retired->packet->releaseRef();
        auto* next = retired->next;
        delete retired;
        retired = next;
    }
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getPublic(Bool* isPublic)
{
    OPENDAQ_PARAM_NOT_NULL(isPublic);

    auto lock = this->getRecursiveConfigLock2();

    *isPublic = this->isPublic;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setPublic(Bool isPublic)
{
    if (this->isFrozen())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);

    {
        auto lock = this->getRecursiveConfigLock2();

        if (this->lockedAttributes.count("Public"))
        {
            if (this->context.assigned() && this->context.getLogger().assigned())
            {
                const auto loggerComponent = this->context.getLogger().getOrAddComponent("Component");
                StringPtr descObj;
                this->getName(&descObj);
                LOG_I("'Public' attribute of {} is locked", descObj);
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

    auto lock = this->getRecursiveConfigLock2();
    
    DataDescriptorPtr dataDescriptorPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetDescriptor, dataDescriptorPtr);
    OPENDAQ_RETURN_IF_FAILED(errCode);

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

    EventPacketPtr packet = DataDescriptorChangedEventPacket(descriptorToEventPacketParam(onGetDescriptor()),
                                                             descriptorToEventPacketParam(domainDataDescriptor));
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
        DAQ_THROW_EXCEPTION(NotAssignedException, "Name of data descriptor not assigned.");

    const auto fields = descriptor.getStructFields();
    auto fieldNames = List<IString>();
    auto fieldTypes = List<IType>();

    if (fields.assigned())
    {
        for (auto const& field : fields)
        {
            const auto dimensions = field.getDimensions();

            if (!dimensions.assigned())
                DAQ_THROW_EXCEPTION(NotAssignedException, "Dimensions of data descriptor not assigned.");

            const auto dimensionCount = dimensions.getCount();

            if (dimensionCount > 1)
                DAQ_THROW_EXCEPTION(NotSupportedException, "getLastValue on signals with dimensions supports only up to one dimension.");

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
    const auto descriptorPtr = DataDescriptorPtr::Borrow(descriptor);
    if (descriptorPtr.assigned() && descriptorPtr.getSampleType() == SampleType::Null)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_SAMPLE_TYPE,
                                   "SampleType \"Null\" is reserved for \"DATA_DESCRIPTOR_CHANGED\" event packet.");

    if (BaseObjectPtr::Equals(descriptorPtr, this->dataDescriptor))
    {
        const auto loggerComponent = this->context.getLogger().getOrAddComponent("Signal");
        LOG_D("Signal descriptor was set to the same value as before");
        return OPENDAQ_IGNORED;
    }

    std::vector<SignalConfigPtr> valueSignalsOfDomainSignal;
    bool success;

    {
        auto lock = this->getRecursiveConfigLock2();

        dataDescriptor = descriptorPtr;
        const auto packet = DataDescriptorChangedEventPacket(descriptorToEventPacketParam(dataDescriptor), nullptr);

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
            try
            {
                if (dataDescriptor.assigned() && dataDescriptor.getSampleType() == SampleType::Struct)
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
        const EventPacketPtr domainChangedPacket =
            DataDescriptorChangedEventPacket(nullptr, descriptorToEventPacketParam(dataDescriptor));
        for (const auto& sig : valueSignalsOfDomainSignal)
        {
            const auto err = sig.asPtr<ISignalPrivate>()->sendPacketRecursiveLock(domainChangedPacket);
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

    auto lock = this->getRecursiveConfigLock2();

    SignalPtr signalPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetDomainSignal, signalPtr);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    *signal = signalPtr.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setDomainSignal(ISignal* signal)
{
    {
        auto lock = this->getRecursiveConfigLock2();
        
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

    auto lock = this->getRecursiveConfigLock2();

    ListPtr<ISignal> signalsPtr{relatedSignals};
    *signals = signalsPtr.detach();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setRelatedSignals(IList* signals)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    {
        auto lock = this->getRecursiveConfigLock2();

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
        auto lock = this->getRecursiveConfigLock2();

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
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DUPLICATEITEM);

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
        auto lock = this->getRecursiveConfigLock2();

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
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

        relatedSignals.erase(it);
    }

    triggerRelatedSignalsChanged();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::clearRelatedSignals()
{
    {
        auto lock = this->getRecursiveConfigLock2();
        relatedSignals.clear();
    }
    
    triggerRelatedSignalsChanged();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getConnections(IList** connections)
{
    OPENDAQ_PARAM_NOT_NULL(connections);

    auto lock = this->getRecursiveConfigLock2();

    auto connectionsPtr = List<IConnection>();
    for (const auto& conn : this->connections)
        connectionsPtr.pushBack(conn);
    for (const auto& conn : this->remoteConnections)
        connectionsPtr.pushBack(conn);

    *connections = connectionsPtr.detach();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
details::ConnectionsSnapshot* SignalBase<TInterface, Interfaces...>::pinConnectionsSnapshot() const
{
    // Publication protocol (Dekker-style; the four seq_cst operations marked below are part
    // of the single seq_cst total order S):
    //   reader: gate++ (A); snapshot load (B); refCount pin; gate--
    //   writer: snapshot exchange (C); spin while gate != 0 (D); release publication ref
    // If B returned the old snapshot, B precedes C in S (a load ordered after C would have
    // to observe the new pointer), hence A precedes C as well, and every D executed after C
    // observes gate > 0 until this reader's gate--, which is sequenced after the pin.
    // The writer therefore can never release the old snapshot between B and the pin.
    snapshotGate.fetch_add(1, std::memory_order_seq_cst);                       // (A)
    auto* snapshot = connectionsSnapshot.load(std::memory_order_seq_cst);       // (B)
    snapshot->addRef();
    snapshotGate.fetch_sub(1, std::memory_order_release);
    return snapshot;
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::publishConnectionsSnapshot()
{
    // config paths only, always under the config lock (serializes writers against each other)
    auto* snapshot = new details::ConnectionsSnapshot();
    snapshot->entries.reserve(connections.size());
    for (const auto& connection : connections)
    {
        details::ConnectionsSnapshot::Entry entry;
        entry.connection = connection;
        IConnectionInternal* internal = nullptr;
        if (OPENDAQ_SUCCEEDED(connection->borrowInterface(IConnectionInternal::Id, reinterpret_cast<void**>(&internal))))
            entry.producerPath = internal;
        snapshot->entries.push_back(std::move(entry));
    }

    auto* old = connectionsSnapshot.exchange(snapshot, std::memory_order_seq_cst);  // (C)
    while (snapshotGate.load(std::memory_order_seq_cst) != 0)                       // (D)
        std::this_thread::yield();
    old->release();
}

template <typename TInterface, typename... Interfaces>
template <class Packet>
void SignalBase<TInterface, Interfaces...>::cacheLastPacketOnSend(const Packet& packet)
{
    const auto dataPacket = packet.template asPtrOrNull<IDataPacket>(true);
    if (!dataPacket.assigned() || dataPacket.getSampleCount() == 0)
        return;

    IPacket* newPacket = packet.addRefAndReturn();
    IPacket* old = lastPacketSlot.exchange(newPacket, std::memory_order_seq_cst);
    lastPacketSeq.fetch_add(1, std::memory_order_release);
    retireOrReleaseSlotPacket(old);

    // enableKeepLastValue(false)/setPublic(false) may have cleared the slot concurrently;
    // re-check so a disabled cache never retains a packet (single producer: nobody else
    // can store into the slot between the exchange above and this check).
    if (!keepLastPacketAtomic.load(std::memory_order_acquire))
        retireOrReleaseSlotPacket(lastPacketSlot.exchange(nullptr, std::memory_order_seq_cst));
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::retireOrReleaseSlotPacket(IPacket* packet)
{
    if (!packet)
        return;

    // Same gate argument as pinConnectionsSnapshot, with roles reversed: the slot exchange
    // that produced `packet` is seq_cst; if the gate reads 0 afterwards, every reader that
    // could have loaded this pointer has already completed its addRef, so releasing here is
    // safe. Otherwise park the packet for config-path reclamation (drainRetiredPackets).
    if (lastValueGate.load(std::memory_order_seq_cst) == 0)
    {
        packet->releaseRef();
        return;
    }

    auto* node = new details::RetiredPacket{packet, nullptr};
    auto* cur = retiredPackets.load(std::memory_order_relaxed);
    do
    {
        node->next = cur;
    } while (!retiredPackets.compare_exchange_weak(cur, node, std::memory_order_release, std::memory_order_relaxed));
}

template <typename TInterface, typename... Interfaces>
PacketPtr SignalBase<TInterface, Interfaces...>::pinLastPacket() const
{
    lastValueGate.fetch_add(1, std::memory_order_seq_cst);
    IPacket* raw = lastPacketSlot.load(std::memory_order_seq_cst);
    if (raw)
        raw->addRef();
    lastValueGate.fetch_sub(1, std::memory_order_release);
    return PacketPtr::Adopt(raw);
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::drainRetiredPackets()
{
    // config path, under the config lock
    auto* chain = retiredPackets.exchange(nullptr, std::memory_order_acq_rel);
    if (!chain)
        return;

    // wait out any reader still inside the pin window; afterwards every packet on the
    // chain either carries the reader's extra ref or is unreferenced by readers
    while (lastValueGate.load(std::memory_order_seq_cst) != 0)
        std::this_thread::yield();

    while (chain)
    {
        chain->packet->releaseRef();
        auto* next = chain->next;
        delete chain;
        chain = next;
    }
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::clearLastPacketSlot()
{
    // config path, under the config lock
    retireOrReleaseSlotPacket(lastPacketSlot.exchange(nullptr, std::memory_order_seq_cst));
    drainRetiredPackets();
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::enqueuePacketToConnections(const PacketPtr& packet,
                                                                       const details::ConnectionsSnapshot& snapshot)
{
    for (const auto& entry : snapshot.entries)
    {
        if (entry.producerPath)
            checkErrorInfo(entry.producerPath->enqueueProducer(packet));
        else
            entry.connection.enqueue(packet);
    }
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::enqueuePacketToConnections(PacketPtr&& packet,
                                                                       const details::ConnectionsSnapshot& snapshot)
{
    if (snapshot.entries.empty())
        return;

    auto startIt = snapshot.entries.begin();
    const auto endIt = std::prev(snapshot.entries.end());

    for (; startIt != endIt; ++startIt)
    {
        if (startIt->producerPath)
            checkErrorInfo(startIt->producerPath->enqueueProducer(packet));
        else
            startIt->connection.enqueue(packet);
    }

    if (startIt->producerPath)
        checkErrorInfo(startIt->producerPath->enqueueProducerAndStealRef(packet.detach()));
    else
        startIt->connection->enqueueAndStealRef(packet.detach());
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::enqueuePacketsToConnections(
    const ListPtr<IPacket>& packets,
    const details::ConnectionsSnapshot& snapshot)
{
    for (const auto& entry : snapshot.entries)
    {
        if (entry.producerPath)
            checkErrorInfo(entry.producerPath->enqueueMultipleProducer(packets));
        else
            entry.connection.enqueueMultiple(packets);
    }
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::enqueuePacketsToConnections(
    ListPtr<IPacket>&& packets,
    const details::ConnectionsSnapshot& snapshot)
{
    if (snapshot.entries.empty())
        return;

    auto startIt = snapshot.entries.begin();
    const auto endIt = std::prev(snapshot.entries.end());

    for (; startIt != endIt; ++startIt)
    {
        if (startIt->producerPath)
            checkErrorInfo(startIt->producerPath->enqueueMultipleProducer(packets));
        else
            startIt->connection.enqueueMultiple(packets);
    }

    if (startIt->producerPath)
        checkErrorInfo(startIt->producerPath->enqueueMultipleProducerAndStealRef(packets.detach()));
    else
        startIt->connection->enqueueMultipleAndStealRef(packets.detach());
}

template <typename TInterface, typename... Interfaces>
template <class Packet>
bool SignalBase<TInterface, Interfaces...>::keepLastPacketAndEnqueue(Packet&& packet, bool /*recursiveLock*/)
{
    // The data path takes no lock: `active` is atomic, the last packet goes into an atomic
    // slot and the connection list is a pinned immutable snapshot. The recursiveLock flag
    // is obsolete (there is no lock left to recurse on) but kept for the internal ABI.
    if (!this->active.load(std::memory_order_relaxed))
        return false;

    if (keepLastPacketAtomic.load(std::memory_order_relaxed))
        cacheLastPacketOnSend(packet);

    auto* snapshot = pinConnectionsSnapshot();
    try
    {
        enqueuePacketToConnections(std::forward<Packet>(packet), *snapshot);
    }
    catch (...)
    {
        snapshot->release();
        throw;
    }
    snapshot->release();

    return true;
}

template <typename TInterface, typename... Interfaces>
template <class ListOfPackets>
bool SignalBase<TInterface, Interfaces...>::keepLastPacketAndEnqueueMultiple(ListOfPackets&& packets)
{
    const size_t cnt = packets.getCount();

    if (!this->active.load(std::memory_order_relaxed) || cnt == 0)
        return false;

    if (keepLastPacketAtomic.load(std::memory_order_relaxed))
        cacheLastPacketOnSend(packets[cnt - 1]);

    auto* snapshot = pinConnectionsSnapshot();
    try
    {
        enqueuePacketsToConnections(std::forward<ListOfPackets>(packets), *snapshot);
    }
    catch (...)
    {
        snapshot->release();
        throw;
    }
    snapshot->release();

    return true;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::sendPacket(IPacket* packet)
{
    return sendPacketInner(packet, false);
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::sendPacketAndStealRef(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    auto packetPtr = PacketPtr::Adopt(packet);

    const ErrCode errCode = daqTry(
    [this, packet = std::move(packetPtr)] () mutable
    {
        if (!keepLastPacketAndEnqueue(std::move(packet)))
            return OPENDAQ_IGNORED;

        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <typename TInterface, typename... Interfaces>
ErrCode INTERFACE_FUNC SignalBase<TInterface, Interfaces...>::sendPackets(IList* packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    const auto packetsPtr = ListPtr<IPacket>::Borrow(packets);

    const ErrCode errCode = daqTry([&packetsPtr, this]
    {
        if (!keepLastPacketAndEnqueueMultiple(packetsPtr))
            return OPENDAQ_IGNORED;

        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <typename TInterface, typename... Interfaces>
ErrCode INTERFACE_FUNC SignalBase<TInterface, Interfaces...>::sendPacketsAndStealRef(IList* packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    auto packetsPtr = ListPtr<IPacket>::Adopt(packets);

    return daqTry([this, packets = std::move(packetsPtr)] () mutable
    {
        if (!keepLastPacketAndEnqueueMultiple(std::move(packets)))
            return OPENDAQ_IGNORED;

        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::setLastValue(IBaseObject* lastValue)
{
    auto lock = this->getRecursiveConfigLock2();

    clearLastPacketSlot();
    setLastValueFromPacket(nullptr);
    lastValueCache.setValue(BaseObjectPtr(lastValue));
    // packets sent after this point win over the explicit value (seq mismatch -> re-cache)
    lastValueCachedSeq = lastPacketSeq.load(std::memory_order_acquire);
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::listenerConnectedInternal(IConnection* connection, bool schedule)
{
    OPENDAQ_PARAM_NOT_NULL(connection);

    const auto connectionPtr = ConnectionPtr::Borrow(connection);

    auto lock = this->getRecursiveConfigLock2();

    if (connectionPtr.isRemote())
    {
        const auto it = std::find(remoteConnections.begin(), remoteConnections.end(), connectionPtr);
        if (it != remoteConnections.end())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DUPLICATEITEM);

        remoteConnections.push_back(connectionPtr);
        return OPENDAQ_SUCCESS;
    }

    const auto it = std::find(connections.begin(), connections.end(), connectionPtr);
    if (it != connections.end())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DUPLICATEITEM);

    const auto packet = createDataDescriptorChangedEventPacket();

    bool listenedStatusChanged = false;
    if (connections.empty())
    {
        const ErrCode errCode = wrapHandler(this, &Self::onListenedStatusChanged, true);
        OPENDAQ_RETURN_IF_FAILED(errCode);
        listenedStatusChanged = true;
    }

    // Seed the new connection's queue with the descriptor event BEFORE publishing it to the
    // producer: any producer snapshot that contains this connection then contains the event
    // ahead of every data packet. This replaces the descriptor-event-before-data ordering
    // that the shared sendPacket mutex used to provide.
    const ErrCode enqueueErrCode = daqTry(
        [&]
        {
            if (!schedule)
                connectionPtr.enqueueOnThisThread(packet);
            else
                connectionPtr.enqueueWithScheduler(packet);
        });
    if (OPENDAQ_FAILED(enqueueErrCode))
    {
        // keep the empty <-> non-empty transitions of onListenedStatusChanged consistent
        if (listenedStatusChanged)
            wrapHandler(this, &Self::onListenedStatusChanged, false);
        return enqueueErrCode;
    }

    connections.push_back(connectionPtr);
    publishConnectionsSnapshot();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::sendPacketInner(IPacket* packet, bool recursiveLock)
{
    OPENDAQ_PARAM_NOT_NULL(packet);
    const auto packetPtr = PacketPtr::Borrow(packet);
    const ErrCode errCode = daqTry([this, &packetPtr, recursiveLock]
    {
        if (!keepLastPacketAndEnqueue(packetPtr, recursiveLock))
            return OPENDAQ_IGNORED;

        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
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
    return listenerConnectedInternal(connection, false);
}

template <typename TInterface, typename ... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::listenerConnectedScheduled(IConnection* connection)
{
    return listenerConnectedInternal(connection, true);
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::listenerDisconnected(IConnection* connection)
{
    OPENDAQ_PARAM_NOT_NULL(connection);

    const auto connectionPtr = ObjectPtr<IConnection>::Borrow(connection);

    auto lock = this->getRecursiveConfigLock2();

    if (connectionPtr.isRemote())
    {
        const auto it = std::find(remoteConnections.begin(), remoteConnections.end(), connectionPtr);
        if (it == remoteConnections.end())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

        remoteConnections.erase(it);
        return OPENDAQ_SUCCESS;
    }

    const auto it = std::find(connections.begin(), connections.end(), connectionPtr);
    if (it == connections.end())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    const ConnectionPtr removedConnection = std::move(*it);
    connections.erase(it);
    // unpublish first, then close: a producer pinning an older snapshot still holds a strong
    // ref (no use-after-free); its in-flight enqueue either lands before the close (and is
    // drained by it) or after (and is dropped) - either way nothing stays pinned in the
    // orphaned queue and no new packet can enter it after this call returns
    publishConnectionsSnapshot();
    if (const auto internal = removedConnection.template asPtrOrNull<IConnectionInternal>(true); internal.assigned())
        internal->closeQueue();

    if (connections.empty())
    {
        const ErrCode errCode = wrapHandler(this, &Self::onListenedStatusChanged, false);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::domainSignalReferenceSet(ISignal* signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    const auto signalPtr = SignalPtr::Borrow(signal).asPtrOrNull<ISignalConfig>(true);
    if (!signalPtr.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOINTERFACE, "Signal does not implement ISignalConfig interface.");

    auto lock = this->getRecursiveConfigLock2();
    for (const auto& refSignal : domainSignalReferences)
    {
        if (refSignal.getRef() == signalPtr)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DUPLICATEITEM);
    }

    domainSignalReferences.push_back(WeakRefPtr<ISignalConfig>(signal));
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::domainSignalReferenceRemoved(ISignal* signal)
{
    auto lock = this->getRecursiveConfigLock2();

    const auto signalPtr = SignalPtr::Borrow(signal).asPtrOrNull<ISignalConfig>(true);
    if (!signalPtr.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOINTERFACE, "Signal does not implement ISignalConfig interface.");

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
    auto lock = this->getRecursiveConfigLock2();

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
    const ErrCode errCode = daqTry([&obj, &serialized, &context, &factoryCallback]()
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
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate)
{
    if (!forUpdate)
    {
        const SignalPtr domainSignalObj = onGetDomainSignal();
        if (domainSignalObj.assigned())
        {
            serializer.key("domainSignalId");
            const auto domainSignalGlobalId = domainSignalObj.getGlobalId();
            serializer.writeString(domainSignalGlobalId);
        }

        const DataDescriptorPtr dataDescriptorObj = onGetDescriptor();
        if (dataDescriptorObj.assigned())
        {
            serializer.key("dataDescriptor");
            dataDescriptorObj.serialize(serializer);
        }

        StringPtr ownerId;
        checkErrorInfo(getSignalSerializeId(&ownerId));
        serializer.key("OwnerSignalGlobalId");
        serializer.writeString(ownerId);
    }

    if (!isPublic)
    {
        serializer.key("public");
        serializer.writeBool(isPublic);
    }

    Super::serializeCustomObjectValues(serializer, forUpdate);
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::updateObject(const SerializedObjectPtr& obj, const BaseObjectPtr& context)
{
    if (!this->lockedAttributes.count("Public"))
    {
        if (obj.hasKey("public"))
            isPublic = obj.readBool("public");
        else
            isPublic = true;
        setKeepLastPacket();
    }

    Super::updateObject(obj, context);
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
    {
        if (const auto internal = connection.template asPtrOrNull<IConnectionInternal>(true); internal.assigned())
            internal->closeQueue();
        disconnectInputPort(connection);
    }
    connections.clear();
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::removed()
{
    clearConnections(connections);
    clearConnections(remoteConnections);
    publishConnectionsSnapshot();  // now empty: producers can no longer reach any connection
    clearLastPacketSlot();

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
    if (parameter == "domainSignalId")
        return deserializedDomainSignalId;

    DAQ_THROW_EXCEPTION(NotFoundException);
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                                                          const BaseObjectPtr& context,
                                                                          const FunctionPtr& factoryCallback)
{
    Super::deserializeCustomObjectValues(serializedObject, context, factoryCallback);
    if (serializedObject.hasKey("domainSignalId"))
        deserializedDomainSignalId = serializedObject.readString("domainSignalId");
    if (serializedObject.hasKey("dataDescriptor"))
        dataDescriptor = serializedObject.readObject("dataDescriptor", context, factoryCallback);
    if (serializedObject.hasKey("public"))
        isPublic = serializedObject.readBool("public");
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
    auto lock = this->getRecursiveConfigLock2();
    keepLastValue = enabled;

    setKeepLastPacket();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::setKeepLastPacket()
{
    keepLastPacket = keepLastValue && isPublic;
    keepLastPacketAtomic.store(keepLastPacket, std::memory_order_release);

    if (!keepLastPacket)
    {
        clearLastPacketSlot();
        setLastValueFromPacket(nullptr);
        lastValueCachedSeq = std::numeric_limits<std::uint64_t>::max();
    }
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getLastValue(IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(value);
    auto lock = this->getRecursiveConfigLock2();
    const ErrCode errCode = getLastValueImpl(value);

    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getLastValueWithTimestamp(IBaseObject** value, IBaseObject** timestamp)
{
    OPENDAQ_PARAM_NOT_NULL(value);
    OPENDAQ_PARAM_NOT_NULL(timestamp);
    auto lock = this->getRecursiveConfigLock2();
    const ErrCode valueErrCode = getLastValueImpl(value);

    OPENDAQ_RETURN_IF_FAILED(valueErrCode);

    const ErrCode tsErrCode = getLastTimestampImpl(timestamp);

    OPENDAQ_RETURN_IF_FAILED(tsErrCode);
    if (valueErrCode == OPENDAQ_IGNORED || tsErrCode == OPENDAQ_IGNORED)
        return OPENDAQ_IGNORED;

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getSignalSerializeId(IString** serializeId)
{
    return this->getGlobalId(serializeId);
}

template <typename TInterface, typename ... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getKeepLastValue(Bool* keepLastValue)
{
    OPENDAQ_PARAM_NOT_NULL(keepLastValue);

    auto lock = this->getRecursiveConfigLock2();

    *keepLastValue = this->keepLastValue ? True : False;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::sendPacketRecursiveLock(IPacket* packet)
{
    return sendPacketInner(packet, true);
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::visibleChanged()
{
    setKeepLastPacket();
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::setLastValueFromPacket(const DataPacketPtr& packet)
{
    lastValueCache.cache(packet);
}

template <typename TInterface, typename... Interfaces>
void SignalBase<TInterface, Interfaces...>::refreshLastValueCacheFromSlot()
{
    // config path, under the config lock: feed lastValueCache lazily from the packet the
    // producer last published (the raw-byte copy no longer happens on the send path)
    const std::uint64_t seq = lastPacketSeq.load(std::memory_order_acquire);
    if (seq == lastValueCachedSeq)
        return;

    const PacketPtr packet = pinLastPacket();
    drainRetiredPackets();
    if (packet.assigned())
    {
        lastValueCache.cache(packet.template asPtr<IDataPacket>(true));
        lastValueCachedSeq = seq;
    }
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getLastValueImpl(IBaseObject** value)
{
    refreshLastValueCacheFromSlot();

    if (lastValueCache.valueCached())
    {
        *value = lastValueCache.getValue().detach();
        return OPENDAQ_SUCCESS;
    }

    if (!lastValueCache.valueDescriptorCached())
        return OPENDAQ_IGNORED;

    const ErrCode errCode = daqTry(
        [&value, this]
        {
            auto manager = this->context.getTypeManager();
            void* rawValue = lastValueCache.getRawValueData();
            lastValueCache.setValue(
                PacketDetails::buildObjectFromDescriptor(rawValue, lastValueCache.getValueDataDescriptor(), manager, lastValueCache.getActualValueSampleSize()));
            *value = lastValueCache.getValue().detach();
        });
    return errCode;
}

template <typename TInterface, typename... Interfaces>
ErrCode SignalBase<TInterface, Interfaces...>::getLastTimestampImpl(IBaseObject** timestamp)
{
    refreshLastValueCacheFromSlot();

    if (lastValueCache.timestampCached())
    {
        *timestamp = lastValueCache.getTimestamp().detach();
        return OPENDAQ_SUCCESS;
    }

    if (!lastValueCache.domainDescriptorCached())
        return OPENDAQ_IGNORED;

    const ErrCode errCode = daqTry(
        [&timestamp, this]
        {
            auto manager = this->context.getTypeManager();
            void* rawValue = lastValueCache.getRawTimestampData();
            auto tsWithoutTweak = PacketDetails::buildObjectFromDescriptor(rawValue, lastValueCache.getDomainDataDescriptor(), manager, 0);

            lastValueCache.calculateTimestamp(tsWithoutTweak);
            *timestamp = lastValueCache.getTimestamp().detach();
        });

    if (OPENDAQ_FAILED(errCode))
    {
        daqClearErrorInfo();
        lastValueCache.resetTimestamp();
        return OPENDAQ_IGNORED;
    }
    return errCode;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SignalImpl)

END_NAMESPACE_OPENDAQ
