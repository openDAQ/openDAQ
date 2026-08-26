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
#include <ref_fb_module/common.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/multi_reader2.h>
#include <opendaq/multi_reader2_status.h>

#include <vector>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace SumReader
{

/*!
 * Reference consumer of MultiReader2. Sums any number of equal-rate input signals into one
 * output signal, driven by the reader's event protocol:
 *
 * - Dynamic ports: one spare disconnected port always exists and rides along in the reader
 *   as an unused input. Connecting it promotes it and spawns a fresh spare.
 * - Every connect, disconnect, and BadInputHandling swap rebuilds the params and calls
 *   `configure` - the reader's single mutation path. Everything else goes through the
 *   event window: read -> on Event: setUsed/setActive -> commitEvent -> read data.
 * - `BadInputHandling` selects the reaction to per-input errors: `Exclude` parks the failing
 *   inputs with setUsed(false), `Deactivate` stops the whole reader with setActive(false).
 *   A clean descriptor handshake is the recovery evidence that lifts either reaction.
 */
class SumReaderFbImpl final : public FunctionBlock
{
public:
    explicit SumReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    ~SumReaderFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

private:
    std::string getNextPortID() const;

    void initProperties();
    void createSignals();
    void createDisconnectedPort();
    void createReader();
    void updateInputPortsLocked();
    void reconfigureReaderLocked();
    void badInputHandlingChanged();

    void onPortEventLocked();
    void onDataReceived();
    void handleEventLocked(const ObjectPtr<IMultiReader2Status>& status);
    void configureOutputLocked(const ObjectPtr<IMultiReader2Status>& status);
    void emitSumLocked(const std::vector<std::vector<double>>& buffers, SizeT count, SizeT offset);

    // Errors the block reacts to by excluding an input; liveness conditions resolve on their own
    static bool isValidityError(MultiReader2InputError error);
    StringPtr mainInputIdLocked() const;
    UnitPtr referenceUnitLocked(const DictPtr<IString, IDataDescriptor>& descriptors) const;

    std::vector<InputPortPtr> connectedPorts;
    InputPortPtr disconnectedPort;

    bool excludeBadInputs = true;

    DataDescriptorPtr sumDataDescriptor;
    DataDescriptorPtr sumDomainDataDescriptor;

    SignalConfigPtr sumSignal;
    SignalConfigPtr sumDomainSignal;

    ObjectPtr<IMultiReader2> reader;
};
}

END_NAMESPACE_REF_FB_MODULE
