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
#include <opendaq/function_block_impl.h>
#include <opendaq/multi_reader2_factory.h>
#include <ref_fb_module/common.h>

#include <string>
#include <unordered_map>
#include <vector>

BEGIN_NAMESPACE_REF_FB_MODULE

// What every MultiReader2 block repeats: it creates the reader, drains it under the block's acquisition lock, caches
// descriptors and reports the component status. A block overrides accepts to reject descriptors and onChanges when it
// has more to do
class ReaderFbBase : public FunctionBlock
{
protected:
    ReaderFbBase(const FunctionBlockTypePtr& type, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);

    // The judgement and the onDataAvailable handler hold the block weakly, so neither keeps a removed block alive
    void createReader();

    // Reads until nothing is deliverable and nothing changed; only contributing slots get a buffer
    void drain();

    // Default: cache unless under Error, then report
    virtual void onChanges(const MultiReader2StatusPtr& status);

    // The block convention: an Error whose inputs in error are all only Disconnected is shown as Warning
    static ComponentStatus blockStatus(const MultiReader2StatusPtr& status);

    // A real or integer sample type without dimensions
    static bool scalarNumeric(const DataDescriptorPtr& descriptor);

    // Keeps `cached` at the descriptors of the contributing inputs and rebuilds the output descriptor from it
    void cacheDescriptors(const MultiReader2StatusPtr& status);

    // Asked by the reader under its lock whenever a descriptor takes effect; `values` holds every input's value descriptor
    // in force, in slot order. Must not call the reader
    virtual bool accepts(const ComponentPtr& input, const DataDescriptorPtr& descriptor, const ListPtr<IDataDescriptor>& values);

    // Every input list goes through here, so a block whose judgement depends on the slot order can follow it
    virtual void setInputs(const ListPtr<IComponent>& inputs);

    // The free-port pattern: one port outside the reader, with the block as its listener, until a signal connects
    void addFreePort(const std::string& prefix);
    void onConnected(const InputPortPtr& port) override;
    ListPtr<IComponent> readerPorts() const;

    // The pattern's other half: configures the reader without the inputs `status` reports Disconnected, then removes
    // their ports, so the reader never sees the removal. Returns the removed ports; `next` is the configure's status
    std::vector<InputPortConfigPtr> removeDisconnectedPorts(const MultiReader2StatusPtr& status, MultiReader2StatusPtr& next);

    // `count` samples per contributing slot sit in `buffers` as Float64, `sampleSizes` bytes each
    virtual void processAndSend(SizeT count, SizeT packetOffset) = 0;
    virtual void rebuildOutputDescriptor() = 0;

    void removed() override;

    MultiReader2ParamsPtr params;
    MultiReader2Ptr reader;
    std::vector<std::vector<double>> storage;
    std::vector<void*> buffers;
    DataDescriptorPtr outputDomain;
    std::unordered_map<std::string, DataDescriptorPtr> cached;  // the contributing inputs, by global ID
    std::unordered_map<std::string, SizeT> sampleSizes;
    InputPortConfigPtr freePort;
    std::string freePortPrefix;
    int nextPortId = 1;
};

END_NAMESPACE_REF_FB_MODULE
