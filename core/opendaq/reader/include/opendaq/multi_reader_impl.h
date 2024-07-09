/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/multi_reader.h>
#include <opendaq/read_info.h>
#include <opendaq/reader_config_ptr.h>
#include <opendaq/signal_reader.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/multi_reader_builder_ptr.h>
#include <opendaq/multi_reader_status_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class MultiReaderImpl : public ImplementationOfWeak<IMultiReader, IReaderConfig, IInputPortNotifications>
{
public:
    MultiReaderImpl(const ListPtr<IComponent>& list,
                    SampleType valueReadType,
                    SampleType domainReadType,
                    ReadMode mode,
                    ReadTimeoutType timeoutType,
                    Int requiredCommonSampleRate = -1,
                    Bool startOnFullUnitOfDomain = false);

    MultiReaderImpl(MultiReaderImpl* old,
                    SampleType valueReadType,
                    SampleType domainReadType);

    MultiReaderImpl(const ReaderConfigPtr& readerConfig,
                    SampleType valueReadType,
                    SampleType domainReadType,
                    ReadMode mode);

    MultiReaderImpl(const MultiReaderBuilderPtr& builder);

    ~MultiReaderImpl() override;

    ErrCode INTERFACE_FUNC setOnDataAvailable(IProcedure* callback) override;
    ErrCode INTERFACE_FUNC getValueReadType(SampleType* sampleType) override;
    ErrCode INTERFACE_FUNC getDomainReadType(SampleType* sampleType) override;
    ErrCode INTERFACE_FUNC setValueTransformFunction(IFunction* transform) override;
    ErrCode INTERFACE_FUNC setDomainTransformFunction(IFunction* transform) override;
    ErrCode INTERFACE_FUNC getReadMode(ReadMode* mode) override;

    ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC read(void* samples, SizeT* count, SizeT timeoutMs, IReaderStatus** status) override;
    ErrCode INTERFACE_FUNC readWithDomain(void* samples, void* domain, SizeT* count, SizeT timeoutMs, IReaderStatus** status) override;
    ErrCode INTERFACE_FUNC skipSamples(SizeT* count, IReaderStatus** status) override;


    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept) override;
    ErrCode INTERFACE_FUNC connected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC disconnected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC packetReceived(IInputPort* inputPort) override;

    // IReaderConfig
    ErrCode INTERFACE_FUNC getValueTransformFunction(IFunction** transform) override;
    ErrCode INTERFACE_FUNC getDomainTransformFunction(IFunction** transform) override;
    ErrCode INTERFACE_FUNC getInputPorts(IList** ports) override;
    ErrCode INTERFACE_FUNC getReadTimeoutType(ReadTimeoutType* timeoutType) override;
    ErrCode INTERFACE_FUNC markAsInvalid() override;

    ErrCode INTERFACE_FUNC getTickResolution(IRatio** resolution) override;
    ErrCode INTERFACE_FUNC getOrigin(IString** origin) override;
    ErrCode INTERFACE_FUNC getOffset(void* domainStart) override;
    ErrCode INTERFACE_FUNC getCommonSampleRate(Int* commonSampleRate) override;

    ErrCode INTERFACE_FUNC getIsSynchronized(Bool* isSynchronized) override;

private:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    template <typename T>
    static bool ListElementsHaveSameType(const ListPtr<IBaseObject>& list);
    static ListPtr<IInputPortConfig> CheckPreconditions(const ListPtr<IComponent>& list, bool overrideMethod, bool& fromInputPorts);
    void updateCommonSampleRateAndDividers();
    ListPtr<ISignal> getSignals() const;

    void setStartInfo();
    void connectPorts(const ListPtr<IInputPortConfig>& inputPorts, SampleType valueRead, SampleType domainRead, ReadMode mode);
    SizeT getMinSamplesAvailable(bool acrossDescriptorChanges = false) const;
    DictPtr<ISignal, IEventPacket> readUntilFirstDataPacket();
    ErrCode synchronize(SizeT& min, SyncStatus& syncStatus);

    SyncStatus getSyncStatus() const;

    MultiReaderStatusPtr readPackets();

    void prepare(void** outValues, SizeT count, std::chrono::milliseconds timeoutTime);
    void prepareWithDomain(void** outValues, void** domain, SizeT count, std::chrono::milliseconds timeoutTime);

    [[nodiscard]] Duration durationFromStart() const;

    void readSamples(SizeT samples);

    void readDomainStart();
    void sync();

    std::mutex mutex;
    bool invalid{false};
    std::string errorMessage;

    SizeT remainingSamplesToRead{};

    void** values{};
    void** domainValues{};

    Clock::duration timeout{};
    Clock::time_point startTime;

    StringPtr readOrigin;
    RatioPtr readResolution;
    std::unique_ptr<Comparable> commonStart;
    std::int64_t requiredCommonSampleRate = -1;
    std::int64_t commonSampleRate = -1;
    std::int32_t sampleRateDividerLcm = 1;

    std::vector<SignalReader> signals;
    PropertyObjectPtr portBinder;
    ProcedurePtr readCallback;

    LoggerComponentPtr loggerComponent;

    bool startOnFullUnitOfDomain;

    MultiReaderStatusPtr defaultStatus;

    NotifyInfo notify{};
};

END_NAMESPACE_OPENDAQ
