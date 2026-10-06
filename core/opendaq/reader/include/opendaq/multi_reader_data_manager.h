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
#include <opendaq/component_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <coretypes/function_ptr.h>
#include <opendaq/multi_reader2_params.h>
#include <opendaq/multi_reader2_status.h>
#include <opendaq/packet_ptr.h>
#include <opendaq/sample_type.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

// Internal engine of the multi reader: per-input queues, the state, synchronization and the read path. A plain
// C++ class owned by MultiReader2Impl; never touches a port, signal, connection or scheduler. One non-recursive
// mutex guards every method, held for the whole call; there are no call-outs under it.
//
// Ingest only queues: data packets as positioned runs, event packets, connection edges and gaps as boundaries. It
// decides nothing about the inputs except whether the consumer owes a wake. Everything else happens in read: the
// aligned run in front of the first boundary is delivered, the boundaries reached are evaluated, the state moves,
// and the result is reported on that read's status.
class MultiReaderDataManager final
{
public:
    // The params resolved to slot indices
    struct Config
    {
        std::vector<ComponentPtr> inputs;  // slot order; the input statuses hand these back
        std::vector<bool> connected;       // connection state per slot at configure time; always true for signals
        std::vector<DataDescriptorPtr> valueDescriptors;   // what new slots start from; retained slots keep what they saw
        std::vector<DataDescriptorPtr> domainDescriptors;
        std::optional<SizeT> mainSlot;     // pinned main input; unset = the reader chooses
        FunctionPtr acceptsDescriptor;     // the owner's judgement of (input, value, domain); null accepts everything
        SampleType readType = SampleType::Float64;
        SizeT minReadCount = 1;
        MultiReader2ErrorPolicy errorPolicy = MultiReader2ErrorPolicy::Invalidate;
    };

    // What a producer call left owed; the reader schedules the wake after the call returns
    struct NotifyOwed
    {
        bool wake = false;
    };

    MultiReaderDataManager();

    // True when the config equals the current one, input by input; configure is then a no-op
    bool isSameConfig(const Config& config);

    // Full reset: drops everything queued, keeps each retained slot's last seen descriptors and connection state,
    // re-evaluates errors, starts synchronization if valid, and marks every descriptor, the domain and the
    // resynchronization as pending for the next status
    void reconfigure(Config config);

    // Consumer side; contracts exactly as on IMultiReader2, data as the real void**
    SizeT getAvailableCount();
    ErrCode read(void** data, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status);
    ErrCode readWithDomain(void** data, SizeT* count, IMultiReader2Status** status);
    ErrCode skipSamples(SizeT* count, IMultiReader2Status** status);

    // Global id of the main input in effect, empty when there is none
    StringPtr getMainInputId();

    // Producer side. Connection edges and event packets queue as boundaries. Data packets queue while the slot can
    // deliver, as of the last read, and are dropped at the door otherwise
    // A connect carries the signal's current descriptors so the input is whole on the status that reports it
    NotifyOwed connected(SizeT slot, const DataDescriptorPtr& value = nullptr, const DataDescriptorPtr& domain = nullptr);
    NotifyOwed disconnected(SizeT slot);
    NotifyOwed setConnected(SizeT slot, bool connected, const DataDescriptorPtr& value = nullptr, const DataDescriptorPtr& domain = nullptr);
    NotifyOwed addPacket(SizeT slot, const PacketPtr& packet);

    // Incremented whenever a producer call owed a wake; the reader re-evaluates after the consumer's handler returns
    std::uint64_t getWakeGeneration();

    // The synchronization guards: a silent input fails after the deadline, an input whose data ends further than
    // the distance behind the others fails at once. Both default to 2 seconds
    void setSyncLimits(std::chrono::milliseconds deadline, std::chrono::milliseconds maxDistance);

    // True on the thread that is inside acceptsDescriptor right now; the reader refuses its calls there
    bool inCallback() const;

private:
    enum class State
    {
        Invalid,
        Syncing,   // valid, common start not reached; the user sees valid with count 0
        Streaming
    };

    struct Boundary
    {
        enum class Kind
        {
            Descriptor,
            Connected,
            Disconnected,
            Gap
        };

        Kind kind;
        bool hasValue = false;
        bool hasDomain = false;
        DataDescriptorPtr value;
        DataDescriptorPtr domain;
    };

    // One queued item: a data run positioned on its own tick grid, or a boundary
    struct Entry
    {
        bool isBoundary;
        Boundary boundary;
        DataPacketPtr packet;
        std::int64_t start = 0;  // first sample tick, rule start included
        std::int64_t delta = 1;
        SizeT count = 0;
    };

