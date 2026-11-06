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
#include <coretypes/function_ptr.h>
#include <opendaq/component_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/multi_reader2_domain.h>
#include <opendaq/multi_reader2_input_status.h>
#include <opendaq/multi_reader2_params.h>
#include <opendaq/multi_reader2_queue.h>
#include <opendaq/multi_reader2_status_ptr.h>
#include <opendaq/packet_ptr.h>
#include <opendaq/sample_type.h>
#include <opendaq/typed_reader.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Internal: the engine of MultiReader2 (spec 0007, Data manager sketch). The reader in front of it maps ports to
 * slots and holds one intake lock per port, so a port's packets and connection changes arrive here in order.
 *
 * Every method runs under one non-recursive mutex, held for the whole call, except a read's conversion: the samples are
 * taken off the queues under it and converted into the user's buffers after it. The one call-out is acceptsDescriptor,
 * made under the mutex on the consumer's thread; reader calls from inside it are refused.
 *
 * Cost model: ingest is O(1) per packet (the queue keeps its counts), plus a pass over the inputs per packet only while
 * synchronizing. A read is one pass over the inputs, plus the events it reaches. Whether a handler call is owed is decided
 * once per ingest run, not per packet.
 *
 * How the code is laid out, one section each in the source:
 *  - configuration: configure, reevaluate and the internal reconfigure;
 *  - judgement: an input's errors, the main input, the reader's level;
 *  - read: the samples every contributing input has in front of its first event, then the events the read reached;
 *  - ingest: packets and connection changes onto the queues, the queue limits, the continuity check;
 *  - synchronization: the check mechanism, run at ingest and after a read applies events;
 *  - status: what the next status reports, found by comparing with the previous one;
 *  - wake: when onDataAvailable is owed.
 */
class MultiReaderDataManager final
{
public:
    // The params resolved to slot indices
    struct Config
    {
        std::vector<ComponentPtr> inputs;                 // slot order; the input statuses hand these back
        std::vector<bool> connected;                      // per slot at configure time; always true for signals
        std::vector<DataDescriptorPtr> valueDescriptors;  // what new slots start from; retained slots keep what they saw
        std::vector<DataDescriptorPtr> domainDescriptors;
        std::optional<SizeT> mainSlot;                    // pinned main input; unset = the reader chooses
        FunctionPtr acceptsDescriptor;                    // null accepts everything
        SampleType readType = SampleType::Float64;
        SizeT minReadCount = 1;
        MultiReader2ErrorPolicy errorPolicy = MultiReader2ErrorPolicy::Invalidate;
        LoggerComponentPtr loggerComponent;               // for warnings; may be null
    };

    // What an ingest call queued; the wake decision is made once per ingest run from the sum of these
    struct Ingested
    {
        bool trigger = false;  // an event, a synchronization outcome or a limit: a call is owed
        bool data = false;     // a data packet: a call is owed if samples are readable
        bool dropped = false;  // the critical limit dropped queues, whose packets releaseDropped frees

        Ingested& operator|=(const Ingested& other)
        {
            trigger = trigger || other.trigger;
            data = data || other.data;
            dropped = dropped || other.dropped;
            return *this;
        }
    };

    // Applies every queued event, drops the data, then a 0-count read. A null status leaves the first status owed to the
    // first read, as after construction
    ErrCode reconfigure(Config newConfig, IMultiReader2Status** status);

    // Asks acceptsDescriptor for every input with its descriptors in force, then a 0-count read
    ErrCode reevaluate(IMultiReader2Status** status);

    // Consumer side; contracts as on IMultiReader2, data as the real void**. Errors and descriptors move only here
    SizeT getAvailableCount();
    ErrCode read(void** data, std::int64_t* domain, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status);
    ErrCode skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status);

    // Producer side. A connection change returns whether onDataAvailable is to be scheduled, and then claims the one
    // pending call. Packets come in batches under lockIngest's lock, which the ingest task waits for and a sending thread
    // only tries, so a sender never waits for a read or another port; claimWake then decides on the call in O(1)
    bool connected(SizeT slot, const DataDescriptorPtr& value, const DataDescriptorPtr& domain);
    bool disconnected(SizeT slot);
    std::unique_lock<std::mutex> lockIngest(bool wait);
    Ingested addPackets(const std::unique_lock<std::mutex>& lock, SizeT slot, const PacketPtr* packets, SizeT count);
    bool claimWake(const std::unique_lock<std::mutex>& lock, const Ingested& ingested);
    // Frees the packets the critical limit dropped, outside the mutex, so a sending thread does not pay for them
    void releaseDropped();

    void callStarted();  // the handler is about to run
    // After the handler returns: clears the pending flag and decides on another call in one step, claiming it. Only an
    // arrival since the call started that is still unread calls again
    bool callFinished(bool failed);
    void callAborted();  // the claimed call could not be scheduled

    bool inCallback() const;  // true on the thread inside acceptsDescriptor

