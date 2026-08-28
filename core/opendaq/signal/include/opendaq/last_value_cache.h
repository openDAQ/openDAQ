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
#include <opendaq/active_operation_tracker.h>
#include <opendaq/context_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_packet_impl.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/packet_ptr.h>
#include <opendaq/signal_exceptions.h>
#include <opendaq/reader_utils.h>
#include <date/date.h>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
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

    // Feeds the cache from a staged raw copy of the last sample (made at send time - the
    // packet itself is never retained). A null descriptor means the corresponding part was
    // not readable and resets it, mirroring the old packet-based failure handling.
    void cacheRaw(const DataDescriptorPtr& stagedValueDescriptor,
                  const void* valueData,
                  SizeT valueSize,
                  const DataDescriptorPtr& stagedDomainDescriptor,
                  const void* domainData,
                  SizeT domainSize)
    {
        value = nullptr;
        if (stagedValueDescriptor.assigned())
        {
            if (stagedValueDescriptor.getObject() != valueDescriptor.getObject())
                valueDescriptor = stagedValueDescriptor;
            const auto* bytes = static_cast<const std::byte*>(valueData);
            rawValue.assign(bytes, bytes + valueSize);
        }
        else
            resetData();

        timestamp = nullptr;
        if (stagedDomainDescriptor.assigned())
        {
            if (stagedDomainDescriptor.getObject() != domainDescriptor.getObject())
            {
                domainDescriptor = stagedDomainDescriptor;
                domainInfoValid = false;
                if (!validateDomainDescriptionForTimestamp())
                {
                    resetTimestamp();
                    return;
                }
            }
            const auto* bytes = static_cast<const std::byte*>(domainData);
            rawTimestamp.assign(bytes, bytes + domainSize);
        }
        else
            resetTimestamp();
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

namespace details
{

/*
 * Copy of the last sample of the most recently sent data packet (raw bytes + descriptor
 * references only) - the packet itself is never retained past sendPacket, so producers
 * backed by circular buffers can reclaim its memory immediately. Reused nodes keep their
 * descriptors as a cache to skip size queries while the descriptor is unchanged.
 */
struct StagedLastValue
{
    DataDescriptorPtr valueDescriptor;
    DataDescriptorPtr domainDescriptor;
    std::vector<std::byte> rawValue;
    std::vector<std::byte> rawDomain;
    BaseObjectPtr explicitValue;      // setLastValue payload (isExplicit nodes only)
    SizeT valueSampleSize{0};
    SizeT domainSampleSize{0};
    bool valueIsVarLength{false};
    bool valueValid{false};
    bool domainValid{false};
    bool isExplicit{false};
    StagedLastValue* next{nullptr};   // retire-list linkage
};

/*
 * All last-value state and handling of a signal. isEnabled()/publish()/publishExplicit()
 * belong to the signal's producer role (owner-serialized, like sendPacket itself) and
 * never block or (steady state) allocate; every other method is a config operation,
 * serialized externally by the signal's config lock. The producer publishes a staged copy
 * through an atomic slot; a displaced node is reused when no reader is active, otherwise
 * retired for config-path reclamation (see active_operation_tracker.h).
 * getValue()/getTimestamp() lazily feed the byte cache.
 */
class LastValueStore
{
public:
    ~LastValueStore()
    {
        delete slot.exchange(nullptr, std::memory_order_acquire);
        deleteStagedChain(retired.exchange(nullptr, std::memory_order_acquire));
        delete spare;
    }

    // ---- hot path (single producer thread) ----

    bool isEnabled() const
    {
        return enabled.load(std::memory_order_relaxed);
    }

    // copies the packet's last sample and publishes it; retains no packet references
    void publish(const PacketPtr& packet)
    {
        const auto dataPacket = packet.asPtrOrNull<IDataPacket>(true);
        if (!dataPacket.assigned() || dataPacket.getSampleCount() == 0)
            return;

        StagedLastValue* staged = acquireStagedNode();
        fillStagedNode(*staged, dataPacket);
        publishNode(staged);

        // setEnabled(false) may have raced us; re-check so a disabled store retains nothing
        if (!enabled.load(std::memory_order_acquire))
            recycleOrRetireStaged(slot.exchange(nullptr, std::memory_order_seq_cst));
    }

    // setLastValue support (producer role, owner-serialized with publish just like
    // sendPacket): publishes the explicit value through the same slot, lock-free
    void publishExplicit(BaseObjectPtr&& value)
    {
        StagedLastValue* staged = acquireStagedNode();
        staged->isExplicit = true;
        staged->explicitValue = std::move(value);
        staged->valueValid = false;
        staged->domainValid = false;
        publishNode(staged);
    }

    // ---- config paths (serialized by the signal's config lock) ----

    void setEnabled(bool value)
    {
        enabled.store(value, std::memory_order_release);
        if (!value)
        {
            clear();
            cache.resetData();
            cache.resetTimestamp();
            cachedSeq = std::numeric_limits<std::uint64_t>::max();
        }
    }

    void resetCachedValue()
    {
        cache.resetData();
        cache.resetTimestamp();
    }

    // drops the published value and reclaims retired nodes; the byte cache is not touched
    void clear()
    {
        if (auto* node = slot.exchange(nullptr, std::memory_order_seq_cst))
            retireStagedNode(node);  // never touch the producer-private spare from here
        drainRetiredStaged();
    }

    ErrCode getValue(IBaseObject** value, const ContextPtr& context)
    {
        refreshCache();

        if (cache.valueCached())
        {
            *value = cache.getValue().detach();
            return OPENDAQ_SUCCESS;
        }

        if (!cache.valueDescriptorCached())
            return OPENDAQ_IGNORED;

        return daqTry(
            [&value, &context, this]
            {
                auto manager = context.getTypeManager();
                void* rawValue = cache.getRawValueData();
                cache.setValue(
                    PacketDetails::buildObjectFromDescriptor(rawValue, cache.getValueDataDescriptor(), manager, cache.getActualValueSampleSize()));
                *value = cache.getValue().detach();
            });
    }

    ErrCode getTimestamp(IBaseObject** timestamp, const ContextPtr& context)
    {
        refreshCache();

        if (cache.timestampCached())
        {
            *timestamp = cache.getTimestamp().detach();
            return OPENDAQ_SUCCESS;
        }

        if (!cache.domainDescriptorCached())
            return OPENDAQ_IGNORED;

        const ErrCode errCode = daqTry(
            [&timestamp, &context, this]
            {
                auto manager = context.getTypeManager();
                void* rawValue = cache.getRawTimestampData();
                auto tsWithoutTweak = PacketDetails::buildObjectFromDescriptor(rawValue, cache.getDomainDataDescriptor(), manager, 0);

                cache.calculateTimestamp(tsWithoutTweak);
                *timestamp = cache.getTimestamp().detach();
            });

        if (OPENDAQ_FAILED(errCode))
        {
            daqClearErrorInfo();
            cache.resetTimestamp();
            return OPENDAQ_IGNORED;
        }
        return errCode;
    }

private:
    void publishNode(StagedLastValue* staged)
    {
        auto* old = slot.exchange(staged, std::memory_order_seq_cst);
        seq.fetch_add(1, std::memory_order_release);
        recycleOrRetireStaged(old);
    }

    // note: getRawLastValue also materializes rule-calculated (implicit) samples
    void fillStagedNode(StagedLastValue& staged, const DataPacketPtr& dataPacket)
    {
        staged.isExplicit = false;
        staged.explicitValue = nullptr;
        staged.valueValid = false;
        staged.domainValid = false;

        try
        {
            if (auto descriptor = dataPacket.getDataDescriptor(); descriptor.assigned())
            {
                if (descriptor.getObject() != staged.valueDescriptor.getObject())
                {
                    const auto sampleType = descriptor.getSampleType();
                    staged.valueIsVarLength = (sampleType == SampleType::Binary || sampleType == SampleType::String);
                    if (!staged.valueIsVarLength)
                        staged.valueSampleSize = descriptor.getSampleSize();
                    staged.valueDescriptor = std::move(descriptor);
                }
                if (staged.valueIsVarLength)
                    staged.valueSampleSize = dataPacket.getRawDataSize();
                staged.rawValue.resize(staged.valueSampleSize);
                void* raw = staged.rawValue.data();
                staged.valueValid = OPENDAQ_SUCCEEDED(dataPacket->getRawLastValue(&raw));
            }

            if (const auto domainPacket = dataPacket.getDomainPacket(); domainPacket.assigned())
            {
                if (auto domainDescriptor = domainPacket.getDataDescriptor(); domainDescriptor.assigned())
                {
                    if (domainDescriptor.getObject() != staged.domainDescriptor.getObject())
                    {
                        staged.domainSampleSize = domainDescriptor.getSampleSize();
                        staged.domainDescriptor = std::move(domainDescriptor);
                    }
                    staged.rawDomain.resize(staged.domainSampleSize);
                    void* raw = staged.rawDomain.data();
                    staged.domainValid = OPENDAQ_SUCCEEDED(domainPacket->getRawLastValue(&raw));
                }
            }
        }
        catch (...)
        {
            // an unreadable packet must not break the send; getValue/getTimestamp report IGNORED
            staged.valueValid = false;
            staged.domainValid = false;
        }
    }

    StagedLastValue* acquireStagedNode()
    {
        if (spare)
        {
            auto* node = spare;
            spare = nullptr;
            return node;
        }
        return new StagedLastValue();
    }

    // producer thread only: reuse the displaced node if provably unobserved, else retire it
    void recycleOrRetireStaged(StagedLastValue* node)
    {
        if (!node)
            return;

        if (readers.isIdle())
        {
            if (!spare)
            {
                node->next = nullptr;
                spare = node;
            }
            else
            {
                delete node;  // only via the disable re-check in publish()
            }
            return;
        }

        retireStagedNode(node);
    }

    void retireStagedNode(StagedLastValue* node)
    {
        auto* cur = retired.load(std::memory_order_relaxed);
        do
        {
            node->next = cur;
        } while (!retired.compare_exchange_weak(cur, node, std::memory_order_release, std::memory_order_relaxed));
    }

    // config path: wait out active readers, then the retired nodes are exclusively ours
    void drainRetiredStaged()
    {
        auto* chain = retired.exchange(nullptr, std::memory_order_acq_rel);
        if (!chain)
            return;

        readers.waitUntilIdle();
        deleteStagedChain(chain);
    }

    // config path: feed the byte cache from the published node; the whole read must stay
    // inside the reader window since the producer reuses nodes once the tracker is idle
    void refreshCache()
    {
        const std::uint64_t currentSeq = seq.load(std::memory_order_acquire);
        if (currentSeq == cachedSeq)
            return;

        {
            details::ActiveOperationTracker::Scope reader(readers);
            if (const auto* staged = slot.load(std::memory_order_seq_cst))
            {
                if (staged->isExplicit)
                {
                    cache.resetData();
                    cache.resetTimestamp();
                    cache.setValue(BaseObjectPtr(staged->explicitValue));
                }
                else
                {
                    cache.cacheRaw(staged->valueValid ? staged->valueDescriptor : nullptr,
                                   staged->rawValue.data(),
                                   staged->valueSampleSize,
                                   staged->domainValid ? staged->domainDescriptor : nullptr,
                                   staged->rawDomain.data(),
                                   staged->domainSampleSize);
                }
                cachedSeq = currentSeq;
            }
        }
        drainRetiredStaged();
    }

    static void deleteStagedChain(StagedLastValue* chain)
    {
        while (chain)
        {
            auto* next = chain->next;
            delete chain;
            chain = next;
        }
    }

    std::atomic<StagedLastValue*> slot{nullptr};
    std::atomic<std::uint64_t> seq{0};
    ActiveOperationTracker readers;
    std::atomic<StagedLastValue*> retired{nullptr};
    StagedLastValue* spare{nullptr};    // producer-private
    std::atomic<bool> enabled{false};
    LastValueCache cache;
    std::uint64_t cachedSeq{std::numeric_limits<std::uint64_t>::max()};  // config-lock guarded
};

}

END_NAMESPACE_OPENDAQ
