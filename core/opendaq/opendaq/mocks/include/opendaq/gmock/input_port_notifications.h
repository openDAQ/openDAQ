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
#include <opendaq/input_port_notifications_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/weakrefobj.h>
#include <gmock/gmock.h>
#include <coretypes/gmock/mock_ptr.h>

struct MockInputPortNotifications : daq::ImplementationOfWeak<daq::IInputPortNotifications>
{
    typedef MockPtr<
        daq::IInputPortNotifications,
        daq::InputPortNotificationsPtr,
        MockInputPortNotifications
    > Strict;

    MOCK_METHOD(daq::ErrCode, acceptsSignal, (daq::IInputPort* port, daq::ISignal* signal, daq::Bool* accept), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, connected, (daq::IInputPort* port), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, disconnected, (daq::IInputPort* port), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, packetReceived, (daq::IInputPort* port), (override MOCK_CALL));

    MockInputPortNotifications()
    {
        using namespace testing;

        EXPECT_CALL(*this, connected)
            .Times(AnyNumber())
            .WillRepeatedly(DoAll(
                Return(OPENDAQ_SUCCESS)
            ));

        EXPECT_CALL(*this, packetReceived)
            .Times(AnyNumber())
            .WillRepeatedly(DoAll(
                Return(OPENDAQ_SUCCESS)
            ));
        
        EXPECT_CALL(*this, acceptsSignal)
            .Times(AnyNumber()).WillRepeatedly(
                DoAll(Invoke([](daq::IInputPort*, daq::ISignal*, daq::Bool* accepted) {
                    *accepted = daq::True;
                    return OPENDAQ_SUCCESS;
                }),
                Return(OPENDAQ_SUCCESS)
            ));
    }
};
