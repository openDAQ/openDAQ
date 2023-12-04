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
#include <opendaq/connection.h>
#include <opendaq/context_ptr.h>
#include <opendaq/component_impl.h>
#include <opendaq/input_port_config.h>
#include <opendaq/input_port_private.h>
#include <opendaq/input_port_notifications_ptr.h>
#include <opendaq/utility_sync.h>
#include <opendaq/removable.h>

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
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj);

protected:
    void serializeCustomObjectValues(const SerializerPtr& serializer) override;

    void updateObject(const SerializedObjectPtr& obj) override;

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

END_NAMESPACE_OPENDAQ
