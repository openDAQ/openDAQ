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
#include <opendaq/connection_factory.h>
#include <opendaq/context_ptr.h>
#include <opendaq/component_impl.h>
#include <opendaq/input_port_config.h>
#include <opendaq/input_port_private.h>
#include <opendaq/input_port_notifications_ptr.h>
#include <opendaq/utility_sync.h>
#include <opendaq/removable_ptr.h>
#include <opendaq/signal_errors.h>
#include <opendaq/signal_events_ptr.h>
#include <opendaq/signal_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class InputPortImpl : public ComponentImpl<IInputPortConfig, IInputPortPrivate>
{
public:
    using Super = ComponentImpl<IInputPortConfig, IInputPortPrivate>;

    explicit InputPortImpl(const ContextPtr& context,
                           const ComponentPtr& parent,
                           const StringPtr& localId,
                           const StringPtr& className = nullptr,
                           ComponentStandardProps propsMode = ComponentStandardProps::Add);

    ErrCode INTERFACE_FUNC acceptsSignal(ISignal* signal, Bool* accepts) override;
    ErrCode INTERFACE_FUNC connect(ISignal* signal) override;
    ErrCode INTERFACE_FUNC disconnect() override;
    ErrCode INTERFACE_FUNC getSignal(ISignal** signal) override;
    ErrCode INTERFACE_FUNC getConnection(IConnection** connection) override;
    ErrCode INTERFACE_FUNC getRequiresSignal(Bool* requiresSignal) override;

    ErrCode INTERFACE_FUNC setNotificationMethod(PacketReadyNotification method) override;
    ErrCode INTERFACE_FUNC notifyPacketEnqueued() override;
    ErrCode INTERFACE_FUNC notifyPacketEnqueuedOnThisThread() override;
    ErrCode INTERFACE_FUNC setListener(IInputPortNotifications* port) override;

    ErrCode INTERFACE_FUNC getCustomData(IBaseObject** data) override;
    ErrCode INTERFACE_FUNC setCustomData(IBaseObject* data) override;
    ErrCode INTERFACE_FUNC setRequiresSignal(Bool requiresSignal) override;

    // IInputPortPrivate
    ErrCode INTERFACE_FUNC disconnectWithoutSignalNotification() override;
    ErrCode INTERFACE_FUNC getSerializedSignalId(IString** serializedSignalId) override;
    ErrCode INTERFACE_FUNC finishUpdate() override;

    // IRemovable
    ErrCode INTERFACE_FUNC remove() override;
    ErrCode INTERFACE_FUNC isRemoved(Bool* removed) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override;

    void updateObject(const SerializedObjectPtr& obj) override;

    BaseObjectPtr getDeserializedParameter(const StringPtr& parameter) override;

    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

private:
    Bool requiresSignal;
    BaseObjectPtr customData;
    PacketReadyNotification notifyMethod{};

    WeakRefPtr<IInputPortNotifications> listenerRef;
    WeakRefPtr<IConnection> connectionRef{};
    bool isInputPortRemoved;
    FunctionPtr notifySchedulerCallback;

    LoggerComponentPtr loggerComponent;
    SchedulerPtr scheduler;

    StringPtr serializedSignalId;
    SignalPtr dummySignal;

    ErrCode canConnectSignal(ISignal* signal) const;
    void disconnectSignalInternal(bool notifyListener, bool notifySignal);
    void notifyPacketEnqueuedSameThread();
    void notifyPacketEnqueuedScheduler();

    SignalPtr getSignalNoLock();
};

inline InputPortImpl::InputPortImpl(const ContextPtr& context,
                             const ComponentPtr& parent,
                             const StringPtr& localId,
                             const StringPtr& className,
                             const ComponentStandardProps propsMode)
    : Super(context, parent, localId, className, propsMode)
    , requiresSignal(true)
    , notifyMethod(PacketReadyNotification::None)
    , listenerRef(nullptr)
    , connectionRef(nullptr)
    , isInputPortRemoved(false)
{
    loggerComponent = context.getLogger().getOrAddComponent("InputPort");
    if (context.assigned())
        scheduler = context.getScheduler();
}

