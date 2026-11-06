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
#include <opendaq/connection_internal.h>
#include <opendaq/connection_ptr.h>
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
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

// The listener a MultiReader2 installs on its ports: a port whose listener supports it is held by a reader
DECLARE_OPENDAQ_INTERFACE(IMultiReader2PortListener, IBaseObject)
{
};

/*!
 * @brief The reader facade: owns the ports, wires each to a slot of the data manager, runs the ingest task and the
 * onDataAvailable calls. The engine work lives in MultiReaderDataManager.
 *
 * Threads (spec 0007, Threading and locking): a port notifies on the sending thread. With no ingest run scheduled or
 * running, the sending thread takes its packets in itself, under locks it only tries, so it never waits; a held lock, or a
 * run in progress, leaves them to the ingest task, which drains every marked port. The handler call is decided with the
 * packets, in O(1), and is a scheduler task of its own, so reads run alongside ingest. Connect and disconnect callbacks run
 * on the thread that changes the port.
 *
 * Locks, in order: the facade mutex (configure and reevaluate only), a port's intake lock, the data manager's mutex. A
 * sending thread only tries the last two. The port callbacks and the ingest task never take the facade mutex: they reach a
 * port's slot through the port's own Intake and the wiring snapshot, so configure may run from inside the onDataAvailable
 * handler. No signal is called under an intake lock.
 */
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
    // One port the reader holds. Under its lock the port's packets, connect and disconnect reach the data manager in order;
    // configure moves the slot index under it too, unset while configure takes the port over and after the port left.
    // The mark is set by the sending thread without the lock, and cleared by the ingest task before it drains the port
    struct Intake
    {
        explicit Intake(InputPortPtr port)
            : port(std::move(port))
        {
        }

        const InputPortPtr port;  // held, so an ingest run on an older wiring never meets a destroyed port
        std::mutex mutex;
        std::optional<SizeT> slot;
        ObjectPtr<IConnectionInternal> connection;  // under the lock: the port's connection, so a packet does not ask the port
        std::atomic<bool> marked{false};
    };

    // One reader input: the component the user named, the port driving its slot, and the port's listener, which the port
    // holds weakly
    struct Slot
    {
        ComponentPtr input;
        InputPortConfigPtr port;
        bool ownsPort = false;  // the reader created the port around a signal input
        std::shared_ptr<Intake> intake;
        InputPortNotificationsPtr listener;
    };

    // Read without a lock by the ingest task and the wake
    struct Wiring
    {
        std::vector<std::shared_ptr<Intake>> intakes;
        SchedulerPtr scheduler;
        LoggerComponentPtr loggerComponent;
    };

    // What the port callbacks and the scheduled work need. The port listeners hold this and not the reader, so a
    // callback in flight does not keep a released reader alive; a destroyed reader clears the wiring
    struct Core
    {
        MultiReaderDataManager dataManager;
        EventEmitter<BaseObjectPtr, EventArgsPtr<>> onDataAvailable;
        std::shared_ptr<const Wiring> wiring;
        WorkPtr wakeWork;    // holds the reader weakly
        WorkPtr ingestWork;  // holds this weakly
        multi_reader2::IngestTask ingestTask;

        std::shared_ptr<const Wiring> currentWiring() const;
        void applyConnect(Intake& intake);
        void applyDisconnect(Intake& intake);
        void notify(Intake& intake);
        bool takeIn(Intake& intake);
        void scheduleIngest();
        void drainMarked();
        MultiReaderDataManager::Ingested ingest(Intake& intake);
        SizeT transferBatch(Intake& intake, const std::unique_lock<std::mutex>& engineLock, MultiReaderDataManager::Ingested& ingested);
        void markForTask(Intake& intake);
        void scheduleWake();

        static constexpr SizeT batch = 64;        // packets per transfer
        static constexpr SizeT maxBatches = 256;  // per port per ingest run, so a busy port does not hold up the others
    };

    // What a port is connected to, with the signal's descriptors
    struct PortState
    {
        ConnectionPtr connection;
        bool connected = false;
        DataDescriptorPtr value, domain;
    };

    class PortListener;

    ErrCode configureInternal(IMultiReader2Params* params, IMultiReader2Status** status);
    static ErrCode resolveParams(IMultiReader2Params* params, MultiReaderDataManager::Config& config);
    ErrCode checkInputs(const MultiReaderDataManager::Config& config, std::vector<bool>& retained) const;
    std::vector<Slot>::const_iterator findSlot(const ComponentPtr& input) const;
    Slot makeSlot(const ComponentPtr& input);
    static void takeOver(const Slot& slot);
    static void releaseSlot(Slot& slot);
    static PortState readPortState(const InputPortPtr& port);
    void publishWiring(const std::vector<Slot>& wired, const SchedulerPtr& scheduler, const LoggerComponentPtr& loggerComponent);
    bool runWake();

    std::mutex mutex;
    std::vector<Slot> slots;
    std::shared_ptr<Core> core;
};

END_NAMESPACE_OPENDAQ
