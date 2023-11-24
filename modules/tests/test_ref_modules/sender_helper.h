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
#include <coreobjects/unit_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/signal_factory.h>
#include <testutils/testutils.h>
#include <opendaq/reader_utils.h>
#include <opendaq/instance_factory.h>

using namespace daq;

struct InputStub
{
    void setUp (
        SampleType signalType = SampleType::UInt64,
        RangePtr signalRange = Range(-10, 10),
        bool domainSync = true) 
    {
        this->domainSync = domainSync;
        this->instance = Instance();
        this->context = this->instance.getContext();
        this->scheduler = this->context.getScheduler();
        this->signal = createSignal(this->context, signalType, signalRange);
        this->domainSignal = createDomainSignal(this->context, domainSync);
        this->signal.setDomainSignal(this->domainSignal);
    }

    InstancePtr getInstance () 
    {
        return instance;
    }

    SignalConfigPtr getSignal ()
    {
        return signal;
    }

    void sendPacket (const PacketPtr& packet, bool wait = true) const
    {
        signal.sendPacket(packet);

        if (wait)
            scheduler.waitAll();
    }

    DataPacketPtr createDataPacket (SizeT numSamples, Int offset = 0) const
    {
        auto domainPacket = DataPacket(
            domainSignal.getDescriptor(),
            numSamples,
            offset
        );

        return DataPacketWithDomain (
            domainPacket,
            this->signal.getDescriptor(),
            numSamples
        );
    }

    static SignalConfigPtr createSignal (
        ContextPtr context, 
        SampleType type = SampleType::UInt64, 
        RangePtr range = Range(-10, 10))
    {
        auto descriptor = DataDescriptorBuilder()
                                .setName("stub")
                                .setSampleType(type)
                                .setValueRange(range)
                                .build();

        auto signal = Signal(context, nullptr, "sig");
        signal.setDescriptor(descriptor);

        return signal;
    }

    static SignalConfigPtr createDomainSignal (
        ContextPtr context, 
        bool domainSync,
        std::string epoch = "2023-11-24T00:02:03+00:00")
    {
        auto descriptor = DataDescriptorBuilder()
                                    .setName("domain stub")
                                    .setSampleType(SampleType::UInt64)
                                    .setOrigin(epoch)
                                    .setTickResolution(Ratio(1, 1000000))
                                    .setRule(domainSync ? LinearDataRule(1000, 0) : ExplicitDataRule())
                                    .setUnit(Unit("s", -1, "seconds", "time"))
                                    .build();

        auto domain = Signal(context, nullptr, "time");
        domain.setDescriptor(descriptor);

        return domain;
    }

protected:
    bool domainSync {true};
    InstancePtr instance;
    ContextPtr context;
    SchedulerPtr scheduler;
    SignalConfigPtr signal;
    SignalConfigPtr domainSignal;
};