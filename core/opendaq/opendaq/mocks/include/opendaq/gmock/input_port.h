/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/input_port_config_ptr.h>
#include <coretypes/intfs.h>
#include <gmock/gmock.h>
#include <coretypes/gmock/mock_ptr.h>
#include <opendaq/gmock/component.h>

struct MockInputPort : daq::MockGenericComponent<MockInputPort, daq::IInputPortConfig>
{
    typedef MockPtr<
        daq::IInputPortConfig,
        daq::InputPortConfigPtr,
        MockInputPort
    > Strict;

    MOCK_METHOD(daq::ErrCode, acceptsSignal, (daq::ISignal* signal, daq::Bool* accepts), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, connect, (daq::ISignal* signal), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, disconnect, (), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getSignal, (daq::ISignal** signal), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getConnection, (daq::IConnection** connection), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getRequiresSignal, (daq::Bool* value), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setRequiresSignal, (daq::Bool value), (override MOCK_CALL));

    MOCK_METHOD(daq::ErrCode, setNotificationMethod, (daq::PacketReadyNotification method), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, notifyPacketEnqueued, (daq::Bool value), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, notifyPacketEnqueuedOnThisThread, (), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setListener, (daq::IInputPortNotifications * port), (override MOCK_CALL));

    MOCK_METHOD(daq::ErrCode, getCustomData, (daq::IBaseObject** customData), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setCustomData, (daq::IBaseObject* customData), (override MOCK_CALL));

    MOCK_METHOD(daq::ErrCode, getGapCheckingEnabled, (daq::Bool* gapCheckingEnabled), (override MOCK_CALL));

    daq::Bool active = true;

    MockInputPort()
    {
        using namespace testing;

        EXPECT_CALL(*this, notifyPacketEnqueued).Times(AnyNumber()).WillRepeatedly(DoAll(Return(OPENDAQ_SUCCESS)));

        EXPECT_CALL(*this, getActive)
            .Times(AnyNumber())
            .WillRepeatedly(DoAll(Invoke([&](daq::Bool* active) { *active = this->active; }), Return(OPENDAQ_SUCCESS)));
    }
};