    // Parsed from a slot's current domain descriptor
    struct DomainInfo
    {
        bool valid = false;
        std::int64_t delta = 1;
        std::int64_t start = 0;
        std::int64_t resNum = 1;
        std::int64_t resDen = 1;
        std::int64_t rate = 0;        // samples per second
        bool hasOrigin = false;
        std::int64_t originSec = 0;   // seconds since the Unix epoch
        // Position on the main grid: tick * scaleNum / scaleDen + originOffset
        std::int64_t scaleNum = 1;
        std::int64_t scaleDen = 1;
        std::int64_t originOffset = 0;
    };

    struct Slot
    {
        // The front data run (same delta, no jump, up to the first boundary) is tracked as entries come and go, so
        // availability and the read plan cost no walk over the queue
        std::deque<Entry> queue;
        SizeT frontOffset = 0;  // samples of the front data entry already consumed
        SizeT runCount = 0;     // samples of the front data run, the consumed part included
        bool runOpen = false;   // the run reaches the back of the queue, so a continuing packet extends it

        void push(Entry entry);
        void popFront();
        void clear();
        void recomputeRun();

        // Last seen on ingest: the state a reconfigure resets to
        DataDescriptorPtr latestValue;
        DataDescriptorPtr latestDomain;
        DataDescriptorPtr packetDomain;  // the domain descriptor the rule below was parsed from
        std::int64_t packetDelta = 1;
        std::int64_t packetStart = 0;
        bool latestConnected = false;

        // Evaluated state, moved only by read
        DataDescriptorPtr value;
        DataDescriptorPtr domain;
        bool connected = false;
        DomainInfo dom;
        bool valueValid = false;
        bool accepted = true;  // the owner's verdict on the current descriptors; false is ValueDescriptorInvalid
        MultiReader2InputError error = MultiReader2InputError::None;
        bool syncFailed = false;  // cleared by the next boundary, by data showing up, or by a reconfigure
        bool offGrid = false;     // a sample position off the main lattice; cleared by the next boundary or a reconfigure

        // Pending for the next status
        bool descriptorChanged = false;

        // Level snapshot of the last status
        MultiReader2InputError reportedError = MultiReader2InputError::None;
    };

    // Position and length of the deliverable run in front of a slot, in main ticks
    struct Run
    {
        bool hasData = false;
        std::int64_t first = 0;
        std::int64_t last = 0;
        SizeT samples = 0;
        bool endsAtBoundary = false;
        bool offGrid = false;
    };

    ErrCode doRead(void** data, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status, bool withDomain, bool copy);

    bool contributes(SizeT index) const;
    bool dropsData(SizeT index) const;
    SizeT availableLocked() const;
    Run runOf(SizeT index) const;
    std::optional<std::int64_t> toMain(SizeT index, std::int64_t tick) const;
    void advance(SizeT index, SizeT samples);
    void discardBefore(SizeT index, std::int64_t mainTick);
    void copyRun(SizeT index, void* buffer, SizeT samples) const;

    void consumeBoundaries();
    void settle(bool wasValid, std::optional<SizeT> oldMain, const std::vector<bool>& oldContributes, bool domainWasPending, bool resyncWasPending);
    std::vector<bool> contributions() const;
    bool askAccepts(SizeT index, const DataDescriptorPtr& value, const DataDescriptorPtr& domain);
    void applyBoundary(SizeT index, const Boundary& boundary);
    void evaluate();
    void chooseMain();
    void parseDomain(Slot& slot);
    void relateToMain(SizeT index);
    static bool valueConvertible(const DataDescriptorPtr& descriptor, SampleType readType);
    void internalReconfigure();
    void startSync();
    bool trySync();  // true when the state moved: streaming reached, or a failure settled
    ObjectPtr<IMultiReader2Status> makeStatus();

    std::mutex mutex;
    Config config;
    std::vector<Slot> slots;
    std::optional<SizeT> mainSlot;
    State state = State::Invalid;
    std::int64_t readTick = 0;  // main-tick position of the next unread sample
    bool domainChanged = false;
    bool resynchronized = false;
    bool firstStatus = true;
    bool reportedValid = false;
    ObjectPtr<IMultiReader2Status> lastStatus;  // handed out again while nothing changes; statuses are immutable
    std::chrono::steady_clock::time_point syncDeadline;
    bool deadlineArmed = false;  // set by the first data seen while synchronizing
    std::chrono::milliseconds deadline{2000};
    std::chrono::milliseconds maxDistance{2000};
    std::uint64_t wakeGeneration = 0;
    std::atomic<std::thread::id> callbackThread{};
};

END_NAMESPACE_OPENDAQ
