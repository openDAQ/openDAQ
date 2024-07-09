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
#include <websocket_streaming/websocket_streaming.h>
#include <opendaq/signal_ptr.h>
#include <streaming_protocol/SynchronousSignal.hpp>
#include <opendaq/sample_type.h>
#include <opendaq/signal_factory.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <variant>
#include <websocket_streaming/signal_info.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class InputSignalBase;
using InputSignalBasePtr = std::shared_ptr<InputSignalBase>;

class InputSignal;
using InputSignalPtr = std::shared_ptr<InputSignal>;

class InputSignalBase
{
public:
    InputSignalBase(const std::string& signalId,
                    const std::string& tabledId,
                    const SubscribedSignalInfo& signalInfo,
                    const InputSignalBasePtr& domainSignal,
                    daq::streaming_protocol::LogCallback logCb);

    virtual void processSamples(const NumberPtr& startDomainValue, const uint8_t* data, size_t sampleCount);
    virtual DataPacketPtr generateDataPacket(const NumberPtr& packetOffset,
                                             const uint8_t* data,
                                             size_t sampleCount,
                                             const DataPacketPtr& domainPacket) = 0;
    virtual bool isDomainSignal() const = 0;
    virtual bool isCountable() const = 0;

    EventPacketPtr createDecriptorChangedPacket(bool valueChanged = true, bool domainChanged = true) const;

    void setDataDescriptor(const DataDescriptorPtr& dataDescriptor);
    bool hasDescriptors() const;

    DataDescriptorPtr getSignalDescriptor() const;
    std::string getTableId() const;
    std::string getSignalId() const;

    InputSignalBasePtr getInputDomainSignal() const;

protected:
    const std::string signalId;
    const std::string tableId;
    DataDescriptorPtr currentDataDescriptor;
    const InputSignalBasePtr inputDomainSignal;

    std::string name;
    std::string description;

    daq::streaming_protocol::LogCallback logCallback;
    mutable std::mutex descriptorsSync;
};

class InputDomainSignal : public InputSignalBase
{
public:
    InputDomainSignal(const std::string& signalId,
                      const std::string& tabledId,
                      const SubscribedSignalInfo& signalInfo,
                      streaming_protocol::LogCallback logCb);

    DataPacketPtr generateDataPacket(const NumberPtr& packetOffset,
                                     const uint8_t* data,
                                     size_t sampleCount,
                                     const DataPacketPtr& domainPacket) override;
    bool isDomainSignal() const override;
    bool isCountable() const override;

private:
    DataPacketPtr lastDomainPacket;
};

class InputExplicitDataSignal : public InputSignalBase
{
public:
    InputExplicitDataSignal(const std::string& signalId,
                            const std::string& tabledId,
                            const SubscribedSignalInfo& signalInfo,
                            const InputSignalBasePtr& domainSignal,
                            streaming_protocol::LogCallback logCb);

    DataPacketPtr generateDataPacket(const NumberPtr& packetOffset,
                                     const uint8_t* data,
                                     size_t sampleCount,
                                     const DataPacketPtr& domainPacket) override;
    bool isDomainSignal() const override;
    bool isCountable() const override;
};

class InputConstantDataSignal : public InputSignalBase
{
public:
    using SignalValueType =
        std::variant<int8_t, int16_t , int32_t, int64_t, uint8_t, uint16_t , uint32_t, uint64_t, float, double>;

    InputConstantDataSignal(const std::string& signalId,
                            const std::string& tabledId,
                            const SubscribedSignalInfo& signalInfo,
                            const InputSignalBasePtr& domainSignal,
                            streaming_protocol::LogCallback logCb);

    void processSamples(const NumberPtr& absoluteStartDomainValue, const uint8_t* data, size_t sampleCount) override;
    DataPacketPtr generateDataPacket(const NumberPtr& packetOffset,
                                     const uint8_t* data,
                                     size_t sampleCount,
                                     const DataPacketPtr& domainPacket) override;
    bool isDomainSignal() const override;
    bool isCountable() const override;

private:
    template<typename DataType>
    static SignalValueType extractConstantValue(const uint8_t* pValue);

    template<typename DataType>
    static DataPacketPtr createTypedConstantPacket(
        SignalValueType startValue,
        const std::vector<std::pair<uint32_t, SignalValueType>>& otherValues,
        size_t sampleCount,
        const DataPacketPtr& domainPacket,
        const DataDescriptorPtr& dataDescriptor);

    NumberPtr calcDomainValue(const NumberPtr& startDomainValue, const uint64_t sampleIndex);
    NumberPtr getDomainRuleDelta();
    uint32_t calcPosition(const NumberPtr& startDomainValue, const NumberPtr& domainValue);

    std::map<NumberPtr, SignalValueType> cachedSignalValues;
};

inline InputSignalBasePtr InputSignal(const std::string& signalId,
                                      const std::string& tabledId,
                                      const SubscribedSignalInfo& signalInfo,
                                      bool isTimeSignal,
                                      const InputSignalBasePtr& domainSignal,
                                      streaming_protocol::LogCallback logCb)
{
    auto dataRuleType = signalInfo.dataDescriptor.getRule().getType();

    if (isTimeSignal)
    {
        if (dataRuleType == daq::DataRuleType::Linear)
            return std::make_shared<InputDomainSignal>(signalId, tabledId, signalInfo, logCb);
        else
            throw ConversionFailedException("Unsupported input domain signal rule");
    }
    else
    {
        if (dataRuleType == daq::DataRuleType::Explicit)
            return std::make_shared<InputExplicitDataSignal>(signalId, tabledId, signalInfo, domainSignal, logCb);
        else if (dataRuleType == daq::DataRuleType::Constant)
            return std::make_shared<InputConstantDataSignal>(signalId, tabledId, signalInfo, domainSignal, logCb);
        else
            throw ConversionFailedException("Unsupported input data signal rule");
    }
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