inline ErrCode InputPortImpl::acceptsSignal(ISignal* signal, Bool* accepts)
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
            return listener->acceptsSignal(borrowInterface(), signal, accepts);
    }

    *accepts = true;
    return OPENDAQ_SUCCESS;
}

inline ErrCode InputPortImpl::canConnectSignal(ISignal* signal) const
{
    const auto removablePtr = SignalPtr::Borrow(signal).asPtrOrNull<IRemovable>();
    if (removablePtr.assigned() && removablePtr.isRemoved())
        return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Removed signal cannot be connected");

    return OPENDAQ_SUCCESS;
}

inline ErrCode InputPortImpl::connect(ISignal* signal)
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

        const auto connection = Connection(thisPtr<InputPortPtr>(), signalPtr, context);

        InputPortNotificationsPtr inputPortListener;
        {
            std::scoped_lock lock(sync);
            if (isInputPortRemoved)
                return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Cannot connect signal to removed input port");

            connectionRef = connection;

            if (listenerRef.assigned())
                inputPortListener = listenerRef.getRef();
        }

        if (inputPortListener.assigned())
        {
            err = inputPortListener->connected(borrowInterface());
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

    return OPENDAQ_SUCCESS;
}

inline void InputPortImpl::disconnectSignalInternal(bool notifyListener, bool notifySignal)
{
    ConnectionPtr connection;
    {
        std::scoped_lock lock(sync);

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
                listener->disconnected(borrowInterface());
        }
    }
}

inline ErrCode InputPortImpl::disconnect()
{
    return daqTry(
        [this]() -> auto
        {
            disconnectSignalInternal(true, true);
            return OPENDAQ_SUCCESS;
        });
}

inline ErrCode InputPortImpl::getSignal(ISignal** signal)
{
    if (signal == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::scoped_lock lock(sync);

    *signal = getSignalNoLock().detach();

    return OPENDAQ_SUCCESS;
}

inline SignalPtr InputPortImpl::getSignalNoLock()
{
    if (!connectionRef.assigned())
        return nullptr;

    const auto connection = connectionRef.getRef();
    return connection.assigned() ? connection.getSignal() : nullptr;
}

inline ErrCode InputPortImpl::getConnection(IConnection** connection)
{
    if (connection == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::scoped_lock lock(sync);

    if (!connectionRef.assigned())
        *connection = nullptr;
    else
        *connection = connectionRef.getRef().detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode InputPortImpl::setNotificationMethod(PacketReadyNotification method)
{
    std::scoped_lock lock(sync);

    if (method == PacketReadyNotification::Scheduler && !scheduler.assigned())
    {
        LOG_W("Scheduler based notification not available");
        notifyMethod = PacketReadyNotification::SameThread;
    }
    else
        notifyMethod = method;

    return OPENDAQ_SUCCESS;
}

inline void InputPortImpl::notifyPacketEnqueuedSameThread()
{
    if (listenerRef.assigned())
    {
        auto listener = listenerRef.getRef();
        if (listener.assigned())
        {
            try
            {
                listener.packetReceived(thisInterface());
            }
            catch (const std::exception& e)
            {
                LOG_E("Input port notification failed: {}", e.what());
            }
        }
    }
}

inline void InputPortImpl::notifyPacketEnqueuedScheduler()
{
    scheduler.scheduleWork(notifySchedulerCallback);
}

inline ErrCode InputPortImpl::notifyPacketEnqueued()
{
    return wrapHandler(
        [this]
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
                case PacketReadyNotification::None:
                    break;
            }
        });
}

inline ErrCode InputPortImpl::notifyPacketEnqueuedOnThisThread()
{
    return wrapHandler(
        [this]
        {
            switch (notifyMethod)
            {
                case PacketReadyNotification::SameThread:
                case PacketReadyNotification::Scheduler:
                    notifyPacketEnqueuedSameThread();

                case PacketReadyNotification::None:
                    break;
            }
        });
}

inline ErrCode InputPortImpl::setListener(IInputPortNotifications* port)
{
    std::scoped_lock lock(sync);

    listenerRef = port;

    if (listenerRef.assigned())
    {
        auto portRef = this->getWeakRefInternal<IInputPort>();
        notifySchedulerCallback = [notifyRef = listenerRef, portRef = portRef, loggerComponent = loggerComponent]
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
                return true;
            }
            return false;
        };
    }
    else
        notifySchedulerCallback.release();

    return OPENDAQ_SUCCESS;
}

