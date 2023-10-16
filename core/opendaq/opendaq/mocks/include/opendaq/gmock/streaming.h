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
#include <opendaq/streaming_impl.h>
#include <opendaq/streaming.h>
#include <opendaq/streaming_ptr.h>
#include <gmock/gmock.h>
#include <coretypes/gmock/mock_ptr.h>

struct MockStreaming : daq::Streaming
{
    typedef MockPtr<
        daq::IStreaming,
        daq::StreamingPtr,
        MockStreaming
    > Strict;

    MOCK_METHOD(void, onSetActive, (bool active), (override));
    MOCK_METHOD(daq::StringPtr, onAddSignal, (const daq::SignalRemotePtr& signal), (override));
    MOCK_METHOD(void, onRemoveSignal, (const daq::SignalRemotePtr& signal), (override));

    MockStreaming() : daq::Streaming("MockStreaming", nullptr)
    {
        using namespace testing;

        ON_CALL(*this, onSetActive)
            .WillByDefault(DoAll(
                Invoke([&](bool active) {})
            ));

        ON_CALL(*this, onAddSignal)
            .WillByDefault(DoAll(
                Invoke([&](const daq::SignalRemotePtr& signal) { return signal.getRemoteId(); })
            ));

        ON_CALL(*this, onRemoveSignal)
            .WillByDefault(DoAll(
                Invoke([&](const daq::SignalRemotePtr& signal) {})
            ));
    }
};
