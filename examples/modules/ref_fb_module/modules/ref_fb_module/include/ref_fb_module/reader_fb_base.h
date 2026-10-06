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
#include <opendaq/function_block_impl.h>
#include <opendaq/multi_reader2_factory.h>

#include <string>
#include <unordered_map>
#include <vector>

BEGIN_NAMESPACE_REF_FB_MODULE

// Shared handling for blocks built on MultiReader2. The reader is the only listener of the ports it reads: the
// block reads from onDataAvailable until drained, processes `count` first, under the descriptors cached from
// earlier statuses, and handles the status after. Descriptors are judged by `accepts`, which the reader calls
// inside read and configure; a block overrides onChanges when it has more to do than cache and wait. A block
// that always offers one free port keeps that port as its own listener: a connect adds the port to the reader
// and offers the next one.
class ReaderFbBase : public FunctionBlock
{
protected:
    ReaderFbBase(const FunctionBlockTypePtr& type, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);

    // Builds the reader from params, wires accepts into them and subscribes drain to onDataAvailable
    void createReader();

    // Reads until nothing is deliverable and nothing changed
    void drain();

    // Default: cache while valid or wait while invalid. Returns true when it reconfigured
    virtual bool onChanges(const MultiReader2StatusPtr& status);

    // Keeps `cached` at the descriptors of the contributing inputs and rebuilds the output descriptor from it
    void cacheDescriptors(const MultiReader2StatusPtr& status);

    // The owner's judgement of an input's value descriptor; runs inside the reader on the block's thread and
    // must not call the reader. Default: everything is accepted
    virtual bool accepts(const ComponentPtr& input, const DataDescriptorPtr& descriptor);

    // The free-port pattern: one port with the block as listener, outside the reader until a signal connects
    void addFreePort(const std::string& prefix);
    void onConnected(const InputPortPtr& port) override;
    ListPtr<IComponent> readerPorts() const;

    // `count` samples per slot sit in buffers as Float64; the buffer of a slot that did not contribute is null
    virtual void processAndSend(SizeT count, SizeT packetOffset) = 0;

    // Builds the output descriptor from `cached` and `outputDomain`; fills `outputError` on failure
    virtual void rebuildOutputDescriptor() = 0;

    // Component status from the reader's validity and the erroring inputs
    void reportStatus(const MultiReader2StatusPtr& status);

    void removed() override;

    MultiReader2ParamsPtr params;
    MultiReader2Ptr reader;
    std::vector<std::vector<double>> storage;
    std::vector<void*> buffers;
    DataDescriptorPtr outputDomain;
    std::string outputError;  // why the output descriptor could not be built; reported as a Warning
    std::unordered_map<std::string, DataDescriptorPtr> cached;   // value descriptors of the contributing inputs, by global id
    InputPortConfigPtr freePort;
    std::string freePortPrefix;
    int nextPortId = 1;
};

END_NAMESPACE_REF_FB_MODULE
