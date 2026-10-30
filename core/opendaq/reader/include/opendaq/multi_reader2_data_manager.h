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
#include <opendaq/event_packet_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/multi_reader2_domain.h>
#include <opendaq/multi_reader2_params.h>
#include <opendaq/multi_reader2_queue.h>
#include <opendaq/multi_reader2_slot.h>
#include <opendaq/multi_reader2_status_ptr.h>
#include <opendaq/packet_ptr.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Internal. The reader in front of it maps ports to keys and holds one intake lock per port, so a port's packets and
 * connection changes arrive here in order. Every method runs under this class's single non-recursive mutex, held for the
 * whole call, except that a read converts the samples it took off the queues after releasing it. The one call-out is
 * acceptsDescriptor, made under the mutex on the consumer's thread; reader calls from inside it are refused.
 */
class MultiReader2DataManager final
{
public:
    // The params resolved to slots
    struct Config
    {
        std::vector<ComponentPtr> inputs;               // slot order; the input statuses hand these back
        std::vector<const void*> keys;                  // the reader's identity of each input's port
        std::vector<multi_reader2::Announced> current;  // what new slots start from; retained slots keep what they saw
        std::optional<SizeT> mainSlot;                  // pinned main input; unset = the reader chooses
        FunctionPtr acceptsDescriptor;                  // null accepts everything
        SampleType readType = SampleType::Float64;
        SizeT minReadCount = 1;
        MultiReader2ErrorPolicy errorPolicy = MultiReader2ErrorPolicy::Invalidate;
        LoggerComponentPtr loggerComponent;  // null logs nothing
    };

    // Construction: applies the config without reading; the first read reports it
    void construct(Config config);
    // Reinitializes, or with equal params judges every input again, and returns the status of a 0-count read
    void configure(Config config, IMultiReader2Status** status);

    // Consumer side; contracts as on IMultiReader2, data as the real void**. Errors and descriptors move only here
    SizeT getAvailableCount();
    ErrCode read(void** data, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status);
    ErrCode readWithDomain(void** data, SizeT* count, SizeT timeoutMs, IMultiReader2Status** status);
    ErrCode skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status);

    // Producer side; each returns whether to schedule onDataAvailable after the mutex: only while subscribed and no call is pending
    bool connected(const void* key, const multi_reader2::Announced& announced, bool subscribed);
    bool disconnected(const void* key, bool subscribed);
    bool addPacket(const void* key, const PacketPtr& packet, bool subscribed);

    // Triggers so far; a failed call reschedules only when it moved
    std::uint64_t triggerCount();
    // After the handler: clears the pending flag and decides on another call; a failed call waits for a trigger after it
    bool callFinished(bool failed, std::uint64_t triggersAtStart, bool subscribed);
    bool inCallback() const;  // true on the thread inside acceptsDescriptor

