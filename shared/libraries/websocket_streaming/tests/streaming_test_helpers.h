/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

#include <opendaq/instance_factory.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/mock/mock_physical_device.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/sample_type_traits.h>
#include "streaming_protocol/Unit.hpp"

namespace streaming_test_helpers
{
    inline daq::InstancePtr createServerInstance()
    {
        const auto moduleManager = daq::ModuleManager("[[none]]");
        auto context = Context(nullptr, daq::Logger(), daq::TypeManager(), moduleManager);
        const daq::ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        const daq::ModulePtr fbModule(MockFunctionBlockModule_Create(context));
        moduleManager.addModule(fbModule);

        auto instance = InstanceCustom(context, "localInstance");
        instance.addDevice("daq_client_device");
        instance.addDevice("mock_phys_device");
        instance.addFunctionBlock("mock_fb_uid");

        return instance;
    }

    inline daq::SignalPtr createLinearTimeSignal(const daq::ContextPtr& ctx)
    {
        const size_t nanosecondsInSecond = 1000000000;
        auto delta = nanosecondsInSecond / 1000;

        auto descriptor = daq::DataDescriptorBuilder()
                              .setSampleType(daq::SampleType::UInt64)
                              .setRule(daq::LinearDataRule(delta, 100))
                              .setTickResolution(daq::Ratio(1, nanosecondsInSecond))
                              .setOrigin(daq::streaming_protocol::UNIX_EPOCH_DATE_UTC_TIME)
                              .setUnit(daq::Unit("s",
                                                 daq::streaming_protocol::Unit::UNIT_ID_SECONDS,
                                                 "seconds",
                                                 "time"))
                              .setName("Time")
                              .build();

        auto signal = SignalWithDescriptor(ctx, descriptor, nullptr, descriptor.getName());

        signal.asPtr<daq::IPropertyObjectInternal>().enableCoreEventTrigger();
        return signal;
    }

    inline daq::SignalPtr createExplicitValueSignal(const daq::ContextPtr& ctx,
                                                    const daq::StringPtr& name,
                                                    const daq::SignalPtr& domainSignal)
    {
        auto meta = daq::Dict<daq::IString, daq::IString>();
        meta["color"] = "green";
        meta["used"] = "0";

        auto descriptor = daq::DataDescriptorBuilder()
                              .setSampleType(daq::SampleType::Float64)
                              .setUnit(daq::Unit("V", 1, "voltage", "quantity"))
                              .setValueRange(daq::Range(0, 10))
                              .setRule(daq::ExplicitDataRule())
                              .setPostScaling(daq::LinearScaling(1.0, 0.0, daq::SampleType::Int16, daq::ScaledSampleType::Float64))
                              .setName(name)
                              .setMetadata(meta)
                              .build();

        auto timeSignal = createLinearTimeSignal(ctx);
        auto signal = SignalWithDescriptor(ctx, descriptor, nullptr, descriptor.getName());
        signal.setDomainSignal(domainSignal);
        signal.setName(name);
        signal.setDescription("TestDescription");

        signal.asPtr<daq::IPropertyObjectInternal>().enableCoreEventTrigger();
        return signal;
    }

    inline daq::SignalPtr createConstantValueSignal(const daq::ContextPtr& ctx,
                                                    const daq::StringPtr& name,
                                                    const daq::SignalPtr& domainSignal)
    {
        auto descriptor = daq::DataDescriptorBuilder()
                              .setSampleType(daq::SampleType::UInt64)
                              .setValueRange(daq::Range(0, 10))
                              .setRule(daq::ConstantDataRule())
                              .setName(name)
                              .build();

        auto timeSignal = createLinearTimeSignal(ctx);
        auto signal = SignalWithDescriptor(ctx, descriptor, nullptr, descriptor.getName());
        signal.setDomainSignal(timeSignal);
        signal.setName(name);
        signal.setDescription("TestDescription");

        signal.asPtr<daq::IPropertyObjectInternal>().enableCoreEventTrigger();
        return signal;
    }
}
