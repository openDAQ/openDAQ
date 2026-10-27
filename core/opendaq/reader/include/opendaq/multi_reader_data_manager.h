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
#include <opendaq/multi_reader2_input_status.h>
#include <opendaq/multi_reader2_status_ptr.h>
#include <opendaq/packet_ptr.h>
#include <opendaq/sample_type.h>

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

// Internal. The reader in front of it maps ports to slots and holds one intake lock per port, so a port's packets
// and connection changes arrive here in order. Every method runs under this class's single non-recursive mutex,
// held for the whole call. The one call-out is acceptsDescriptor, made under the mutex on the consumer's thread;
// reader calls from inside it are refused. See spec 0007, Data manager sketch.
//
// Implemented so far: configure without data (reconfigure, isSameConfig, rejudge) and the status probe. The queues,
// synchronization, reads with data and the wake are still to be written.
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
    };

    // Equal params skip the reinitialization (Configure); never true before the first reconfigure
    bool isSameConfig(const Config& config);

    // Applies every queued event, drops the data, then a 0-count read. A null status leaves the first status owed
    // to the first read, as after construction
    ErrCode reconfigure(Config newConfig, IMultiReader2Status** status);

    // Equal params: asks acceptsDescriptor for every input with its descriptors in force, then a 0-count read (B5).
    // Not in the spec's sketch yet, whose isSameConfig comment still says "configure is a no-op when true"
    ErrCode rejudge(IMultiReader2Status** status);

    // Consumer side; contracts as on IMultiReader2, data as the real void**. Errors and descriptors move only here
    SizeT getAvailableCount();
    ErrCode read(void** data, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status);
    ErrCode readWithDomain(void** data, SizeT* count, SizeT timeoutMs, IMultiReader2Status** status);
    ErrCode skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status);

    // Producer side; each returns whether onDataAvailable is to be scheduled once the call is out of the mutex.
    // Connection changes and event packets queue as events; a connect carries the signal's descriptors. Data
    // packets queue behind them and run the synchronization check; a failure found there is queued as a marker
    // at its packet and applied by the read that reaches it
    bool connected(SizeT slot, const DataDescriptorPtr& value, const DataDescriptorPtr& domain);
    bool disconnected(SizeT slot);
    bool addPacket(SizeT slot, const PacketPtr& packet);

    bool hasTrigger() const;   // enough samples, a status with changes pending, or a SyncFailed input that can clear

    // After the handler returns: clears the pending flag and decides on another call in one step. After a failed
    // call, only a trigger that arrived since the call started schedules again, and once-per-cause triggers re-arm
    bool callFinished(bool failed);

    bool inCallback() const;   // true on the thread inside acceptsDescriptor

private:
    enum class State { Invalid, Syncing, Streaming };  // Syncing: valid, common start not reached
    enum class SyncCause { None, Late, OffGrid };      // public error SyncFailed; the cause picks the recovery rule

    // The domain descriptor as the checks see it; not in the spec's sketch, which leaves the domain arithmetic to the code
    struct DomainInfo
    {
        bool valid = false;         // the constraints that need only the input itself hold
        RatioPtr resolution;        // seconds per tick
        std::int64_t delta = 0;     // ticks per sample
        std::int64_t start = 0;     // a packet's first tick is packetOffset + start
        // TODO(ingest): in main ticks, ((packetOffset + start) * resolution + origin difference) / main resolution
        std::int64_t rate = 0;      // samples per second
        std::int64_t originS = 0;   // origin in whole epoch seconds
    };

    struct Slot
    {
        // TODO(ingest): queue, frontCount and newestTick of the spec's sketch
        DataDescriptorPtr latestValue, latestDomain; // last seen at ingest; what a reconfigure evaluates
        bool latestConnected = false;
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
        // TODO(wake): clearPending
        bool descriptorChanged = false;              // pending for the next status
    };

    static DomainInfo parseDomain(const DataDescriptorPtr& domain);
    static bool matchesMain(const DomainInfo& input, const DomainInfo& main);

    // All under the mutex
    void checkInput(SizeT slot);
    bool askAccept(SizeT slot);
    void settleMainInput(bool keepMain);
    void restart();
    ComponentStatus level() const;
    std::string describeError(const Slot& slot) const;
    SizeT sampleSize(const Slot& slot) const;
    MultiReader2StatusPtr buildStatus();
    enum class Buffers { Values, WithDomain, None };   // read, readWithDomain, skipSamples
    ErrCode checkRead(SizeT count, SizeT timeoutMs, void** data, Buffers buffers) const;
    void readStatus(SizeT* count, SizeT* packetOffset, IMultiReader2Status** status);

    std::mutex mutex;
    Config config;
    bool configured = false;                        // not in the sketch: the first reconfigure is never skipped
    std::vector<Slot> slots;
    std::optional<SizeT> mainSlot;                  // the main input in effect
    State state = State::Invalid;
    // TODO(sync): commonStart, readTick and syncLimitTicks of the spec's sketch
    bool domainChanged = false;                     // pending for the next status
    bool resynchronized = false;                    // pending for the next status
    bool statusOwed = false;                        // not in the sketch: hasChanges of the next status (C1)
    // TODO(wake): callPending and triggers of the spec's sketch
    std::atomic<std::thread::id> callbackThread{std::thread::id()};   // not in the sketch: inCallback is asked without the mutex
};

END_NAMESPACE_OPENDAQ
