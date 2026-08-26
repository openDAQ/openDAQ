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
#include <coretypes/dictptr.h>
#include <coretypes/stringobject_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/multi_reader2_status.h>
#include <opendaq/packet_ptr.h>
#include <opendaq/sample_type.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

// Internal engine of the multi reader: per-input packet queues, event staging, and the read path.
// Plain C++ object fully owned by MultiReader2Impl; lives for the reader's whole lifetime.
// Producer calls (addPacket, armDataAvailable) are lock-free; consumer calls share one mutex.
class MultiReaderDataManager final
{
public:
    struct Config
    {
        std::vector<StringPtr> inputIds;
        std::vector<bool> usedFlags;
        std::vector<bool> connectedFlags;
        StringPtr mainInputId;
        SampleType valueReadType = SampleType::Invalid;
        SizeT minReadCount = 1;
        Bool requireSameRates = False;
    };

    // Full reset: applies the new input set and settings, all queued data and staged state is dropped
    void reconfigure(Config config);

    // Drops all queued packets and any pending event; committed descriptors survive
    void clear();

    // Samples readable from every used input; 0 while an event is pending
    ErrCode getAvailableCount(SizeT* count);

    // Executes one read; while an event is pending it returns the last status and no data
    ErrCode read(IMultiReader2Status** status, void** data, SizeT* count, SizeT* packetOffset);

    // Applies the pending event (staged setUsed/setActive included) and resumes past the boundary
    ErrCode commitEvent();

    // Only valid between an event read and commitEvent
    ErrCode setActive(Bool active);
    ErrCode setUsed(IString* inputId, Bool used);

    // While any used input is disconnected nothing wakes and all packets are dropped; descriptors stay cached
    void connected(SizeT slotIndex);
    void disconnected(SizeT slotIndex);

    // True = data or an event became deliverable: run a notification pass, then call armDataAvailable
    // Data is dropped for unused inputs and inactive readers; descriptor events are always cached
    Bool addPacket(SizeT slotIndex, const PacketPtr& packet);

    // Re-arms notifications; true = more is already deliverable and the caller owes another pass now
    Bool armDataAvailable();

private:
    // Unbounded SPSC queue with a dummy node: the slot's port thread pushes, the reader thread pops
    class SpscPacketQueue final
    {
    public:
        SpscPacketQueue();
        ~SpscPacketQueue();

        void push(PacketPtr packet);
        PacketPtr pop();

    private:
        struct Node
        {
            PacketPtr packet;
            std::atomic<Node*> next{nullptr};
        };

        Node* head;
        Node* tail;
    };

    // Per-slot hot state, cache-line separated so producers of adjacent slots never share a line
#pragma warning(push)
#pragma warning(disable : 4324)
    struct alignas(64) SlotCell
    {
        SpscPacketQueue queue;  // data packets only; descriptor events are cache-only
        std::atomic<SizeT> dataPacketCount{0};
        std::atomic<IPacket*> lastEventPacket{nullptr};  // merged full-state descriptor event, newest wins
        std::atomic<uint64_t> eventVersion{0};
        // Merge sources owned by the slot's single producer thread; never read by the consumer
        DataDescriptorPtr producerValueDescriptor;
        DataDescriptorPtr producerDomainDescriptor;
    };
#pragma warning(pop)

    // Producer-visible state, swapped wholesale on reconfigure; stragglers finish on the old block
    struct State
    {
        explicit State(SizeT slotCount);
        ~State();

        std::vector<std::unique_ptr<SlotCell>> slots;
        std::atomic<uint64_t> readyMask{0};
        std::atomic<uint64_t> usedMask{0};
        std::atomic<uint64_t> connectedMask{0};
        std::atomic<SizeT> pendingEvents{0};  // cached descriptor events not yet delivered by a read
        std::atomic<bool> parked{false};
        std::atomic<bool> active{true};
        std::atomic<bool> armed{true};
    };

    // Consumer-side view of one slot: staged packets, committed descriptors, and parsed domain facts
    struct SlotView
    {
        std::deque<PacketPtr> staged;
        SizeT stagedSamples = 0;   // readable samples under the committed value descriptor
        SizeT frontOffset = 0;     // samples already consumed from the front staged packet
        uint64_t deliveredVersion = 0;
        DataDescriptorPtr valueDescriptor;
        DataDescriptorPtr domainDescriptor;
        // Parsed from the committed domain descriptor; valid only under the pinned constraints
        bool domainValid = false;
        MultiReader2InputError domainError = MultiReader2InputError::InvalidDescriptor;
        StringPtr origin;
        Int delta = 1;
        Int resolutionNum = 1;
        Int resolutionDen = 1;
        // Scales this input's ticks into main input ticks; set during sync validation
        Int tickNum = 1;
        Int tickDen = 1;
        // A failed input sits out of sync and reading until a new descriptor or reconfigure refreshes it
        bool failed = false;
    };

    // Wake condition: an event is undelivered, or every used input has data; never while a commit is owed
    static bool deliverable(const State& state);
    static bool matchesDescriptor(const DataDescriptorPtr& committed, const PacketPtr& packet);

    // Consumer helpers; all expect consumerMutex to be held
    void drainSlots(State& state);
    void discardAllData(State& state);
    void discardSlotData(State& state, SizeT index);
    void settleReadyBit(State& state, SizeT index);
    bool deliverEvents(State& state, uint64_t& deliveredMask);
    static void parseDomain(SlotView& view);
    bool nextTimestamp(SlotView& view, Int& timestamp);
    void discardBefore(SlotView& view, Int target);
    bool runSync(State& state);
    ObjectPtr<IMultiReader2Status> makeStatus(MultiReader2StatusType type, const DictPtr<IString, IInteger>& errors);
    void park(State& state, MultiReader2StatusType type, const DictPtr<IString, IInteger>& errors);

    std::mutex consumerMutex;
    Config config;
    std::shared_ptr<State> state;
    std::vector<SlotView> views;
    ObjectPtr<IMultiReader2Status> pendingStatus;
    uint64_t reportedDisconnectMask = 0;
    SizeT mainIndex = 0;
    // Synchronization progress; sync restarts after every committed event
    bool synced = false;
    bool syncStarted = false;
    std::chrono::steady_clock::time_point syncDeadline;
    Int syncedStart = 0;
};

END_NAMESPACE_OPENDAQ
