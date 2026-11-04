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
#include <coretypes/function_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/multi_reader2_factory.h>

#include <algorithm>
#include <string>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ_MODULE(reader_fb_base)

// A MultiReader2 block: reads under the acquisition lock, caches descriptors, reports the component status.
// `Interfaces` are the block's interfaces besides IFunctionBlock, such as IRecorder
template <typename... Interfaces>
class ReaderFbBase : public FunctionBlockImpl<IFunctionBlock, Interfaces...>
{
protected:
    using Base = FunctionBlockImpl<IFunctionBlock, Interfaces...>;

    ReaderFbBase(const FunctionBlockTypePtr& type, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Base(type, ctx, parent, localId)
        , params(MultiReader2Params())
    {
        this->initComponentStatus();
    }

    // The judgement and the handler hold the block weakly
    void createReader()
    {
        params.setAcceptsDescriptor(Function(
            [this, thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>()](ComponentPtr input, DataDescriptorPtr value, DataDescriptorPtr /*domain*/) -> bool
            {
                const auto thisFb = thisWeakRef.getRef();
                return !thisFb.assigned() || accepts(input, value);
            }));
        reader = MultiReader2(params);
        reader.getOnDataAvailable() +=
            [this, thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>()](BaseObjectPtr&, EventArgsPtr<>&)
        {
            const auto thisFb = thisWeakRef.getRef();
            if (thisFb.assigned())
                drain();
        };
        SizeT count = 0;
        SizeT offset = 0;
        handle(reader.read(nullptr, nullptr, &count, 0, offset));  // the first status
    }

    // Reads until nothing is readable and nothing changed
    void drain()
    {
        auto lock = this->getAcquisitionLock2();
        if (!reader.assigned())
            return;

        for (;;)
        {
            SizeT count = reader.getAvailableCount();
            storage.resize(descriptors.size());
            buffers.resize(descriptors.size());
            for (SizeT i = 0; i < descriptors.size(); i++)
            {
                buffers[i] = nullptr;
                if (!descriptors[i].assigned())
                    continue;
                // Rounded up, so a read type narrower than a double still fits
                storage[i].resize((count * sampleSizes[i] + sizeof(double) - 1) / sizeof(double));
                buffers[i] = storage[i].data();
            }

            SizeT offset = 0;
            const MultiReader2StatusPtr status = reader.read(count > 0 ? buffers.data() : nullptr, nullptr, &count, 0, offset);
            if (count > 0)
                processAndSend(count, offset);  // under the cached descriptors

            if (status.getHasChanges())
                handle(status);
            else if (count == 0)
                return;
        }
    }

    // Every status: removes left ports, lets the block react while `descriptors` is still the previous status, then caches,
    // rebuilds the outputs and reports, unless the block asked to judge its inputs again
    void handle(const MultiReader2StatusPtr& status)
    {
        if (freePort.assigned() && syncPorts())
            return;
        const bool rejudge = onChanges(status);
        if (status.getStatus() != ComponentStatus::Error)
        {
            cacheDescriptors(status);
            if (anyContributing())
                rebuildOutputDescriptor();  // with no input, the outputs keep their descriptors
        }
        if (rejudge)
            return handle(reader.reevaluate());  // its status replaces this one
        reportStatus(status);
    }

    // Returns true when the block's judgement changed
    virtual bool onChanges(const MultiReader2StatusPtr& /*status*/)
    {
        return false;
    }

    // Warning with no input, and for an Error whose inputs are only Disconnected
    void reportStatus(const MultiReader2StatusPtr& status)
    {
        if (params.getInputs().getCount() == 0)
            return this->setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");
        auto level = status.getStatus();
        if (level == ComponentStatus::Error)
        {
            level = ComponentStatus::Warning;
            for (const MultiReader2InputStatusPtr in : status.getInputs())
                if (in.getError() != MultiReader2InputError::None && in.getError() != MultiReader2InputError::Disconnected)
                    level = ComponentStatus::Error;
        }
        this->setComponentStatusWithMessage(level, status.getStatusMessage());
    }

    // A real or integer sample type without dimensions
    static bool scalarNumeric(const DataDescriptorPtr& descriptor)
    {
        if (!descriptor.assigned())
            return false;
        switch (descriptor.getSampleType())
        {
            case SampleType::Float32:
            case SampleType::Float64:
            case SampleType::Int8:
            case SampleType::UInt8:
            case SampleType::Int16:
            case SampleType::UInt16:
            case SampleType::Int32:
            case SampleType::UInt32:
            case SampleType::Int64:
            case SampleType::UInt64:
                break;
            default:
                return false;
        }
        const auto dimensions = descriptor.getDimensions();
        return !dimensions.assigned() || dimensions.getCount() == 0;
    }

    // Slot by slot; a status after a configure reports every healthy input afresh
    void cacheDescriptors(const MultiReader2StatusPtr& status)
    {
        if (status.getDomainDescriptorChanged())
            outputDomain = status.getDomainDescriptor();
        const auto inputs = status.getInputs();
        descriptors.resize(inputs.getCount());
        sampleSizes.resize(inputs.getCount());
        for (SizeT i = 0; i < inputs.getCount(); i++)
        {
            const MultiReader2InputStatusPtr in = inputs[i];
            if (in.getError() != MultiReader2InputError::None)
                descriptors[i] = nullptr;
            else if (in.getDescriptorChanged())
            {
                descriptors[i] = in.getDescriptor();
                sampleSizes[i] = in.getSampleSize();
            }
        }
    }

    bool anyContributing() const
    {
        return std::any_of(descriptors.begin(), descriptors.end(), [](const DataDescriptorPtr& d) { return d.assigned(); });
    }

    // Runs under the reader's lock: must not call the reader
    virtual bool accepts(const ComponentPtr& /*input*/, const DataDescriptorPtr& /*descriptor*/)
    {
        return true;
    }

    // One port outside the reader until a signal connects
    void addFreePort(const std::string& prefix)
    {
        freePortPrefix = prefix;
        freePort = this->createAndAddInputPort(fmt::format("{}_{}", prefix, nextPortId++), PacketReadyNotification::Scheduler);
    }

    // The free port joins the reader and the next free port is offered
    void onConnected(const InputPortPtr& port) override
    {
        auto lock = this->getAcquisitionLock2();
        if (reader.assigned() && freePort.assigned() && port == freePort)
        {
            addFreePort(freePortPrefix);
            syncPorts();
        }
    }

    // Called just before a left port is removed
    virtual void onPortRemoved(const InputPortConfigPtr& /*port*/)
    {
    }

    // `count` samples per contributing slot in `buffers`, in the read type (Float64 unless the block sets another)
    virtual void processAndSend(SizeT count, SizeT packetOffset) = 0;
    virtual void rebuildOutputDescriptor() = 0;

    void removed() override
    {
        reader.release();
        Base::removed();
    }

    MultiReader2ParamsPtr params;
    MultiReader2Ptr reader;
    std::vector<std::vector<double>> storage;
    std::vector<void*> buffers;
    DataDescriptorPtr outputDomain;
    std::vector<DataDescriptorPtr> descriptors;  // per slot of the last cached status; null while the input does not contribute
    std::vector<SizeT> sampleSizes;
    InputPortConfigPtr freePort;
    std::string freePortPrefix;
    int nextPortId = 1;

private:
    // The reader reads every port with a signal but the free one; a port whose signal left is removed after the reader
    // let go of it. Returns whether the inputs changed
    bool syncPorts()
    {
        auto inputs = List<IComponent>();
        std::vector<InputPortConfigPtr> left;
        for (const InputPortConfigPtr port : this->inputPorts.getItems())
        {
            if (port == freePort)
                continue;
            if (port.getSignal().assigned())
                inputs.pushBack(port);
            else
                left.push_back(port);
        }
        if (left.empty() && inputs.getCount() == params.getInputs().getCount())
            return false;
        params.setInputs(inputs);
        const auto next = reader.configure(params);
        for (const auto& port : left)
        {
            onPortRemoved(port);
            this->removeInputPort(port);
        }
        handle(next);
        return true;
    }
};

END_NAMESPACE_OPENDAQ_MODULE
