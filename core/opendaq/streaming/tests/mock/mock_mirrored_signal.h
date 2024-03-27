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
#include <opendaq/mirrored_signal_impl.h>

class MockMirroredSignalImpl : public daq::MirroredSignal
{
public:
    explicit MockMirroredSignalImpl(const daq::ContextPtr& ctx,
                                    const daq::ComponentPtr& parent,
                                    const daq::StringPtr& localId);

    daq::StringPtr onGetRemoteId() const override;
    daq::Bool onTriggerEvent(const daq::EventPacketPtr& eventPacket) override;

private:
    daq::StringPtr streamingId;
};
