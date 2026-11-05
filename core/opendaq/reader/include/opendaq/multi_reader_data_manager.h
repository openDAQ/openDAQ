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
#include <opendaq/multi_reader2_params.h>
#include <coretypes/ratio_ptr.h>
#include <opendaq/component.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/multi_reader2_input_status.h>
#include <opendaq/multi_reader2_status_ptr.h>
#include <opendaq/packet_ptr.h>
#include <opendaq/sample_type.h>
#include <opendaq/typed_reader.h>

#include <atomic>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

// Internal. The reader in front of it maps ports to slots and holds one intake lock per port, so a port's packets
// and connection changes arrive here in order. Every method runs under this class's single non-recursive mutex,
// held for the whole call. The one call-out is acceptsDescriptor, made under the mutex on the consumer's thread;
// reader calls from inside it are refused. The one exception to the single mutex is a read's conversion: the samples
// are taken off the queues under it and converted into the user's buffers after it. See spec 0007, Data manager sketch.
class MultiReaderDataManager final
{
public:
    // The params resolved to slot indices
    struct Config
    {
        std::vector<ComponentPtr> inputs;                  // slot order; the input statuses hand these back
        std::vector<bool> connected;                       // per slot at configure time; always true for signals
        std::vector<DataDescriptorPtr> valueDescriptors;   // what new slots start from; retained slots keep what they saw
        std::vector<DataDescriptorPtr> domainDescriptors;
        std::optional<SizeT> mainSlot;                     // pinned main input; unset = the reader chooses
        FunctionPtr acceptsDescriptor;                     // null accepts everything
        SampleType readType = SampleType::Float64;
        SizeT minReadCount = 1;
        MultiReader2ErrorPolicy errorPolicy = MultiReader2ErrorPolicy::Invalidate;
        LoggerComponentPtr loggerComponent;                // for an acceptsDescriptor that throws; may be null
    };

    // Applies every queued event, drops the data, then a 0-count read. A null status leaves the first status owed
    // to the first read, as after construction
    ErrCode reconfigure(Config newConfig, IMultiReader2Status** status);

    // Asks acceptsDescriptor for every input with its descriptors in force, then a 0-count read (B5)
    ErrCode reevaluate(IMultiReader2Status** status);

    // Consumer side; contracts as on IMultiReader2, data as the real void**. Errors and descriptors move only here
    SizeT getAvailableCount();
    ErrCode read(void** data, std::int64_t* domain, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status);
    ErrCode skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status);

    // Producer side; each returns whether onDataAvailable is to be scheduled once the call is out of the mutex, and
    // then claims the one pending call. Connection changes and event packets queue as events; a connect carries the
    // signal's descriptors. Data packets queue behind them and run the synchronization check; a failure found there
    // is queued as a marker in front of the input's data and applied by the next read
    bool connected(SizeT slot, const DataDescriptorPtr& value, const DataDescriptorPtr& domain);
    bool disconnected(SizeT slot);
    bool addPacket(SizeT slot, const PacketPtr& packet);
    bool addPackets(SizeT slot, const PacketPtr* packets, SizeT count);

    // The next read would change something: samples, a status with changes, a SyncFailed input that can clear, or an
    // event it applies
    bool hasTrigger();

    void callStarted();   // the handler is about to run
    // After the handler returns: clears the pending flag and decides on another call in one step, claiming it. Only an
    // arrival since the call started that is still unread calls again; after a failed call once-per-cause triggers re-arm
    bool callFinished(bool failed);
    void callAborted();   // the claimed call could not be scheduled; not in the sketch

    bool inCallback() const;   // true on the thread inside acceptsDescriptor

