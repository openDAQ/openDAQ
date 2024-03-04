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
#include <opendaq/sample_type.h>
#include <opendaq/signal_ptr.h>
#include <opendaq/stream_reader.h>
#include <opendaq/reader_config_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/typed_reader.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/read_info.h>
#include <coreobjects/property_object_factory.h>

#include <condition_variable>

BEGIN_NAMESPACE_OPENDAQ


class StreamReaderImpl final : public ImplementationOfWeak<IStreamReader, IReaderConfig, IInputPortNotifications>
{
public:
    explicit StreamReaderImpl(const SignalPtr& signal,
                              SampleType valueReadType,
                              SampleType domainReadType,
                              ReadMode mode,
                              ReadTimeoutType timeoutType);
    
    explicit StreamReaderImpl(IInputPortConfig* port,
                              SampleType valueReadType,
                              SampleType domainReadType,
                              ReadMode mode,
                              ReadTimeoutType timeoutType);

    explicit StreamReaderImpl(const ReaderConfigPtr& readerConfig,
                              SampleType valueReadType,
                              SampleType domainReadType,
                              ReadMode readMode);

    explicit StreamReaderImpl(StreamReaderImpl* old,
                              SampleType valueReadType,
                              SampleType domainReadType);
    
    ~StreamReaderImpl() override;

    // IReader
    ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC setOnDataAvailable(IProcedure* callback) override;

    // ISampleReader
    ErrCode INTERFACE_FUNC getValueReadType(SampleType* sampleType) override;
    ErrCode INTERFACE_FUNC getDomainReadType(SampleType* sampleType) override;

    ErrCode INTERFACE_FUNC setValueTransformFunction(IFunction* transform) override;
    ErrCode INTERFACE_FUNC setDomainTransformFunction(IFunction* transform) override;

    ErrCode INTERFACE_FUNC getReadMode(ReadMode* mode) override;

    // StreamReader
    ErrCode INTERFACE_FUNC read(void* samples, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr) override;

    ErrCode INTERFACE_FUNC readWithDomain(void* samples,
                                          void* domain,
                                          SizeT* count,
                                          SizeT timeoutMs = 0,
                                          IReaderStatus** status = nullptr) override;

    // IReaderConfig
    ErrCode INTERFACE_FUNC getValueTransformFunction(IFunction** transform) override;
    ErrCode INTERFACE_FUNC getDomainTransformFunction(IFunction** transform) override;
    ErrCode INTERFACE_FUNC getInputPorts(IList** ports) override;

    ErrCode INTERFACE_FUNC getReadTimeoutType(ReadTimeoutType* timeout) override;
    ErrCode INTERFACE_FUNC markAsInvalid() override;

    // IInputPortNotifications
    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept) override;
    ErrCode INTERFACE_FUNC connected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC disconnected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override;

private:
    void readDescriptorFromPort();
    void connectSignal(const SignalPtr& signal);
    void inferReaderReadType(const DataDescriptorPtr& newDescriptor, std::unique_ptr<Reader>& reader) const;

    ErrCode onPacketReady();

    void handleDescriptorChanged(const EventPacketPtr& eventPacket);

    [[nodiscard]]
    bool trySetDomainSampleType(const daq::DataPacketPtr& domainPacket);

    ErrCode readPackets(IReaderStatus** status);
    ErrCode readPacketData();

    ReadInfo info{};
    NotifyInfo notify{};
    std::unique_ptr<Reader> valueReader;
    std::unique_ptr<Reader> domainReader;

    DataDescriptorPtr dataDescriptor;

    ReadMode readMode;
    ReadTimeoutType timeoutType;
    InputPortConfigPtr inputPort;
    PropertyObjectPtr portBinder;
    ConnectionPtr connection;

    bool invalid{};

    std::mutex mutex;
    ProcedurePtr readCallback;
};

END_NAMESPACE_OPENDAQ
