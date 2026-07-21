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
#include <opendaq/activity_counter.h>
#include <opendaq/context_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_packet_impl.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/last_value_cache.h>
#include <opendaq/packet_ptr.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

namespace details
{

/*
 * Signal-owned copy of the last sample of the most recently sent data packet. The producer
 * fills one of these at send time (raw bytes + descriptor references only - descriptors are
 * plain immutable objects, never packet-buffer memory) and publishes it through an atomic
 * slot. The packet itself is NOT retained past sendPacket: producers backed by circular
 * buffers must be able to reclaim a packet's memory as soon as the send returns.
 *
 * Nodes are reused, so the descriptor members double as a producer-side cache: when the
 * incoming packet's descriptor is pointer-identical to the one from this node's previous
 * use, sample-size queries and buffer resizing are skipped.
 */
struct StagedLastValue
{
    DataDescriptorPtr valueDescriptor;
    DataDescriptorPtr domainDescriptor;
    std::vector<std::byte> rawValue;
    std::vector<std::byte> rawDomain;
    SizeT valueSampleSize{0};
    SizeT domainSampleSize{0};
    bool valueIsVarLength{false};
    bool valueValid{false};
    bool domainValid{false};
    StagedLastValue* next{nullptr};   // retire-list linkage
};

/*
 * All last-value state and handling of a signal.
 *
 * Threading contract: publish() and isEnabled() are for the signal's single producer
 * thread and never block, wait or (in steady state) allocate. Every other method is a
 * configuration operation and must be serialized externally by the signal's config lock.
 *
 * Structure: the producer copies the packet's last sample into a StagedLastValue node and
 * publishes it via `slot` (atomic exchange + seq bump). The displaced node is reused as
 * the producer's spare when no reader is inside `readers`, otherwise parked on the
 * `retired` list, which config-path calls reclaim after waiting the readers out (see
 * activity_counter.h for the correctness argument). getValue()/getTimestamp() lazily feed
 * the byte cache from the published node - `cachedSeq` tells whether it is current.
 */
class LastValueStore
{
public:
    ~LastValueStore()
    {
        // no producer or reader can be active anymore: the owning signal is being destroyed
        delete slot.exchange(nullptr, std::memory_order_acquire);
        deleteStagedChain(retired.exchange(nullptr, std::memory_order_acquire));
        delete spare;
    }

    // ---- hot path (single producer thread) ----

    bool isEnabled() const
    {
        return enabled.load(std::memory_order_relaxed);
    }

    // Copies the packet's last sample and publishes it; retains no packet references.
    void publish(const PacketPtr& packet)
    {
        const auto dataPacket = packet.asPtrOrNull<IDataPacket>(true);
        if (!dataPacket.assigned() || dataPacket.getSampleCount() == 0)
            return;

        StagedLastValue* staged = acquireStagedNode();
        fillStagedNode(*staged, dataPacket);

        auto* old = slot.exchange(staged, std::memory_order_seq_cst);
        seq.fetch_add(1, std::memory_order_release);
        recycleOrRetireStaged(old);

        // setEnabled(false) may have cleared the slot concurrently; re-check so a disabled
        // store never retains a stale value (single producer: nobody else can store into
        // the slot between the exchange above and this check).
        if (!enabled.load(std::memory_order_acquire))
            recycleOrRetireStaged(slot.exchange(nullptr, std::memory_order_seq_cst));
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

    // setLastValue support: an explicitly set value wins until the next published packet
    void setExplicitValue(BaseObjectPtr&& value)
    {
        clear();
        cache.resetData();
        cache.resetTimestamp();
        cache.setValue(std::move(value));
        cachedSeq = seq.load(std::memory_order_acquire);
    }

    void resetCachedValue()
    {
        cache.resetData();
        cache.resetTimestamp();
    }

    // Drops the published value and reclaims retired nodes; the byte cache is not touched.
    void clear()
    {
        // must not touch the producer-private spare, so the displaced node goes through
        // the retire list even when the readers are idle
        if (auto* node = slot.exchange(nullptr, std::memory_order_seq_cst))
            retireStagedNode(node);
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
    void fillStagedNode(StagedLastValue& staged, const DataPacketPtr& dataPacket)
    {
        staged.valueValid = false;
        staged.domainValid = false;

        try
        {
            if (auto descriptor = dataPacket.getDataDescriptor(); descriptor.assigned())
            {
                // reused nodes keep the descriptor from their previous use as a cache: same
                // descriptor object means the sample-size queries and resize can be skipped
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

    // producer thread only
    void recycleOrRetireStaged(StagedLastValue* node)
    {
        if (!node)
            return;

        // one-sided variant of the reclamation protocol (see activity_counter.h): after
        // the seq_cst slot exchange that displaced `node`, an idle counter proves no
        // reader can still be inside a window that observed it, so the producer may reuse
        // it immediately. Otherwise park it for config-path reclamation - never wait here.
        if (readers.isIdle())
        {
            if (!spare)
            {
                node->next = nullptr;
                spare = node;
            }
            else
            {
                // only reachable via the disable re-check in publish(), which can displace
                // a second node in one call
                delete node;
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

    // config path
    void drainRetiredStaged()
    {
        auto* chain = retired.exchange(nullptr, std::memory_order_acq_rel);
        if (!chain)
            return;

        // wait out any reader still inside its window, then the nodes are exclusively ours
        readers.waitUntilIdle();
        deleteStagedChain(chain);
    }

    // config path, under the config lock: feed the byte cache lazily from the staged copy
    // the producer last published. The staged node has no refcount, so the read must
    // complete entirely inside the reader window: the producer only reuses a displaced
    // node once the counter is idle.
    void refreshCache()
    {
        const std::uint64_t currentSeq = seq.load(std::memory_order_acquire);
        if (currentSeq == cachedSeq)
            return;

        {
            details::ActivityCounter::Scope reader(readers);
            if (const auto* staged = slot.load(std::memory_order_seq_cst))
            {
                cache.cacheRaw(staged->valueValid ? staged->valueDescriptor : nullptr,
                               staged->rawValue.data(),
                               staged->valueSampleSize,
                               staged->domainValid ? staged->domainDescriptor : nullptr,
                               staged->rawDomain.data(),
                               staged->domainSampleSize);
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
    ActivityCounter readers;
    std::atomic<StagedLastValue*> retired{nullptr};
    StagedLastValue* spare{nullptr};    // producer-private
    std::atomic<bool> enabled{false};
    LastValueCache cache;
    std::uint64_t cachedSeq{std::numeric_limits<std::uint64_t>::max()};  // config-lock guarded
};

}

END_NAMESPACE_OPENDAQ
