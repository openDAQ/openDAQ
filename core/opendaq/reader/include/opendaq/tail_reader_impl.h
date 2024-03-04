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
#include <opendaq/reader_impl.h>
#include <opendaq/tail_reader.h>
#include <opendaq/reader_config_ptr.h>
#include <opendaq/data_packet_ptr.h>

#include <deque>

BEGIN_NAMESPACE_OPENDAQ

struct TailReaderInfo;

extern template class ReaderImpl<ITailReader>;

class TailReaderImpl final : public ReaderImpl<ITailReader>
{
public:
    using Super = ReaderImpl<ITailReader>;

    TailReaderImpl(ISignal* signal,
                   SizeT historySize,
                   SampleType valueReadType,
                   SampleType domainReadType,
                   ReadMode mode);

    TailReaderImpl(IInputPortConfig* port,
                   SizeT historySize,
                   SampleType valueReadType,
                   SampleType domainReadType,
                   ReadMode mode);

    TailReaderImpl(const ReaderConfigPtr& readerConfig,
                   SampleType valueReadType,
                   SampleType domainReadType,
                   SizeT historySize,
                   ReadMode mode);

    TailReaderImpl(TailReaderImpl* old,
                   SampleType valueReadType,
                   SampleType domainReadType,
                   SizeT historySize);

    ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC getHistorySize(SizeT* size) override;

    ErrCode INTERFACE_FUNC read(void* values, SizeT* count, IReaderStatus** status) override;
    ErrCode INTERFACE_FUNC readWithDomain(void* values, void* domain, SizeT* count, IReaderStatus** status) override;

    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override;

private:
    ErrCode readPacket(TailReaderInfo& info, const DataPacketPtr& packet);
    ErrCode readData(TailReaderInfo& info, IReaderStatus** status);

private:
    SizeT historySize;

    SizeT cachedSamples;
    std::deque<PacketPtr> packets;
};

END_NAMESPACE_OPENDAQ
