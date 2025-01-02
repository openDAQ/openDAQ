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
#include <opendaq/reader_status.h>
#include <opendaq/block_reader_status.h>
#include <opendaq/tail_reader_status.h>
#include <opendaq/multi_reader_status.h>
#include <opendaq/event_packet_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <class MainInterface, class ... Interfaces>
class GenericReaderStatusImpl : public ImplementationOf<MainInterface, Interfaces...>
{
public:
    explicit GenericReaderStatusImpl(const EventPacketPtr& eventPacket, Bool valid, const NumberPtr& offset);

    virtual ErrCode INTERFACE_FUNC getReadStatus(ReadStatus* status) override;

    ErrCode INTERFACE_FUNC getEventPacket(IEventPacket** packet) override;

    ErrCode INTERFACE_FUNC getValid(Bool* valid) override;

    ErrCode INTERFACE_FUNC getOffset(INumber** offset) override;

private:
    EventPacketPtr eventPacket;
    Bool valid;
    NumberPtr offset;
};

using ReaderStatusImpl = GenericReaderStatusImpl<IReaderStatus>;

class BlockReaderStatusImpl final : public GenericReaderStatusImpl<IBlockReaderStatus>
{
public:
    using Super = GenericReaderStatusImpl<IBlockReaderStatus>;
    explicit BlockReaderStatusImpl(const EventPacketPtr& eventPacket, Bool valid, const NumberPtr& offset, SizeT readSamples);

    ErrCode INTERFACE_FUNC getReadSamples(SizeT* readSamples) override;

private:
    SizeT readSamples;
};

class TailReaderStatusImpl final : public GenericReaderStatusImpl<ITailReaderStatus>
{
public:
    using Super = GenericReaderStatusImpl<ITailReaderStatus>;
    explicit TailReaderStatusImpl(const EventPacketPtr& eventPacket, Bool valid, const NumberPtr& offset, Bool sufficientHistory);

    ErrCode INTERFACE_FUNC getReadStatus(ReadStatus* status) override;

    ErrCode INTERFACE_FUNC getSufficientHistory(Bool* status) override;
private:
    Bool sufficientHistory;
};

class MultiReaderStatusImpl final : public GenericReaderStatusImpl<IMultiReaderStatus>
{
public:
    using Super = GenericReaderStatusImpl<IMultiReaderStatus>;
    explicit MultiReaderStatusImpl(const EventPacketPtr& mainDescriptor, const DictPtr<IString, IEventPacket>& eventPackets, Bool valid, const NumberPtr& offset);

    ErrCode INTERFACE_FUNC getReadStatus(ReadStatus* status) override;

    ErrCode INTERFACE_FUNC getEventPackets(IDict** events) override;

    ErrCode INTERFACE_FUNC getEventPacket(IEventPacket** packet) override;

    ErrCode INTERFACE_FUNC getMainDescriptor(IEventPacket** descriptor) override;

private:
    DictPtr<IString, IEventPacket> eventPackets;
};

END_NAMESPACE_OPENDAQ