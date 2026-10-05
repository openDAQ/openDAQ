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
#include <unordered_set>
#include <vector>

BEGIN_NAMESPACE_REF_FB_MODULE

// Shared handling for blocks built on MultiReader2. The reader is the only listener of the block's ports: the
// block implements no port callbacks and reads from onDataAvailable until drained. Every read processes `count`
// first, under the descriptors cached from earlier statuses, and handles the status after. A block overrides
// onChanges when it has more to do than reject, cache and wait, and accepts when it rejects descriptors.
class ReaderFbBase : public FunctionBlock
{
protected:
    ReaderFbBase(const FunctionBlockTypePtr& type, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);

    // Builds the reader from params and subscribes drain to onDataAvailable
    void createReader();

    // Reads until nothing is deliverable and nothing changed
    void drain();

    // Default: reject, then cache while valid or wait while invalid. Returns true when it reconfigured
    virtual bool onChanges(const MultiReader2StatusPtr& status);

    // Sets rejected inputs unused and accepted ones used; a pinned main input is left alone
    bool applyRejections(const MultiReader2StatusPtr& status);

    // Caches the descriptors marked new and rebuilds the output descriptor from the cache
    void cacheDescriptors(const MultiReader2StatusPtr& status);

    virtual bool accepts(const DataDescriptorPtr& descriptor);

    // `count` samples per slot sit in buffers as Float64; slots that did not contribute hold nothing
    virtual void processAndSend(SizeT count, SizeT packetOffset) = 0;

    // Builds the output descriptor from `cached` and `outputDomain`; fills `outputError` on failure
    virtual void rebuildOutputDescriptor() = 0;

    // Component status from the reader's validity and the owner's rejections
    void reportStatus(const MultiReader2StatusPtr& status);

    bool contributes(SizeT slot) const;
    void removed() override;

    MultiReader2ParamsPtr params;
    MultiReader2Ptr reader;
    std::vector<std::vector<double>> storage;
    std::vector<void*> buffers;
    std::vector<bool> active;                                    // per slot, as of the last status: used and healthy
    std::vector<ComponentPtr> slotInputs;                        // slot order as of the last status
    DataDescriptorPtr outputDomain;
    std::string outputError;  // why the output descriptor could not be built; reported as a Warning
    std::unordered_map<std::string, DataDescriptorPtr> cached;   // value descriptors by global id
    std::unordered_set<std::string> rejected;                    // inputs the owner set unused
};

END_NAMESPACE_REF_FB_MODULE
