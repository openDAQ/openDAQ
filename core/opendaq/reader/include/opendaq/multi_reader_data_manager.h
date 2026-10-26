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
#include <opendaq/multi_reader2_status.h>
#include <opendaq/packet_ptr.h>
#include <opendaq/sample_type.h>

#include <optional>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

// Internal. The reader in front of it maps ports to slots and holds one intake lock per port, so a port's packets
// and connection changes arrive here in order. Every method runs under this class's single non-recursive mutex,
// held for the whole call. The one call-out is acceptsDescriptor, made under the mutex on the consumer's thread;
// reader calls from inside it are refused. See spec 0007, Data manager sketch.
//
// Placeholder: the surface follows the spec's sketch; every body is still to be written.
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

    // Equal params skip the reinitialization (Configure)
    bool isSameConfig(const Config& config);

    // Applies every queued event, drops the data, then a 0-count read
    ErrCode reconfigure(Config config, IMultiReader2Status** status);

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
    // To be written: the state of the spec's sketch (slots, queues, front runs, state machine, sync, wake)
};

END_NAMESPACE_OPENDAQ
