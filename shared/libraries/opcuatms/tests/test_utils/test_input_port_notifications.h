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
#include <opendaq/input_port_notifications_ptr.h>
#include <coretypes/weakrefobj.h>
#include <coretypes/impl.h>

class TestInputPortNotificationsImpl : public daq::ImplementationOfWeak<daq::IInputPortNotifications>
{
public:
    TestInputPortNotificationsImpl();

    daq::ErrCode INTERFACE_FUNC acceptsSignal(daq::IInputPort* port, daq::ISignal* signal, daq::Bool* accept) override;
    daq::ErrCode INTERFACE_FUNC connected(daq::IInputPort* port) override;
    daq::ErrCode INTERFACE_FUNC disconnected(daq::IInputPort* port) override;
    daq::ErrCode INTERFACE_FUNC packetReceived(daq::IInputPort* port) override;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(INLINE_FACTORY, TestInputPortNotifications, daq::IInputPortNotifications)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INLINE_FACTORY, TestInputPortNotifications, daq::IInputPortNotifications);

inline daq::InputPortNotificationsPtr TestInputPortNotifications()
{
    daq::InputPortNotificationsPtr inputPortNotification = TestInputPortNotifications_Create();
    return inputPortNotification;
}
