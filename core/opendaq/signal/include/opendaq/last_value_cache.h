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
#include <coretypes/common.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/ratio_factory.h>
#include <coretypes/integer_factory.h>
#include <coretypes/number_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/signal_exceptions.h>
#include <opendaq/reader_utils.h>
#include <date/date.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

class LastValueCache
{
public:
    void resetData()
    {
        value = nullptr;
        valueDescriptor = nullptr;
        rawValue.clear();
    }

    void resetTimestamp()
    {
        domainInfoValid = false;
        timestamp = nullptr;
        domainDescriptor = nullptr;
        rawTimestamp.clear();
    }

    const DataDescriptorPtr& getDomainDataDescriptor() const
    {
        return domainDescriptor;
    }

    const DataDescriptorPtr& getValueDataDescriptor() const
    {
        return valueDescriptor;
    }

    BaseObjectPtr getTimestamp() const
    {
        return timestamp.addRefAndReturn();
    }

    BaseObjectPtr getValue() const
    {
        return value.addRefAndReturn();
    }

    bool timestampCached() const
    {
        return timestamp.assigned();
    }

    bool valueCached() const
    {
        return value.assigned();
    }

    bool domainDescriptorCached() const
    {
        return domainDescriptor.assigned();
    }

    bool valueDescriptorCached() const
    {
        return valueDescriptor.assigned();
    }

    SizeT getActualValueSampleSize() const
    {
        if (!valueDescriptor.assigned())
            return 0;
        using ST = SampleType;
        const auto valueST = valueDescriptor.getSampleType();
        return (valueST == ST::Binary || valueST == ST::String) ? rawValue.size() : 0;
    }

    void* getRawTimestampData()
    {
        return rawTimestamp.data();
    }

    void* getRawValueData()
    {
        return rawValue.data();
    }

    void calculateTimestamp(const BaseObjectPtr& timestamp)
    {
        if (!timestamp.assigned())
            DAQ_THROW_EXCEPTION(NotAssignedException, "Timestamp value is not assigned.");

        const auto& resolution = getResolution();
        int64_t tickUs = 0;
        if (const auto floatObj = timestamp.asPtrOrNull<IFloat>(); floatObj.assigned())
            tickUs = static_cast<int64_t>(static_cast<double>(floatObj) * resolution.getNumerator() / resolution.getDenominator());
        else if (const auto intObj = timestamp.asPtrOrNull<IInteger>(); intObj.assigned())
            tickUs = static_cast<int64_t>(intObj) * resolution.getNumerator() / resolution.getDenominator();
        else
            DAQ_THROW_EXCEPTION(NotSupportedException, "Unsupported timestamp type. Expected a numeric type.");

        const int64_t resultUs = tickUs + getOriginOffset();
        this->timestamp = Integer(resultUs);
    }

    void setValue(BaseObjectPtr&& value)
    {
        this->value = std::move(value);
    }

    void cache(const DataPacketPtr& packet)
    {
        cacheValue(packet);
        cacheTimestamp(packet);
    }

private:
    const RatioPtr& getResolution()
    {
        if (domainInfoValid || !domainDescriptor.assigned())
            return domainResolution;

        recalculateInternalCache();
        return domainResolution;
    }

    int64_t getOriginOffset()
    {
        if (domainInfoValid || !domainDescriptor.assigned())
            return originOffsetUs;

        recalculateInternalCache();
        return originOffsetUs;
    }

    void recalculateInternalCache()
    {
        {
            // Convert to microseconds
            domainResolution = domainDescriptor.getTickResolution();
            if (!domainResolution.assigned())
                domainResolution = Ratio(1'000'000, 1);
            else
                domainResolution = (domainResolution / Ratio(1, 1'000'000)).simplify();
        }
        {
            // Normalize ISO 8601 origin to the format date::from_stream expects: "YYYY-mm-ddTHH:MM:SS+HH:MM"

            const auto originStr = domainDescriptor.getOrigin();
            if (!originStr.assigned())
                DAQ_THROW_EXCEPTION(NotAssignedException, "Origin in domain descriptor is not assigned.");
            const auto origin = originStr.toStdString();

            bool parsingIsOk = false;
            const auto signalEpoch = reader::parseEpoch(origin, &parsingIsOk);

            if (parsingIsOk == false)
                DAQ_THROW_EXCEPTION(InvalidParametersException, "Origin string is not a valid ISO 8601 date-time.");

            originOffsetUs = std::chrono::duration_cast<std::chrono::microseconds>(signalEpoch.time_since_epoch()).count();
        }
        domainInfoValid = true;
    }

