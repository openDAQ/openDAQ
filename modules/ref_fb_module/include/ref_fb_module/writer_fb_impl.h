/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/event_packet_ptr.h>
#include <deque>
#include <mutex>

BEGIN_NAMESPACE_REF_FB_MODULE
    
namespace WritterFb
{

template <typename T>
class SafeQueue
{
    void pushBack(const T& value)
    {
        std::scoped_lock lock(mx);
        deque.push_back(value);
    }

    T&& popFront()
    {
        std::scoped_lock lock(mx);
        auto result = deque.front();
        deque.pop_front();
        return std::move(result);
    }

    bool empty() const
    {
        std::scoped_lock lock(mx);
        return deque.empty();
    }

private:
    mutable std::mutex mx;
    std::deque<double> deque;
};

class WriterFbImpl final : public FunctionBlock
{
public:
    explicit WriterFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~WriterFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

private:

    DataDescriptorPtr outputDataDescriptor;
    DataDescriptorPtr outputDomainDataDescriptor;

    SignalConfigPtr outputSignal;
    SignalConfigPtr outputDomainSignal;

    void createInputPorts();
    void createSignals();

    void processEventPacket(const EventPacketPtr& packet);
    void onPacketReceived(const InputPortPtr& port) override;
    void configure();

    void initProperties();
    void propertyChanged(bool configure);
    void readProperties();

    SafeQueue<int64_t> queue;
};

}

END_NAMESPACE_REF_FB_MODULE
