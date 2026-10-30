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
#include <coretypes/function_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/component_ptr.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/input_port_notifications.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/multi_reader2.h>
#include <opendaq/multi_reader2_data_manager.h>
#include <opendaq/multi_reader2_params.h>
#include <opendaq/scheduler_ptr.h>
#include <opendaq/work_ptr.h>
#include <memory>
#include <vector>

#include <mutex>

BEGIN_NAMESPACE_OPENDAQ

// Stores what it is given; configure and construction validate
class MultiReader2ParamsImpl : public ImplementationOf<IMultiReader2Params>
{
public:
    ErrCode INTERFACE_FUNC getInputs(IList** inputs) override;
    ErrCode INTERFACE_FUNC setInputs(IList* inputs) override;
    ErrCode INTERFACE_FUNC getMainInput(IComponent** input) override;
    ErrCode INTERFACE_FUNC setMainInput(IComponent* input) override;
    ErrCode INTERFACE_FUNC getAcceptsDescriptor(IFunction** acceptsDescriptor) override;
    ErrCode INTERFACE_FUNC setAcceptsDescriptor(IFunction* acceptsDescriptor) override;
    ErrCode INTERFACE_FUNC getValueReadType(SampleType* valueReadType) override;
    ErrCode INTERFACE_FUNC setValueReadType(SampleType valueReadType) override;
    ErrCode INTERFACE_FUNC getMinReadCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC setMinReadCount(SizeT count) override;
    ErrCode INTERFACE_FUNC getErrorPolicy(MultiReader2ErrorPolicy* policy) override;
    ErrCode INTERFACE_FUNC setErrorPolicy(MultiReader2ErrorPolicy policy) override;

private:
    std::mutex mutex;
    ListPtr<IComponent> inputs = List<IComponent>();
    ComponentPtr mainInput;
    FunctionPtr acceptsDescriptor;
    SampleType valueReadType = SampleType::Float64;
    SizeT minReadCount = 1;
    MultiReader2ErrorPolicy errorPolicy = MultiReader2ErrorPolicy::Invalidate;
};

// The listener a MultiReader2 installs on its ports: a port whose listener supports it is held by a reader
DECLARE_OPENDAQ_INTERFACE(IMultiReader2Intake, IBaseObject)
{
};

/*!
 * @brief The reader facade: resolves the params, takes the ports over, and runs the onDataAvailable handler the data manager
 * asks for. The ports' listener is the intake, which shares the ingest state with the reader: ingest never holds the
 * reader, so a released reader is destroyed at once and its scheduled handler call is skipped.
 */
class MultiReader2Impl : public ImplementationOfWeak<IMultiReader2>
{
public:
    explicit MultiReader2Impl(IMultiReader2Params* params);
    ~MultiReader2Impl() override;

    ErrCode INTERFACE_FUNC configure(IMultiReader2Params* params, IMultiReader2Status** status) override;
    ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC read(void* data, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status) override;
    ErrCode INTERFACE_FUNC readWithDomain(void* data, SizeT* count, SizeT timeoutMs, IMultiReader2Status** status) override;
    ErrCode INTERFACE_FUNC skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status) override;
    ErrCode INTERFACE_FUNC getOnDataAvailable(IEvent** event) override;

private:
    // One input and its port; the intake lock keeps the port's packets and connection changes in order to the data manager
    struct Port
    {
        ComponentPtr input;
        InputPortConfigPtr port;
        IInputPort* key = nullptr;
        bool owned = false;  // created by the reader around a signal input
        std::mutex intake;
    };
    using Wiring = std::vector<std::shared_ptr<Port>>;

    // What ingest needs, shared by the reader and its intake
    struct Core
    {
        MultiReader2DataManager dataManager;
        mutable std::mutex wiringMutex;
        std::shared_ptr<const Wiring> wiring = std::make_shared<Wiring>();
        EventEmitter<BaseObjectPtr, EventArgsPtr<>> onDataAvailable;
        // Under wiringMutex
        SchedulerPtr scheduler;
        WorkPtr work;  // the handler call; holds the reader weakly

        std::shared_ptr<Port> portOf(IInputPort* port) const;
        void connected(IInputPort* port);
        void disconnected(IInputPort* port);
        void ingest(Port& port);
        bool subscribed() const;
        void schedule();
    };
    class Intake;

    void apply(IMultiReader2Params* params, IMultiReader2Status** status);
    static void release(Port& entry);
    bool runHandler();
    ErrCode notFromJudgement() const;

    std::shared_ptr<Core> core = std::make_shared<Core>();
    InputPortNotificationsPtr intake;
    std::mutex configureMutex;
    LoggerComponentPtr loggerComponent;
};

END_NAMESPACE_OPENDAQ