    void cacheValue(const DataPacketPtr& packet)
    {
        value = nullptr;
        if (packet.assigned())
        {
            if (auto packetValueDescriptor = packet.getDataDescriptor(); packetValueDescriptor.assigned())
            {
                const bool descriptorChanged =
                    reinterpret_cast<std::uintptr_t>(packetValueDescriptor.getObject()) != reinterpret_cast<std::uintptr_t>(valueDescriptor.getObject());
                if (descriptorChanged)
                    valueDescriptor = std::move(packetValueDescriptor);

                const auto sampleType = valueDescriptor.getSampleType();
                const bool isVarLength = (sampleType == SampleType::Binary || sampleType == SampleType::String);
                // variable length samples (String/Binary) carry a packet-dependent raw size
                // so the buffer must be resized on every packet, not only when the descriptor object changes
                if (descriptorChanged || isVarLength)
                {
                    const SizeT sampleSize = isVarLength ? packet.getRawDataSize() : valueDescriptor.getSampleSize();
                    rawValue.resize(sampleSize);
                }

                void* rawValue = this->rawValue.data();
                const ErrCode errCode = packet->getRawLastValue(&rawValue);
                if (errCode == OPENDAQ_SUCCESS)
                    return;
            }
        }
        // we come here only if packet is not assigned or if getRawLastValue failed, so we reset the cache
        resetData();
    }

    void cacheTimestamp(const DataPacketPtr& packet)
    {
        timestamp = nullptr;
        if (packet.assigned())
        {
            if (const auto domainPacket = packet.getDomainPacket(); domainPacket.assigned())
            {
                if (auto packetDomainDescriptor = domainPacket.getDataDescriptor(); packetDomainDescriptor.assigned())
                {
                    if (reinterpret_cast<std::uintptr_t>(packetDomainDescriptor.getObject()) != reinterpret_cast<std::uintptr_t>(domainDescriptor.getObject()))
                    {
                        domainDescriptor = std::move(packetDomainDescriptor);
                        domainInfoValid = false;
                        if (!validateDomainDescriptionForTimestamp())
                        {
                            resetTimestamp();
                            return;
                        }
                        rawTimestamp.resize(domainDescriptor.getSampleSize());
                    }
                    void* rawValue = rawTimestamp.data();
                    const ErrCode errCode = domainPacket->getRawLastValue(&rawValue);
                    if (errCode == OPENDAQ_SUCCESS)
                        return;
                }
            }
        }
        // we come here only if packet or domain packet is not assigned or if getRawLastValue failed, so we reset the cache
        resetTimestamp();
    }

    bool validateDomainDescriptionForTimestamp() const
    {
        if (!domainDescriptor.assigned())
            return false;

        const auto tsType = domainDescriptor.getSampleType();
        const bool isNumeric = (tsType == SampleType::Int8    || tsType == SampleType::UInt8  ||
                                tsType == SampleType::Int16   || tsType == SampleType::UInt16 ||
                                tsType == SampleType::Int32   || tsType == SampleType::UInt32 ||
                                tsType == SampleType::Int64   || tsType == SampleType::UInt64 ||
                                tsType == SampleType::Float32 || tsType == SampleType::Float64);
        if (!isNumeric)
            return false;

        if (domainDescriptor.getDimensions().getCount() > 0)
            return false;

        if (auto unit = domainDescriptor.getUnit(); !unit.assigned() || unit.getSymbol() != "s")
            return false;

        if (auto origin = domainDescriptor.getOrigin(); !origin.assigned())
            return false;

        return true;
    }

    BaseObjectPtr value;
    BaseObjectPtr timestamp;

    DataDescriptorPtr valueDescriptor;
    DataDescriptorPtr domainDescriptor;
    std::vector<std::byte> rawValue;
    std::vector<std::byte> rawTimestamp;

    RatioPtr domainResolution = Ratio(1'000'000, 1);
    int64_t originOffsetUs{0};
    bool domainInfoValid{false};
};

END_NAMESPACE_OPENDAQ
