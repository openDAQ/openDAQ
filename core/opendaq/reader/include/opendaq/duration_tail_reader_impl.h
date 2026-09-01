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

#include <opendaq/data_packet_ptr.h>
#include <opendaq/duration_tail_reader.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/reader_config_ptr.h>
#include <opendaq/reader_impl.h>
#include <opendaq/tail_reader_status_ptr.h>

#include <deque>
#include <optional>

BEGIN_NAMESPACE_OPENDAQ

struct DurationTailReaderInfo;

extern template class ReaderImpl<IDurationTailReader>;

class DurationTailReaderImpl final : public ReaderImpl<IDurationTailReader>
{
public:
    using Super = ReaderImpl<IDurationTailReader>;

    DurationTailReaderImpl(ISignal* signal,
                           UInt historyDurationMs,
                           SampleType valueReadType,
                           SampleType domainReadType,
                           ReadMode mode,
                           Bool skipEvents = false);

    DurationTailReaderImpl(IInputPortConfig* port,
                           UInt historyDurationMs,
                           SampleType valueReadType,
                           SampleType domainReadType,
                           ReadMode mode,
                           Bool skipEvents = false);

    ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC getEmpty(Bool* empty) override;

    ErrCode INTERFACE_FUNC read(void* values, SizeT* count, ITailReaderStatus** status) override;
    ErrCode INTERFACE_FUNC readWithDomain(void* values,
                                          void* domain,
                                          SizeT* count,
                                          ITailReaderStatus** status) override;

    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override;

    ErrCode INTERFACE_FUNC getHistoryDurationMs(UInt* milliseconds) override;
    ErrCode INTERFACE_FUNC setHistoryDurationMs(UInt milliseconds) override;

private:
    ErrCode readPacket(DurationTailReaderInfo& info, const DataPacketPtr& dataPacket);
    TailReaderStatusPtr readData(DurationTailReaderInfo& info);

    // Returns the packet's domain start time in milliseconds, matching the unit
    // historyDurationMs is stored in so trimOldPackets()/hasSufficientHistory()
    // never need to convert between units.
    std::optional<Float> getPacketStartMs(const PacketPtr& packet) const;
    void trimOldPackets();
    Bool hasSufficientHistory() const;

    UInt historyDurationMs;
    std::deque<PacketPtr> packets;
    // Kept in sync with `packets`: incremented as data packets are appended in
    // packetReceived(), decremented as they are dropped in trimOldPackets()/on the
    // packets.clear() path, so getAvailableCount()/read() don't need to rescan the buffer.
    SizeT bufferedSampleCount = 0;
};

END_NAMESPACE_OPENDAQ
