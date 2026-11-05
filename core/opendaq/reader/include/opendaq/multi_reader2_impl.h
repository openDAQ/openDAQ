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
#include <coretypes/event_args_ptr.h>
#include <coretypes/event_emitter.h>
#include <coretypes/intfs.h>
#include <opendaq/connection_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/input_port_notifications_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/multi_reader2.h>
#include <opendaq/multi_reader2_ingest_task.h>
#include <opendaq/multi_reader_data_manager.h>
#include <opendaq/scheduler_ptr.h>
#include <opendaq/work_ptr.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

// The reader facade: owns the ports, installs a port listener on each, translates ports into slot indices, schedules
// the wake the manager says is owed. The engine work lives in MultiReaderDataManager. The facade mutex guards the slot
// list during configure only; the ingest task and the connect callbacks look the port's intake up through an immutable
// wiring snapshot and never take it, so configure may run from inside the onDataAvailable handler. Lock order: the
// facade mutex, the intake locks, the data manager's mutex.
//
// The ports notify on the sending thread, which only marks its port and, when the ingest task is idle, schedules it. The
// task drains every marked port, so a burst of packets on any number of ports costs one scheduler task.
class MultiReader2Impl : public ImplementationOfWeak<IMultiReader2>
{
public:
    explicit MultiReader2Impl(IMultiReader2Params* params);
    ~MultiReader2Impl() override;

    // IMultiReader2
    ErrCode INTERFACE_FUNC configure(IMultiReader2Params* params, IMultiReader2Status** status) override;
    ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC read(void* data, void* domain, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status) override;
    ErrCode INTERFACE_FUNC skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status) override;
    ErrCode INTERFACE_FUNC getOnDataAvailable(IEvent** event) override;
    ErrCode INTERFACE_FUNC reevaluate(IMultiReader2Status** status) override;

private:
    // One port's intake lock (spec A12): under it the port's packets, connect and disconnect reach the data manager
    // in order. configure moves the slot index under it too; unset while configure takes the port over, and after
    // the port left. No signal is called under it (C18). The mark is set by the sending thread, without the lock, and
    // cleared by the ingest task before it drains the port
    struct Intake
    {
        std::mutex mutex;
        std::optional<SizeT> slot;
        std::atomic<bool> marked{false};
    };

    // One reader input: the component the user named, the port driving its slot, and the port's listener, which the
    // port holds weakly
    struct Slot
    {
        ComponentPtr input;
        InputPortConfigPtr port;
        bool ownsPort;  // true for the port the reader created around a signal input
        std::shared_ptr<Intake> intake;
        InputPortNotificationsPtr listener;
    };

    // A wired port, held strongly, so an ingest run that still works from an older wiring never meets a destroyed port
    struct Wired
    {
        InputPortPtr port;
        std::shared_ptr<Intake> intake;
    };

    // Read without a lock by the port callbacks, the ingest task and the wake
    struct Wiring
    {
        std::unordered_map<IInputPort*, Wired> intakeOf;
        SchedulerPtr scheduler;
        LoggerComponentPtr loggerComponent;
    };

    // What a port is connected to, with the signal's descriptors
    struct PortState
    {
        ConnectionPtr connection;
        bool connected = false;
        DataDescriptorPtr value, domain;
    };

    // What the port callbacks need, shared by the reader and its port listener. The ports hold the listener, which holds
    // this and not the reader, so a callback in flight does not keep a released reader alive (W4). A destroyed reader
    // clears the wiring, and the callbacks after it find no intake
    struct Core
    {
        MultiReaderDataManager dataManager;
        std::shared_ptr<const Wiring> wiring;
        WorkPtr wakeWork;     // holds the reader weakly
        WorkPtr ingestWork;   // holds this weakly
        multi_reader2::IngestTask ingestTask;

        std::shared_ptr<Intake> intakeOf(IInputPort* port) const;
        void applyConnect(IInputPort* port);
        void applyDisconnect(IInputPort* port);
        void notify(Intake& intake);
        void scheduleIngest();
        void drainMarked();
        bool ingest(IInputPort* port, Intake& intake);
        void scheduleWake();
    };

    // The listener the reader installs on its ports; defined with the callbacks
    class PortListener;

    // configure, and the constructor with a null status: the first status is then owed to the first read
    ErrCode configureInternal(IMultiReader2Params* params, IMultiReader2Status** status);
    ErrCode resolveParams(IMultiReader2Params* params, MultiReaderDataManager::Config& config, ComponentPtr& mainInput);
    Slot makeSlot(const ComponentPtr& input);
    void takeOver(const Slot& slot);
    void releaseSlot(Slot& slot);
    static PortState readPortState(const InputPortPtr& port);
    void publishWiring(const std::vector<Slot>& wired);
    bool runWake();

    std::mutex mutex;
    EventEmitter<BaseObjectPtr, EventArgsPtr<>> onDataAvailable;
    ContextPtr context;
    SchedulerPtr scheduler;
    LoggerComponentPtr loggerComponent;
    std::vector<Slot> slots;
    std::shared_ptr<Core> core;
};

END_NAMESPACE_OPENDAQ