inline ErrCode InputPortImpl::getCustomData(IBaseObject** data)
{
    OPENDAQ_PARAM_NOT_NULL(data);

    std::scoped_lock lock(sync);

    *data = this->customData.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

inline ErrCode InputPortImpl::setCustomData(IBaseObject* data)
{
    std::scoped_lock lock(sync);
    this->customData = data;

    return OPENDAQ_SUCCESS;
}

inline ErrCode InputPortImpl::disconnectWithoutSignalNotification()
{
    return daqTry(
        [this]() -> auto
        {
            // disconnectWithoutSignalNotification is meant to be called from signal, so don't notify it
            disconnectSignalInternal(true, false);
            return OPENDAQ_SUCCESS;
        });
}

inline ErrCode INTERFACE_FUNC InputPortImpl::getSerializedSignalId(IString** serializedSignalId)
{
    OPENDAQ_PARAM_NOT_NULL(serializedSignalId);
    std::scoped_lock lock(sync);

    *serializedSignalId = this->serializedSignalId.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

inline ErrCode InputPortImpl::finishUpdate()
{
    dummySignal.release();
    serializedSignalId.release();
    return OPENDAQ_SUCCESS;
}

inline ErrCode InputPortImpl::remove()
{
    {
        std::scoped_lock lock(sync);

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

inline ErrCode InputPortImpl::isRemoved(Bool* removed)
{
    OPENDAQ_PARAM_NOT_NULL(removed);

    std::scoped_lock lock(sync);

    *removed = this->isInputPortRemoved;
    return OPENDAQ_SUCCESS;
}

inline ErrCode INTERFACE_FUNC InputPortImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

inline ConstCharPtr InputPortImpl::SerializeId()
{
    return "InputPort";
}

inline ErrCode InputPortImpl::Deserialize(ISerializedObject* serialized,
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
                       [](const SerializedObjectPtr& serialized,
                          const ComponentDeserializeContextPtr& deserializeContext,
                          const StringPtr& className)
                       {
                           return createWithImplementation<IInputPort, InputPortImpl>(
                               deserializeContext.getContext(), deserializeContext.getParent(), deserializeContext.getLocalId(), className);
                       })
                       .detach();
        });
}

inline void InputPortImpl::serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate)
{
    Super::serializeCustomObjectValues(serializer, forUpdate);

    auto signal = getSignalNoLock();

    if (signal.assigned())
    {
        serializer.key("signalId");
        const auto signalGlobalId = getRelativeGlobalId(signal.getGlobalId());
        serializer.writeString(signalGlobalId.c_str(), signalGlobalId.size());
    }
}

inline void InputPortImpl::updateObject(const SerializedObjectPtr& obj)
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

inline BaseObjectPtr InputPortImpl::getDeserializedParameter(const StringPtr& parameter)
{
    if (parameter == "signalId")
        return serializedSignalId;

    throw NotFoundException();
}

inline void InputPortImpl::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
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

inline ErrCode InputPortImpl::getRequiresSignal(Bool* requiresSignal)
{
    OPENDAQ_PARAM_NOT_NULL(requiresSignal);

    std::scoped_lock lock(sync);

    *requiresSignal = this->requiresSignal;
    return OPENDAQ_SUCCESS;
}

inline ErrCode InputPortImpl::setRequiresSignal(Bool requiresSignal)
{
    std::scoped_lock lock(sync);

    this->requiresSignal = requiresSignal;
    return OPENDAQ_SUCCESS;
}


OPENDAQ_REGISTER_DESERIALIZE_FACTORY(InputPortImpl)

END_NAMESPACE_OPENDAQ
