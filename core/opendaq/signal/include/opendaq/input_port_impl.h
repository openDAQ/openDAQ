/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/connection_factory.h>
#include <opendaq/context_ptr.h>
#include <opendaq/component_impl.h>
#include <opendaq/input_port_config.h>
#include <opendaq/input_port_private.h>
#include <opendaq/input_port_notifications_ptr.h>
#include <opendaq/input_port_ptr.h>
#include <opendaq/removable_ptr.h>
#include <opendaq/signal_errors.h>
#include <opendaq/signal_events_ptr.h>
#include <opendaq/signal_factory.h>
#include <opendaq/work_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <class... Interfaces>
class GenericInputPortImpl;

using InputPortImpl = GenericInputPortImpl<>;

template <class ... Interfaces>
class GenericInputPortImpl : public ComponentImpl<IInputPortConfig, IInputPortPrivate, Interfaces ...>
{
public:
    using Super = ComponentImpl<IInputPortConfig, IInputPortPrivate, Interfaces ...>;

    explicit GenericInputPortImpl(const ContextPtr& context,
                                  const ComponentPtr& parent,
                                  const StringPtr& localId,
                                  bool gapChecking = false);

    ErrCode INTERFACE_FUNC acceptsSignal(ISignal* signal, Bool* accepts) override;
    ErrCode INTERFACE_FUNC connect(ISignal* signal) override;
    ErrCode INTERFACE_FUNC disconnect() override;
    ErrCode INTERFACE_FUNC getSignal(ISignal** signal) override;
    ErrCode INTERFACE_FUNC getConnection(IConnection** connection) override;
    ErrCode INTERFACE_FUNC getRequiresSignal(Bool* requiresSignal) override;

    ErrCode INTERFACE_FUNC setNotificationMethod(PacketReadyNotification method) override;
    ErrCode INTERFACE_FUNC notifyPacketEnqueued(Bool queueWasEmpty) override;
    ErrCode INTERFACE_FUNC notifyPacketEnqueuedOnThisThread() override;
    ErrCode INTERFACE_FUNC setListener(IInputPortNotifications* port) override;

    ErrCode INTERFACE_FUNC getCustomData(IBaseObject** data) override;
    ErrCode INTERFACE_FUNC setCustomData(IBaseObject* data) override;
    ErrCode INTERFACE_FUNC setRequiresSignal(Bool requiresSignal) override;

    ErrCode INTERFACE_FUNC getGapCheckingEnabled(Bool* gapCheckingEnabled) override;

    // IInputPortPrivate
    ErrCode INTERFACE_FUNC disconnectWithoutSignalNotification() override;

    // IRemovable
    ErrCode INTERFACE_FUNC remove() override;
    ErrCode INTERFACE_FUNC isRemoved(Bool* removed) override;

    // IOwnable
    ErrCode INTERFACE_FUNC setOwner(IPropertyObject* owner) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    // IBaseObject
    ErrCode INTERFACE_FUNC queryInterface(const IntfID& id, void** intf) override;
    ErrCode INTERFACE_FUNC borrowInterface(const IntfID& id, void** intf) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override;

    void updateObject(const SerializedObjectPtr& obj) override;
    void onUpdatableUpdateEnd() override;
    ComponentPtr getRootComponent(const ComponentPtr& curComponent);

    BaseObjectPtr getDeserializedParameter(const StringPtr& parameter) override;

    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

    virtual ConnectionPtr createConnection(const SignalPtr& signal);

    ConnectionPtr getConnectionNoLock();
    
    StringPtr serializedSignalId;

private:
    Bool requiresSignal;
    bool gapCheckingEnabled;
    BaseObjectPtr customData;
    PacketReadyNotification notifyMethod{};

    WeakRefPtr<IInputPortNotifications> listenerRef;
    WeakRefPtr<IConnection> connectionRef{};
    bool isInputPortRemoved;
    WorkPtr notifySchedulerCallback;

    LoggerComponentPtr loggerComponent;
    SchedulerPtr scheduler;

    SignalPtr dummySignal;