private:
    using SyncCause = multi_reader2::SyncCause;
    using Announced = multi_reader2::Announced;
    using Event = multi_reader2::Event;
    using Slot = multi_reader2::Slot;

    // Invalid: nothing is read. Idle: valid without inputs. Synchronizing: valid, the common start not reached
    enum class Phase
    {
        Invalid,
        Idle,
        Synchronizing,
        Streaming
    };

    // What the next status reports as changed; force marks the first status of a configuration
    struct Pending
    {
        bool domain = false;
        bool resync = false;
        bool force = false;
    };

    // The fields of a status that hold until the reader changes
    struct State
    {
        struct Input
        {
            MultiReader2InputError error = MultiReader2InputError::None;
            DataDescriptorPtr descriptor;  // null in error
            SizeT sampleSize = 0;
        };
        ComponentStatus level = ComponentStatus::Ok;
        std::string message;
        ComponentPtr main;
        DataDescriptorPtr domain;
        std::vector<Input> inputs;
        bool operator==(const State& other) const;
    };

    // Samples a read hands out, converted into the read buffer once the mutex is released
    struct Copy
    {
        DataPacketPtr packet;
        SizeT from;
        SizeT count;
        std::uint8_t* dst;
    };

    void apply(Config config, bool reinitialize);
    ErrCode doRead(
        void** data, bool withDomain, bool copy, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status);
    // Copying reads list their samples in copies; the others pass null
    void readLocked(void** data,
                    bool withDomain,
                    bool copy,
                    SizeT* count,
                    SizeT* packetOffset,
                    IMultiReader2Status** status,
                    std::vector<Copy>* copies);
    void buildStatus(IMultiReader2Status** status);
    State captureState() const;
    struct Changes
    {
        bool force = false;
        bool domain = false;
        bool resync = false;
        bool descriptors = false;
        bool any() const
        {
            return force || domain || resync || descriptors;
        }
    };
    Changes pendingChanges() const;
    bool descriptorReported(const Slot& s) const;
    bool pendingEventFields() const;
    bool stateChanged() const;

    // Judgement
    void ask(SizeT i, const ListPtr<IDataDescriptor>& values);
    void rejudge(SizeT i, const ListPtr<IDataDescriptor>& values);
    ListPtr<IDataDescriptor> valuesInForce() const;
    std::pair<MultiReader2InputError, std::string> judge(SizeT i, bool alone) const;
    void evaluate(SizeT i);
    void evaluateAll();
    void relateAll();
    bool valid() const
    {
        return phase != Phase::Invalid;
    }
    bool contributing(SizeT i) const;
    bool computeValid() const;
    std::optional<SizeT> firstHealthy() const;

    // Read path
    SizeT alignedCount() const;
    bool barrierBounded(SizeT k) const;
    bool applyReached();
    void applyEvent(SizeT i, const Event& event);
    void settle(bool wasValid, bool mainDomainChanged);
    void internalReconfigure(bool recovered);
    void restart();
    void startSync(bool fromStreaming);
    bool trySync();

    // Ingest
    std::optional<SizeT> slotOf(const void* key);
    bool ingestEvent(SizeT i, const EventPacketPtr& event);
    bool ingestData(SizeT i, const DataPacketPtr& data);
    bool checkAgainstMain();
    bool checkLate(SizeT i);
    bool trigger(bool cause, bool subscribed);
    bool readable() const;
    bool eventReached(SizeT i) const;
    bool statusOwed() const;
    bool hasTrigger() const;
    const multi_reader2::TickDomain& mainTicks() const;

    std::mutex mutex;
    std::atomic<std::thread::id> callbackThread;
    std::vector<Slot> slots;
    SizeT lastSlot = 0;             // the slot of the previous ingest
    std::vector<SizeT> syncJudged;  // trySync's inputs, kept so a packet allocates nothing
    std::vector<Copy> spareCopies;  // a read's copy list, kept so a read allocates nothing
    FunctionPtr acceptsDescriptor;
    LoggerComponentPtr loggerComponent;
    SampleType readType = SampleType::Float64;
    SizeT minReadCount = 1;
    MultiReader2ErrorPolicy errorPolicy = MultiReader2ErrorPolicy::Invalidate;
    std::optional<SizeT> pinnedMain;
    std::optional<SizeT> mainSlot;  // in effect; kept while invalid with an automatic main input
    bool configured = false;
    Phase phase = Phase::Invalid;
    std::optional<std::int64_t> syncMainFirst;  // main tick of the main input's first sample since synchronization started
    std::int64_t readTick = 0;                  // main tick of the next unread sample
    std::optional<std::int64_t> lastEnd;        // main tick after the last sample delivered since the last resynchronized
    std::int64_t syncLimit = 0;
    Pending owed;
    std::optional<State> lastState;
    bool stateDirty = true;               // lastState may differ from the reader; set by every judgement
    MultiReader2StatusPtr lastUnchanged;  // the previous status as one without changes
    bool callPending = false;
    bool rearmed = false;  // a call failed: a queued clear triggers once more
    std::uint64_t triggers = 0;
};

END_NAMESPACE_OPENDAQ