private:
    // Each input's queue holds at most this much data; the oldest goes while synchronizing, all of it otherwise
    static constexpr std::int64_t queueLimitSeconds = 3;
    static constexpr SizeT queueLimitBytes = SizeT(100) * 1000 * 1000;

    enum class State { Invalid, Syncing, Streaming };  // Syncing: valid, common start not reached
    enum class SyncCause { None, Limit, OffGrid };     // public error SyncFailed; the cause picks the recovery rule

    // The domain descriptor as the checks see it; not in the spec's sketch, which leaves the domain arithmetic to the code
    struct DomainInfo
    {
        bool valid = false;         // the constraints that need only the input itself hold
        RatioPtr resolution;        // seconds per tick
        std::int64_t delta = 0;     // ticks per sample
        std::int64_t start = 0;     // a packet's first tick is packetOffset + start
        std::int64_t rate = 0;      // samples per second
        std::int64_t originS = 0;   // origin in whole epoch seconds
    };

    // An input tick in main ticks is tick * p / q + offset; not in the sketch
    struct TickMap
    {
        bool valid = false;         // a main input with a valid domain, and an origin difference of whole main ticks
        std::int64_t p = 1, q = 1;  // input resolution / main resolution, reduced
        std::int64_t offset = 0;    // the origin difference in main ticks
    };

    // A queued event on one input, applied when a read reaches it
    struct Event
    {
        enum class Kind { Descriptor, Connected, Disconnected, Gap, SyncFailed };
        Kind kind;
        // Descriptor and Connected: both descriptors after the event, so a removal is a null; the sketch has only
        // the changed ones assigned, which cannot express a removal
        DataDescriptorPtr value, domain;
        SyncCause cause = SyncCause::None; // SyncFailed: a failure found at ingest, queued in front of the input's data
    };

    // What applying one event changed on its input; not in the sketch
    struct Applied
    {
        bool judge = false;    // descriptors or connection: the input is checked again
        bool domain = false;   // a new domain descriptor
        bool gap = false;
    };

    // One queue element: a data packet with its position in main ticks, or an event
    struct Entry
    {
        DataPacketPtr packet;
        SizeT start = 0;                        // samples of the packet already discarded; not in the sketch
        SizeT count = 0;                        // samples left from start; not in the sketch
        std::optional<std::int64_t> firstTick;  // main tick of sample start; unset when it is no whole main tick
        std::optional<Event> event;
        SizeT bytes = 0;                        // the packet's value and domain bytes, held until it leaves; not in the sketch
    };

    struct Slot
    {
        std::deque<Entry> queue;                     // in ingest order
        SizeT frontCount = 0;                        // samples in front of the first event
        SizeT events = 0;                            // events in the queue; not in the sketch
        SizeT queuedSamples = 0;                     // samples of the queued data, events aside; not in the sketch
        SizeT queuedBytes = 0;                       // bytes of the queued packets; not in the sketch
        std::optional<std::int64_t> newestTick;      // main tick of the last ingested sample, dropped or not
        DataDescriptorPtr latestValue, latestDomain; // last seen at ingest; what a reconfigure evaluates
        bool latestConnected = false;
        DomainInfo latestInfo;                       // parsed from latestDomain; not in the sketch
        TickMap toMain;                              // from latestInfo and the main input's domain in force; not in the sketch
        std::optional<std::int64_t> nextOffset;      // the packetOffset that continues the last packet; not in the sketch
        DataDescriptorPtr value, domain;             // in force, moved only by read
        bool connected = false;
        DomainInfo domainInfo;                       // parsed from domain; not in the sketch
        MultiReader2InputError ownError = MultiReader2InputError::None;   // the input alone; not in the sketch
        bool rejectedByOwner = false;                // ownError came from acceptsDescriptor; picks the message text
        MultiReader2InputError error = MultiReader2InputError::None;      // reported: ownError, then the checks
                                                                          // against the main input, then SyncFailed
        std::string errorText;                       // the input's entry in the status message, set with its error and cause
        SyncCause syncFailed = SyncCause::None;      // applied by the read that reaches its marker; survives an internal
                                                     // reconfigure that keeps the main input
        bool clearPending = false;                   // failed on the limit, a packet now within it: triggers once until a read clears it
        bool descriptorChanged = false;              // pending for the next status
        std::shared_ptr<Reader> converter;           // to the read type, made for converterFor; a copy made outside the
                                                     // mutex holds it; not in the sketch
        DataDescriptorPtr converterFor;
    };

    // Samples a read took off a queue, converted into the user's buffer once the mutex is released; not in the sketch
    struct Copy
    {
        DataPacketPtr packet;
        SizeT start = 0;
        SizeT count = 0;
        void* out = nullptr;
        std::shared_ptr<Reader> converter;  // null under Undefined: the samples are copied as they are
        SizeT sampleSize = 0;               // bytes per sample in the packet, under Undefined
    };

    static DomainInfo parseDomain(const DataDescriptorPtr& domain);
    static bool matchesMain(const DomainInfo& input, const DomainInfo& main);

    // All under the mutex
    void checkInput(SizeT slot);
    bool askAccept(SizeT slot);
    void settleMainInput(bool keepMain);
    void restart();
    void startSync();
    void internalReconfigure(bool wasValid);
    std::vector<std::pair<MultiReader2InputError, std::string>> reportedErrors() const;
    void noteChanges(ComponentStatus previousLevel, const std::vector<std::pair<MultiReader2InputError, std::string>>& previous);

    // Read
    std::optional<SizeT> shortRun() const;
    void dropShortRun();
    static bool contributes(const Slot& slot, bool valid);
    static bool reachesEvent(const Slot& slot, bool contributing);
    void consume(Slot& slot, SizeT count, void* out, std::vector<Copy>* copies);
    void queueCopy(Slot& slot, const Entry& entry, SizeT count, void*& out, std::vector<Copy>& copies);
    static void runCopy(const Copy& copy);
    static Applied applyEvent(Slot& slot, const Event& event);
    static void countFront(Slot& slot);
    void applyEvents();

    // Ingest
    bool queueConnect(SizeT slot, const DataDescriptorPtr& value, const DataDescriptorPtr& domain);
    bool queueDisconnect(SizeT slot);
    bool queuePacket(SizeT slot, const PacketPtr& packet);
    bool queueDescriptor(SizeT slot, const DataDescriptorPtr& value, const DataDescriptorPtr& domain);
    bool ingestData(SizeT slot, const DataPacketPtr& packet);
    void mapToMain(SizeT slot);
    static std::optional<std::int64_t> toMainTick(const Slot& slot, std::int64_t packetOffset);
    static void pushEvent(Slot& slot, Event event);
    static void dropFront(Slot& slot);
    static void dropData(Slot& slot);
    static bool hasData(const Slot& slot);
    void dropForRejoin(const std::vector<SizeT>& rejoined);
    void discardBefore(Slot& slot, std::int64_t tick) const;
    static void popData(Slot& slot);
    static void clearQueue(Slot& slot);
    bool overLimit(const Slot& slot, SizeT count, SizeT bytes) const;
    bool limitQueue(SizeT slot, SizeT count, SizeT bytes);
    static void failSync(Slot& slot, SyncCause cause);
    static bool failedOnLimit(const Slot& slot);
    bool synchronize();
    bool markClear(SizeT slot, const std::optional<std::int64_t>& firstTick);
    SizeT frontAvailable() const;
    ComponentStatus level() const;
    std::string describeError(const Slot& slot) const;
    SizeT sampleSize(const Slot& slot) const;
    MultiReader2StatusPtr buildStatus();

    // Wake
    bool claimWake(bool owed);
    bool triggerHolds() const;
    ErrCode checkRead(SizeT count, SizeT timeoutMs, void** data, bool skip) const;
    void performRead(void** data, std::int64_t* domain, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status,
                     std::vector<Copy>* copies);

    std::mutex mutex;
    Config config;
    std::vector<Slot> slots;
    std::optional<SizeT> mainSlot;                  // the main input in effect
    State state = State::Invalid;
    std::int64_t readTick = 0;                      // main-tick position of the next unread sample
    std::optional<std::int64_t> mainTrimmedFrom;    // the main input's first tick the queue limit dropped while
                                                    // synchronizing; the behind check counts from it
    bool positioned = false;                        // readTick continues a read that delivered samples since the last
                                                    // restart; not in the sketch
    std::int64_t syncLimitTicks = 0;                // 2 s of data in main ticks; set from the main domain
    std::int64_t mainDelta = 1;                     // main ticks per sample; not in the sketch
    bool domainChanged = false;                     // pending for the next status
    bool resynchronized = false;                    // pending for the next status
    bool statusOwed = false;                        // not in the sketch: hasChanges of the next status (C1)
    MultiReader2StatusPtr steadyStatus;             // the last status without changes, returned again until a change;
                                                    // not in the sketch
    bool callPending = false;                       // an onDataAvailable call is scheduled or running
    bool arrivedDuringCall = false;                 // an arrival since the call started; the sketch counts them
    std::atomic<std::thread::id> callbackThread{std::thread::id()};   // not in the sketch: inCallback is asked without the mutex
};

END_NAMESPACE_OPENDAQ