    WeakRefPtr<IPropertyObject> owner;

    ErrCode canConnectSignal(ISignal* signal) const;
    void disconnectSignalInternal(bool notifyListener, bool notifySignal);
    void notifyPacketEnqueuedSameThread();
    void notifyPacketEnqueuedScheduler();
    void finishUpdate();

    SignalPtr getSignalNoLock();
};

template <class... Interfaces>
GenericInputPortImpl<Interfaces ...>::GenericInputPortImpl(const ContextPtr& context,
                             const ComponentPtr& parent,
                             const StringPtr& localId,
                             bool gapCheckingEnabled)
    : Super(context, parent, localId)
    , requiresSignal(true)
    , gapCheckingEnabled(gapCheckingEnabled)
    , notifyMethod(PacketReadyNotification::None)
    , listenerRef(nullptr)
    , connectionRef(nullptr)
    , isInputPortRemoved(false)
{
    loggerComponent = context.getLogger().getOrAddComponent("InputPort");
    if (context.assigned())
        scheduler = context.getScheduler();
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::acceptsSignal(ISignal* signal, Bool* accepts)
{
    if (signal == nullptr || accepts == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    const auto err = canConnectSignal(signal);
    if (OPENDAQ_FAILED(err))
        return err;

    if (listenerRef.assigned())
    {
        const auto listener = listenerRef.getRef();
        if (listener.assigned())
            return listener->acceptsSignal(Super::template borrowInterface<IInputPort>(), signal, accepts);
    }

    *accepts = true;
    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::canConnectSignal(ISignal* signal) const
{
    const auto removablePtr = SignalPtr::Borrow(signal).asPtrOrNull<IRemovable>();
    if (removablePtr.assigned() && removablePtr.isRemoved())
        return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Removed signal cannot be connected");

    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::connect(ISignal* signal)
{
    if (signal == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    try
    {
        auto err = canConnectSignal(signal);
        if (OPENDAQ_FAILED(err))
            return err;

        auto signalPtr = SignalPtr::Borrow(signal);
        Bool accepted;
        err = this->acceptsSignal(signalPtr, &accepted);
        if (OPENDAQ_FAILED(err))
            return err;

        if (!accepted)
            return OPENDAQ_ERR_SIGNAL_NOT_ACCEPTED;

        const auto connection = createConnection(signalPtr);

        InputPortNotificationsPtr inputPortListener;
        {
            std::scoped_lock lock(this->sync);
            if (isInputPortRemoved)
                return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Cannot connect signal to removed input port");

            connectionRef = connection;

            if (listenerRef.assigned())
                inputPortListener = listenerRef.getRef();
        }

        if (inputPortListener.assigned())
        {
            err = inputPortListener->connected(Super::template borrowInterface<IInputPort>());
            if (OPENDAQ_FAILED(err))
            {
                connectionRef.release();
                return err;
            }
        }

        const auto events = signalPtr.asPtrOrNull<ISignalEvents>(true);
        if (events.assigned())
        {
            // NORRIS/TODO: what's the correct behavior if this fails? what about when the error is OPENDAQ_ERR_NOTIMPLEMENTED?
            // - behavior before my change is to propagate the error up to the caller even in the latter case, which seems wrong
            // - behavior now is that OPENDAQ_ERR_NOTIMPLEMENTED is ignored but other errors are propagated
            // - do we have (and if not, do we want) helper methods for exception handling like this? something like:
            //       ignoreErrors([]() { events.listenerConnected(connection); }, OPENDAQ_ERR_HARMLESS1, OPENDAQ_ERR_HARMLESS2);

            try
            {
                events.listenerConnected(connection);
            }
            catch (const DaqException& e)
            {
                if (e.getErrCode() != OPENDAQ_ERR_NOTIMPLEMENTED)
                    throw;
            }
        }

        dummySignal.release();
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (const std::exception& e)
    {
        return errorFromException(e);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(CoreEventId::SignalConnected,
                                                                                      Dict<IString, IBaseObject>({{"Signal", signal}}));

        this->triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
void GenericInputPortImpl<Interfaces...>::disconnectSignalInternal(bool notifyListener, bool notifySignal)
{
    ConnectionPtr connection;
    {
        std::scoped_lock lock(this->sync);

        if (!connectionRef.assigned())
            return;

        connection = connectionRef.getRef();
        connectionRef.release();
    }

    if (!connection.assigned())
        return;

    if (notifySignal)
    {
        const auto signal = connection.getSignal();
        if (signal.assigned())
        {
            const SignalEventsPtr events = signal.asPtrOrNull<ISignalEvents>(true);
            if (events.assigned())
                events.listenerDisconnected(connection);
        }
    }

    connection.release();

    if (notifyListener)
    {
        if (listenerRef.assigned())
        {
            const auto listener = listenerRef.getRef();
            if (listener.assigned())
                listener->disconnected(Super::template borrowInterface<IInputPort>());
        }
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const auto args =
            createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(CoreEventId::SignalDisconnected, Dict<IString, IBaseObject>());

        this->triggerCoreEvent(args);
    }
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::disconnect()
{
    return daqTry(
        [this]() -> auto
        {
            disconnectSignalInternal(true, true);
            return OPENDAQ_SUCCESS;
        });
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::getSignal(ISignal** signal)
{
    if (signal == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::scoped_lock lock(this->sync);

    *signal = getSignalNoLock().detach();

    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
SignalPtr GenericInputPortImpl<Interfaces...>::getSignalNoLock()
{
    if (!connectionRef.assigned())
        return nullptr;

    const auto connection = connectionRef.getRef();
    return connection.assigned() ? connection.getSignal() : nullptr;
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::getConnection(IConnection** connection)
{
    if (connection == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::scoped_lock lock(this->sync);

    return daqTry([this, &connection] { *connection = getConnectionNoLock().detach(); });
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::setNotificationMethod(PacketReadyNotification method)
{
    std::scoped_lock lock(this->sync);

    if ((method == PacketReadyNotification::Scheduler || method == PacketReadyNotification::SchedulerQueueWasEmpty) && !scheduler.assigned())
    {
        LOG_W("Scheduler based notification not available");
        notifyMethod = PacketReadyNotification::SameThread;
    }
    else
        notifyMethod = method;

    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
void GenericInputPortImpl<Interfaces...>::notifyPacketEnqueuedSameThread()
{
    if (listenerRef.assigned())
    {
        auto listener = listenerRef.getRef();
        if (listener.assigned())
        {
            try
            {
                listener.packetReceived(this->template thisInterface<IInputPort>());
            }
            catch (const std::exception& e)
            {
                LOG_E("Input port notification failed: {}", e.what());
            }
        }
    }
}

template <class... Interfaces>
void GenericInputPortImpl<Interfaces...>::notifyPacketEnqueuedScheduler()
{
    scheduler.scheduleWork(notifySchedulerCallback);
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::notifyPacketEnqueued(Bool queueWasEmpty)
{
    return wrapHandler(
        [this, &queueWasEmpty]
        {
            switch (notifyMethod)
            {
                case PacketReadyNotification::SameThread:
                {
                    notifyPacketEnqueuedSameThread();
                    break;
                }
                case PacketReadyNotification::Scheduler:
                {
                    notifyPacketEnqueuedScheduler();
                    break;
                }
                case PacketReadyNotification::SchedulerQueueWasEmpty:
                {
                    if (!queueWasEmpty)
                        break;

                    notifyPacketEnqueuedScheduler();
                    break;
                }
                case PacketReadyNotification::None:
                    break;
            }
        });
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::notifyPacketEnqueuedOnThisThread()
{
    return wrapHandler(
        [this]
        {
            switch (notifyMethod)
            {
                case PacketReadyNotification::SameThread:
                case PacketReadyNotification::Scheduler:
                case PacketReadyNotification::SchedulerQueueWasEmpty:
                    notifyPacketEnqueuedSameThread();

                case PacketReadyNotification::None:
                    break;
            }
        });
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::setListener(IInputPortNotifications* port)
{
    std::scoped_lock lock(this->sync);

    listenerRef = port;

    if (listenerRef.assigned())
    {
        auto portRef = this->template getWeakRefInternal<IInputPort>();
        notifySchedulerCallback = Work([notifyRef = listenerRef, portRef = portRef, loggerComponent = loggerComponent]
        {
            auto notify = notifyRef.getRef();
            auto port = portRef.getRef();
            if (notify.assigned() && port.assigned())
            {
                try
                {
                    notify.packetReceived(port);
                }
                catch (const std::exception& e)
                {
                    LOG_E("Input port notification failed: {}", e.what());
                }
            }
        });
    }
    else
        notifySchedulerCallback.release();

    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::getCustomData(IBaseObject** data)
{
    OPENDAQ_PARAM_NOT_NULL(data);

    std::scoped_lock lock(this->sync);

    *data = this->customData.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::setCustomData(IBaseObject* data)
{
    std::scoped_lock lock(this->sync);
    this->customData = data;

    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::disconnectWithoutSignalNotification()
{
    return daqTry(
        [this]() -> auto
        {
            // disconnectWithoutSignalNotification is meant to be called from signal, so don't notify it
            disconnectSignalInternal(true, false);
            return OPENDAQ_SUCCESS;
        });
}

template <class... Interfaces>
void GenericInputPortImpl<Interfaces...>::finishUpdate()
{
    dummySignal.release();
    serializedSignalId.release();
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::remove()
{
    {
        std::scoped_lock lock(this->sync);

        if (isInputPortRemoved)
            return OPENDAQ_IGNORED;

        if (customData.assigned())
        {
            auto customDataRemovable = customData.asPtrOrNull<IRemovable>();
            if (customDataRemovable.assigned())
                customDataRemovable.remove();
        }

        isInputPortRemoved = true;
    }

    return daqTry(
        [this]() -> auto
        {
            // remove is meant to be called from listener, so don't notify it
            disconnectSignalInternal(false, true);
            return OPENDAQ_SUCCESS;
        });
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::isRemoved(Bool* removed)
{
    OPENDAQ_PARAM_NOT_NULL(removed);

    std::scoped_lock lock(this->sync);

    *removed = this->isInputPortRemoved;
    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode INTERFACE_FUNC GenericInputPortImpl<Interfaces...>::setOwner(IPropertyObject* owner)
{
    if (this->owner.assigned())
    {
        auto ref = this->owner.getRef();
        if (ref != nullptr && ref != owner)
            return this->makeErrorInfo(OPENDAQ_ERR_ALREADYEXISTS, "Owner is already assigned.");
    }
    this->owner = owner;
    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode INTERFACE_FUNC GenericInputPortImpl<Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode INTERFACE_FUNC GenericInputPortImpl<Interfaces...>::queryInterface(const IntfID& id, void** intf)
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == IInputPort::Id)
    {
        *intf = static_cast<IInputPort*>(this);
        this->addRef();

        return OPENDAQ_SUCCESS;
    }

    return Super::queryInterface(id, intf);
}

template <class... Interfaces>
ErrCode INTERFACE_FUNC GenericInputPortImpl<Interfaces...>::borrowInterface(const IntfID& id, void** intf) const
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == IInputPort::Id)
    {
        *intf = const_cast<IInputPort*>(static_cast<const IInputPort*>(this));

        return OPENDAQ_SUCCESS;
    }

    return Super::borrowInterface(id, intf);
}

template <class... Interfaces>
ConstCharPtr GenericInputPortImpl<Interfaces...>::SerializeId()
{
    return "InputPort";
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::Deserialize(ISerializedObject* serialized,
                                                         IBaseObject* context,
                                                         IFunction* factoryCallback,
                                                         IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializeComponent(
                       serialized,
                       context,
                       factoryCallback,
                       [](const SerializedObjectPtr&,
                          const ComponentDeserializeContextPtr& deserializeContext,
                          const StringPtr&)
                       {
                           return createWithImplementation<IInputPort, InputPortImpl>(
                               deserializeContext.getContext(), deserializeContext.getParent(), deserializeContext.getLocalId());
                       })
                       .detach();
        });
}

template <class... Interfaces>
void GenericInputPortImpl<Interfaces...>::serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate)
{
    Super::serializeCustomObjectValues(serializer, forUpdate);

    auto signal = getSignalNoLock();

    if (signal.assigned())
    {
        serializer.key("signalId");
        const auto signalGlobalId = signal.getGlobalId();
        serializer.writeString(signalGlobalId);
    }
}

template <class... Interfaces>
void GenericInputPortImpl<Interfaces...>::updateObject(const SerializedObjectPtr& obj)
{
    if (obj.hasKey("signalId"))
    {
        serializedSignalId = obj.readString("signalId");
        dummySignal = Signal(this->context, nullptr, "dummy");
        checkErrorInfo(connect(dummySignal));
    }
    else
        serializedSignalId.release();
}

template <class ... Interfaces>
void GenericInputPortImpl<Interfaces...>::onUpdatableUpdateEnd()
{
    Super::onUpdatableUpdateEnd();

    if (serializedSignalId.assigned() && serializedSignalId != "")
    {
        const auto thisPtr = this->template borrowPtr<InputPortPtr>();
        const auto root = this->getRootComponent(thisPtr);
        ComponentPtr sig;
        root->findComponent(serializedSignalId, &sig);
        if (sig.assigned())
        {
            try
            {
                thisPtr.connect(sig);
            }
            catch (const DaqException&)
            {
                LOG_W("Failed to connect signal: {}", serializedSignalId);
            }
        }
        else
        {
            LOG_W("Signal not found: {}", serializedSignalId);
        }
    }
    
    finishUpdate();
}

template <class ... Interfaces>
ComponentPtr GenericInputPortImpl<Interfaces...>::getRootComponent(const ComponentPtr& curComponent)
{
    const auto parent = curComponent.getParent();
    if (!parent.assigned())
        return curComponent;
    return getRootComponent(parent);
}

template <class... Interfaces>
BaseObjectPtr GenericInputPortImpl<Interfaces...>::getDeserializedParameter(const StringPtr& parameter)
{
    if (parameter == "signalId")
        return serializedSignalId;

    throw NotFoundException();
}

template <class... Interfaces>
void GenericInputPortImpl<Interfaces...>::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                                         const BaseObjectPtr& context,
                                                         const FunctionPtr& factoryCallback)
{
    Super::deserializeCustomObjectValues(serializedObject, context, factoryCallback);

    if (serializedObject.hasKey("signalId"))
    {
        serializedSignalId = serializedObject.readString("signalId");
        dummySignal = Signal(this->context, nullptr, "dummy");
        checkErrorInfo(connect(dummySignal));
    }
}

template <class ... Interfaces>
ConnectionPtr GenericInputPortImpl<Interfaces...>::createConnection(const SignalPtr& signal)
{
    const auto connection = Connection(this->template thisPtr<InputPortPtr>(), signal, this->context);
    return connection;
}

template <class ... Interfaces>
ConnectionPtr GenericInputPortImpl<Interfaces...>::getConnectionNoLock()
{
    if (!connectionRef.assigned())
        return nullptr;

    return connectionRef.getRef();
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::getRequiresSignal(Bool* requiresSignal)
{
    OPENDAQ_PARAM_NOT_NULL(requiresSignal);

    std::scoped_lock lock(this->sync);

    *requiresSignal = this->requiresSignal;
    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::setRequiresSignal(Bool requiresSignal)
{
    std::scoped_lock lock(this->sync);

    this->requiresSignal = requiresSignal;
    return OPENDAQ_SUCCESS;
}

template <class ... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::getGapCheckingEnabled(Bool* gapCheckingEnabled)
{
    OPENDAQ_PARAM_NOT_NULL(gapCheckingEnabled);

    *gapCheckingEnabled = this->gapCheckingEnabled;
    return OPENDAQ_SUCCESS;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(InputPortImpl)

END_NAMESPACE_OPENDAQ
