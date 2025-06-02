/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <opendaq/connection_internal.h>
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
#include <opendaq/signal_private_ptr.h>
#include <opendaq/work_factory.h>
#include <opendaq/scheduler_errors.h>
#include <opendaq/component_update_context_ptr.h>

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

    // IOwnable
    ErrCode INTERFACE_FUNC setOwner(IPropertyObject* owner) override;

    // IComponent
    ErrCode INTERFACE_FUNC getActive(Bool* active) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override;

    void updateObject(const SerializedObjectPtr& obj, const BaseObjectPtr& context) override;
    void onUpdatableUpdateEnd(const BaseObjectPtr& context) override;

    BaseObjectPtr getDeserializedParameter(const StringPtr& parameter) override;

    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

    virtual ConnectionPtr createConnection(const SignalPtr& signal);

    ConnectionPtr getConnectionNoLock();
    void removed() override;
    
    StringPtr serializedSignalId;

private:
    Bool requiresSignal;
    const bool gapCheckingEnabled;
    BaseObjectPtr customData;
    PacketReadyNotification notifyMethod{};

    WeakRefPtr<IInputPortNotifications> listenerRef;
    WeakRefPtr<IConnection> connectionRef{};
    WorkPtr notifySchedulerCallback;

    LoggerComponentPtr loggerComponent;
    SchedulerPtr scheduler;

    WeakRefPtr<IPropertyObject> owner;

    ErrCode canConnectSignal(ISignal* signal) const;
    void disconnectSignalInternal(ConnectionPtr&& connection, bool notifyListener, bool notifySignal, bool triggerCoreEvent);
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
{
    loggerComponent = context.getLogger().getOrAddComponent("InputPort");
    if (context.assigned())
        scheduler = context.getScheduler();
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::acceptsSignal(ISignal* signal, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(signal);

    const auto err = canConnectSignal(signal);
    OPENDAQ_RETURN_IF_FAILED(err);

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
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Removed signal cannot be connected");

    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::connect(ISignal* signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    try
    {
        auto err = canConnectSignal(signal);
        OPENDAQ_RETURN_IF_FAILED(err);

        auto signalPtr = SignalPtr::Borrow(signal);

        const auto connection = createConnection(signalPtr);

        InputPortNotificationsPtr inputPortListener;
        {
            auto lock = this->getRecursiveConfigLock();
            if (this->isComponentRemoved)
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Cannot connect signal to removed input port");

            {
                ConnectionPtr oldConnection = connectionRef.assigned() ? connectionRef.getRef() : nullptr;
                connectionRef.release();
                disconnectSignalInternal(std::move(oldConnection), false, true, false);
            }
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
                return DAQ_MAKE_ERROR_INFO(err);
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
    }
    catch (const DaqException& e)
    {
        return errorFromException(e, this->getThisAsBaseObject());
    }
    catch (const std::exception& e)
    {
        return DAQ_ERROR_FROM_STD_EXCEPTION(e, this->getThisAsBaseObject(), OPENDAQ_ERR_GENERALERROR);
    }
    catch (...)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR);
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
void GenericInputPortImpl<Interfaces...>::disconnectSignalInternal(ConnectionPtr&& connection, bool notifyListener, bool notifySignal, bool triggerCoreEvent)
{
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

    if (!this->coreEventMuted && this->coreEvent.assigned() && triggerCoreEvent)
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
            ConnectionPtr connection;
            {
                auto lock = this->getRecursiveConfigLock();
                connection = getConnectionNoLock();
                connectionRef.release();
            }

            disconnectSignalInternal(std::move(connection), true, true, true);
            return OPENDAQ_SUCCESS;
        });
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::getSignal(ISignal** signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    auto lock = this->getRecursiveConfigLock();

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
    OPENDAQ_PARAM_NOT_NULL(connection);

    auto lock = this->getRecursiveConfigLock();

    return daqTry([this, &connection] { *connection = getConnectionNoLock().detach(); });
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::setNotificationMethod(PacketReadyNotification method)
{
    auto lock = this->getRecursiveConfigLock();

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
    const auto errCode = scheduler->scheduleWork(notifySchedulerCallback);
    if (OPENDAQ_FAILED(errCode) && (errCode != OPENDAQ_ERR_SCHEDULER_STOPPED))
        checkErrorInfo(errCode);
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
    auto lock = this->getRecursiveConfigLock();

    if (auto connection = getConnectionNoLock(); connection.assigned())
    {
        connection.template as<IConnectionInternal>(true)->enqueueLastDescriptor();
    }

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

    auto lock = this->getRecursiveConfigLock();

    *data = this->customData.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::setCustomData(IBaseObject* data)
{
    auto lock = this->getRecursiveConfigLock();
    this->customData = data;

    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::disconnectWithoutSignalNotification()
{
    return daqTry(
        [this]() -> auto
        {
            ConnectionPtr connection;
            {
                auto lock = this->getRecursiveConfigLock();
                connection = getConnectionNoLock();
                connectionRef.release();
            }

            // disconnectWithoutSignalNotification is meant to be called from signal, so don't notify it
            disconnectSignalInternal(std::move(connection), true, false, true);
            return OPENDAQ_SUCCESS;
        });
}

template <class... Interfaces>
void GenericInputPortImpl<Interfaces...>::finishUpdate()
{
    serializedSignalId.release();
}

template <class... Interfaces>
void GenericInputPortImpl<Interfaces...>::removed()
{
    if (customData.assigned())
    {
        auto customDataRemovable = customData.asPtrOrNull<IRemovable>();
        if (customDataRemovable.assigned())
            customDataRemovable.remove();
    }

    ConnectionPtr connection = getConnectionNoLock();
    connectionRef.release();

    // remove is meant to be called from listener, so don't notify it
    disconnectSignalInternal(std::move(connection), false, true, true);
}

template <class... Interfaces>
ErrCode INTERFACE_FUNC GenericInputPortImpl<Interfaces...>::setOwner(IPropertyObject* owner)
{
    if (this->owner.assigned())
    {
        auto ref = this->owner.getRef();
        if (ref != nullptr && ref != owner)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ALREADYEXISTS, "Owner is already assigned.");
    }
    this->owner = owner;
    return OPENDAQ_SUCCESS;
}

template <class ... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::getActive(Bool* active)
{
    OPENDAQ_PARAM_NOT_NULL(active);

    auto lock = this->getAcquisitionLock();

    *active = this->active;
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
        const auto signalSerializedId = signal.template asPtr<ISignalPrivate>(true).getSignalSerializeId();
        serializer.writeString(signalSerializedId);
    }
}

template <class... Interfaces>
void GenericInputPortImpl<Interfaces...>::updateObject(const SerializedObjectPtr& obj, const BaseObjectPtr& context)
{
    if (obj.hasKey("signalId"))
    {
        ComponentUpdateContextPtr contextPtr = context.asPtr<IComponentUpdateContext>(true);
        ComponentPtr parent;
        this->getParent(&parent);
        StringPtr parentId = parent.assigned() ? parent.getGlobalId() : "";
        contextPtr.setInputPortConnection(parentId, this->localId, obj.readString("signalId"));
    }
    else
    {
        serializedSignalId.release();
    }
}

template <class ... Interfaces>
void GenericInputPortImpl<Interfaces...>::onUpdatableUpdateEnd(const BaseObjectPtr& context)
{
    if (this->getSignalNoLock().assigned())
        return;

    auto contextPtr = context.asPtr<IComponentUpdateContext>(true);
    ComponentPtr parent;
    this->getParent(&parent);
    StringPtr parentId = parent.assigned() ? parent.getGlobalId() : "";
    auto signal = contextPtr.getSignal(parentId, this->localId);

    if (signal.assigned())
    {
        try
        {
            const auto thisPtr = this->template borrowPtr<InputPortPtr>();
            thisPtr.connect(signal);
            finishUpdate();
        }
        catch (const DaqException&)
        {
            LOG_W("Failed to connect signal: {}", signal.getGlobalId());
        }
    }
    Super::onUpdatableUpdateEnd(context);
}

template <class... Interfaces>
BaseObjectPtr GenericInputPortImpl<Interfaces...>::getDeserializedParameter(const StringPtr& parameter)
{
    if (parameter == "signalId")
        return serializedSignalId;

    DAQ_THROW_EXCEPTION(NotFoundException);
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

    auto lock = this->getRecursiveConfigLock();

    *requiresSignal = this->requiresSignal;
    return OPENDAQ_SUCCESS;
}

template <class... Interfaces>
ErrCode GenericInputPortImpl<Interfaces...>::setRequiresSignal(Bool requiresSignal)
{
    auto lock = this->getRecursiveConfigLock();

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
