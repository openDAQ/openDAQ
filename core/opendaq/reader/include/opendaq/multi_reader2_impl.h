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
#include <coreobjects/property_object_ptr.h>
#include <coretypes/event_args_ptr.h>
#include <coretypes/event_emitter.h>
#include <coretypes/intfs.h>
#include <opendaq/context_ptr.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/input_port_notifications.h>
#include <opendaq/multi_reader2.h>

#include <mutex>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

class MultiReader2Impl : public ImplementationOfWeak<IMultiReader2, IInputPortNotifications>
{
public:
    explicit MultiReader2Impl(IMultiReader2Params* params);

    // IMultiReader2
    ErrCode INTERFACE_FUNC configure(IMultiReader2Params* params) override;
    ErrCode INTERFACE_FUNC getMainInput(IString** inputId) override;
    ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC read(IMultiReader2Status** status, void** data, SizeT* count, SizeT* packetOffset) override;
    ErrCode INTERFACE_FUNC readWithDomain(IMultiReader2Status** status, void** data, SizeT* count) override;
    ErrCode INTERFACE_FUNC commitEvent() override;
    ErrCode INTERFACE_FUNC setUsed(IString* inputId, Bool used) override;
    ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC getOnConnected(IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnDisconnected(IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnDataAvailable(IEvent** event) override;

    // IInputPortNotifications
    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept) override;
    ErrCode INTERFACE_FUNC connected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC disconnected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override;

private:
    // One reader input: the port driving it plus the global id of the added component
    struct Slot
    {
        InputPortConfigPtr port;
        StringPtr inputId;
        bool ownsPort;
        bool used = true;
    };

    std::vector<Slot>::iterator findSlot(const StringPtr& inputId);
    StringPtr findSlotId(IInputPort* port);
    ErrCode addInputComponent(const ComponentPtr& component);
    void detachSlot(Slot& slot);

    std::mutex mutex;
    EventEmitter<InputPortPtr, EventArgsPtr<>> onConnected;
    EventEmitter<InputPortPtr, EventArgsPtr<>> onDisconnected;
    EventEmitter<InputPortPtr, EventArgsPtr<>> onDataAvailable;
    ContextPtr context;
    PropertyObjectPtr portBinder;
    std::vector<Slot> slots;
    StringPtr mainInputId;
    SizeT minReadCount = 1;
    Bool requireSameRates = False;
    Bool active = True;
    bool eventPending = false;
};

END_NAMESPACE_OPENDAQ
