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
#include <coretypes/stringobject_factory.h>
#include <opendaq/multi_reader2_status.h>
#include <opendaq/packet_ptr.h>

#include <atomic>
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
        StringPtr mainInputId;
        SizeT minReadCount = 1;
        Bool requireSameRates = False;
    };

    // Full reset: applies the new input set and settings, all queued data and staged state is dropped
    void reconfigure(Config config);

    // Drops all queued packets and staged event state
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

    void connected(const StringPtr& inputId);
    void disconnected(const StringPtr& inputId);

    // True = data or an event became deliverable: run a notification pass, then call armDataAvailable
    // Data is dropped for unused inputs and inactive readers; event packets always flow
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
    struct alignas(64) SlotCell
    {
        SpscPacketQueue queue;
        std::atomic<SizeT> dataPacketCount{0};
    };

    // Producer-visible state, swapped wholesale on reconfigure; stragglers finish on the old block
    struct State
    {
        explicit State(SizeT slotCount);

        std::vector<std::unique_ptr<SlotCell>> slots;
        std::atomic<uint64_t> readyMask{0};
        std::atomic<uint64_t> usedMask{0};
        std::atomic<SizeT> queuedEventPackets{0};
        std::atomic<bool> parked{false};
        std::atomic<bool> active{true};
        std::atomic<bool> armed{true};
    };

    // Wake condition: an event packet is queued, or every used input has data; never while a commit is owed
    static bool deliverable(const State& state);

    std::mutex consumerMutex;
    Config config;
    std::shared_ptr<State> state;
};

END_NAMESPACE_OPENDAQ
