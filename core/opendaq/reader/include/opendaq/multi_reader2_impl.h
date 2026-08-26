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
    explicit MultiReader2Impl(MultiReader2InputType inputType);

    // IMultiReader2
    ErrCode INTERFACE_FUNC addInput(IComponent* input) override;
    ErrCode INTERFACE_FUNC removeInput(IComponent* input) override;

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
    };

    std::vector<Slot>::iterator findSlot(const StringPtr& inputId);

    std::mutex mutex;
    MultiReader2InputType inputType;
    ContextPtr context;
    PropertyObjectPtr portBinder;
    std::vector<Slot> slots;
};

END_NAMESPACE_OPENDAQ
