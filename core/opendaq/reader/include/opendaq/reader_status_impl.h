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
#include <opendaq/reader_status.h>
#include <opendaq/event_packet_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ReaderStatusImpl final : public ImplementationOf<IReaderStatus>
{
public:
    explicit ReaderStatusImpl(const EventPacketPtr& eventPacket, Bool valid);

    ErrCode INTERFACE_FUNC getReadStatus(ReadStatus* status) override;

    ErrCode INTERFACE_FUNC getEventPacket(IEventPacket** packet) override;

    ErrCode INTERFACE_FUNC getValid(Bool* valid) override;

private:
    EventPacketPtr eventPacket;
    Bool valid;
};

END_NAMESPACE_OPENDAQ