private:
    using Event = multi_reader2::Event;
    using SyncCause = multi_reader2::SyncCause;

    // The limits on what an input queues, in its own data time and in bytes:
    //  - sync limit: how far apart the reader expects the inputs' data. An input that queues more has the inputs judged for
    //    a stall, and nothing is dropped; an input that starts more than this past the main input's samples is ahead;
    //  - recovery limit: a stalled or ahead input rejoins once its newest sample is this close to the others';
    //  - critical limit: an input that queues more is read too slowly, and every queue goes. Also over all inputs together
    static constexpr std::int64_t syncLimitSeconds = 3;
    static constexpr std::int64_t recoveryLimitSeconds = 1;
    static constexpr std::int64_t criticalLimitSeconds = 5;
    static constexpr SizeT criticalLimitBytes = SizeT(100) * 1000 * 1000;
    static constexpr SizeT syncLimitBytes = criticalLimitBytes / criticalLimitSeconds * syncLimitSeconds;
    static constexpr SizeT totalLimitBytes = SizeT(1000) * 1000 * 1000;

    enum class State
    {
        Invalid,
        Syncing,   // valid, the common start not reached
        Streaming
    };

    // What an input announced: its connection and descriptors
    struct Announced
    {
        bool connected = false;
        DataDescriptorPtr value;
        DataDescriptorPtr domain;
    };

    struct Slot
    {
        multi_reader2::InputQueue queue;

        // Producer side: the tail of the queue
        Announced latest;                                   // what a reconfigure evaluates
        std::optional<multi_reader2::TickDomain> latestDomain;
        std::optional<multi_reader2::TickMap> toMain;       // latest domain onto the main domain in force
        std::int64_t syncLimitSamples = 0;                  // the limits in the latest domain's samples
        std::int64_t criticalLimitSamples = 0;
        std::optional<std::int64_t> nextOffset;             // the packetOffset that continues the last packet
        std::optional<std::int64_t> firstSentTick;          // main ticks of the first and the newest sample ingested on the
        std::optional<std::int64_t> newestTick;             // current main grid, dropped or not; the stall and rejoin measure them

        // Consumer side: in force, moved only by read, configure and reevaluate
        Announced current;
        std::optional<multi_reader2::TickDomain> domain;
        MultiReader2InputError ownError = MultiReader2InputError::None;  // the input alone
        bool rejectedByOwner = false;                       // ownError came from acceptsDescriptor
        const char* domainFault = nullptr;                  // the failed domain constraint, named in the status message
        MultiReader2InputError error = MultiReader2InputError::None;     // reported: own, then against the main input
        SyncCause syncFailed = SyncCause::None;             // a relative cause survives an internal reconfigure that keeps
                                                            // the main input; Stalled survives a new main input too
        bool descriptorChanged = false;                     // pending for the next status
        SizeT sampleSize = 0;                               // bytes per sample in the read buffer, healthy only

        // To the read type, for the value descriptor in force; a copy run outside the mutex holds it
        std::shared_ptr<Reader> converter;
    };

    // Samples a read took off a queue, converted into the user's buffer once the mutex is released
    struct Copy
    {
        DataPacketPtr packet;
        SizeT first = 0;
        SizeT count = 0;
        void* out = nullptr;
        std::shared_ptr<Reader> converter;  // null under Undefined: the samples are copied as they are
        SizeT sampleSize = 0;               // bytes per sample in the packet, under Undefined
    };

    // The state fields of a status (Status): they hold until the reader changes
    struct Shown
    {
        ComponentStatus level = ComponentStatus::Ok;
        std::vector<MultiReader2InputError> errors;
        std::string message;
        std::optional<SizeT> mainSlot;
        DataDescriptorPtr domain;

        bool operator==(const Shown& other) const;
    };

    // Configuration
    void internalReconfigure(bool wasValid);
    void restart();
    void afterJudgement(bool wasValid, const std::optional<SizeT>& previousMain, const std::vector<MultiReader2InputError>& previous,
                        bool reconfigureNeeded, bool resync);

    // Judgement
    void checkInput(SizeT slot);
    bool askAccept(SizeT slot);
    void judgeInputs(bool keepMain);
    std::vector<MultiReader2InputError> errors() const;
    bool valid() const
    {
        return level != ComponentStatus::Error;
    }
    bool contributes(const Slot& slot) const
    {
        return valid() && slot.error == MultiReader2InputError::None;
    }

    // Read
    ErrCode checkRead(SizeT count, SizeT timeoutMs, void** data, bool skip) const;
    void performRead(void** data, std::int64_t* domain, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status,
                     std::vector<Copy>* copies);
    SizeT frontAvailable() const;
    bool readable() const;
    bool isShort(const Slot& slot) const;
    void recountShort();
    bool isWaiting(const Slot& slot) const;
    std::optional<SizeT> shortRun() const;
    void dropShortRun();
    void take(Slot& slot, SizeT count, void* out, std::vector<Copy>* copies);
    bool makeConverter(Slot& slot) const;
    static void runCopy(const Copy& copy);
    void applyEvents();
    void applyReachedEvents();
    struct Applied
    {
        bool judge = false;   // descriptors or connection: the input is checked again
        bool domain = false;  // a new domain descriptor
        bool gap = false;
    };
    static Applied applyEvent(Slot& slot, const Event& event);
    static bool reachesEvent(const Slot& slot, bool contributing);

    // Ingest
    bool queueConnect(SizeT slot, const DataDescriptorPtr& value, const DataDescriptorPtr& domain);
    bool queueDisconnect(SizeT slot);
    bool queueDescriptor(SizeT slot, const DataDescriptorPtr& value, const DataDescriptorPtr& domain);
    Ingested ingestPacket(SizeT slot, const PacketPtr& packet);
    bool ingestData(SizeT slot, const DataPacketPtr& packet);
    void useLatestDomain(Slot& slot) const;
    static bool over(const Slot& slot, std::int64_t samples, SizeT bytes, SizeT addedCount, SizeT addedBytes);
    void dropAll(SizeT cause);
    void dropForRejoin(const std::vector<SizeT>& rejoined);

    // Synchronization
    void startSync();
    bool synchronize();
    std::optional<SyncCause> startFailure(const Slot& slot, std::int64_t mainFirst, std::int64_t mainLatest) const;
    bool judgeStart(SizeT i);
    static void failSync(Slot& slot, SyncCause cause);
    bool checkRejoin(SizeT slot);

    // Stalls
    struct Senders;
    Senders senders() const;
    std::optional<std::int64_t> silence(const Slot& slot, const Senders& senders) const;
    bool stallDue() const;
    bool checkStall();
    void planStallCheck();
    void applyStalls();
    void markResynchronized();
    void reportAfresh();

    // Status
    Shown capture() const;
    void noteChanges();
    std::string describeError(const Slot& slot) const;
    MultiReader2StatusPtr buildStatus();

    // Wake
    bool claim(bool owed);
    bool triggerHolds() const;

    std::mutex mutex;
    Config config;
    std::vector<Slot> slots;
    std::optional<SizeT> mainSlot;  // the main input in effect
    ComponentStatus level = ComponentStatus::Ok;
    State state = State::Invalid;

    // Synchronization and read position, in main ticks
    std::int64_t mainDelta = 1;                    // main ticks per sample
    std::int64_t mainStart = 0;                    // the main rule's start: packetOffset is a tick minus it
    std::int64_t syncLimitTicks = 0;               // the sync and recovery limits in main ticks
    std::int64_t recoveryLimitTicks = 0;
    // While synchronizing, ingest checks for a stall only once an input sends past this main tick: the earliest at which an
    // input with nothing queued can be more than the sync limit behind. Unset: at the next ingest
    std::optional<std::int64_t> stallCheckTick;
    std::int64_t readTick = 0;                     // the next unread sample
    bool positioned = false;                       // readTick continues a read that delivered samples since the restart
    std::optional<SizeT> tickedMain;                // the main input and domain the inputs' sent ticks are measured on
    DataDescriptorPtr tickedMainDomain;

    // Limits: the bytes queued over all inputs, and what the critical limit dropped, freed outside the mutex
    SizeT queuedBytes = 0;
    std::vector<multi_reader2::InputQueue::Backlog> dropped;

    // Status: event fields pending for the next status, and what the previous status showed
    bool afresh = true;            // the first status after a configure: owed whatever it shows
    bool domainChanged = false;
    bool resynchronized = false;
    bool statusOwed = true;        // the next status has changes; it holds back samples
    Shown shown;
    MultiReader2StatusPtr steadyStatus;  // the last status without changes, returned again until a change

    // Wake
    // frontAvailable() > 0 without a pass over the inputs, for the wake decision on a sending thread: the healthy inputs, and
    // those with fewer than MinReadCount samples in front. Kept on ingest, counted again after reads, judgements and
    // synchronization, which pass over the inputs anyway
    SizeT healthyInputs = 0;
    SizeT shortInputs = 0;
    // While synchronizing: the healthy inputs the common start waits for, with nothing in front but an event other than a
    // failure marker. At 0 the pass over the inputs can find the start; before that a new input is judged on its own
    SizeT waitingInputs = 0;

    bool callPending = false;       // an onDataAvailable call is scheduled or running
    bool retryCall = false;         // the last call failed: the next arrival calls again for whatever is pending
    bool arrivedDuringCall = false;  // an arrival since the call started
    std::atomic<std::thread::id> callbackThread{std::thread::id()};  // inCallback is asked without the mutex
};

END_NAMESPACE_OPENDAQ
