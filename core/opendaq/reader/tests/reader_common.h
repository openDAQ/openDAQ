/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/logger_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/signal_factory.h>
#include <testutils/testutils.h>
#include <opendaq/reader_utils.h>
#include <opendaq/logger_sink_last_message_private_ptr.h>

#include <thread>

using namespace daq;

#if !defined(SKIP_TEST_MAC_CI)
    #if defined(__clang__) && !defined(__RESHARPER__)
        #define SKIP_TEST_MAC_CI return
    #else
        #define SKIP_TEST_MAC_CI
    #endif   
#endif

daq::DataDescriptorPtr setupDescriptor(daq::SampleType type,
                                       daq::DataRulePtr rule = nullptr,
                                       daq::ScalingPtr scaling = nullptr);
daq::DataDescriptorBuilderPtr setupConfigurableDescriptor(daq::SampleType type,
                                                          daq::DataRulePtr rule = nullptr,
                                                          daq::ScalingPtr scaling = nullptr,
                                                          daq::StringPtr referenceDomainId = nullptr,
                                                          daq::BooleanPtr referenceDomainIsAbsolute = nullptr);

template <typename T = void>
class ReaderTest : public testing::Test
{
protected:
    void SetUp() override
    {
        auto debugSink = LastMessageLoggerSink();
        debugSink.setLevel(LogLevel::Info);
        auto sinks = DefaultSinks(nullptr);
        sinks.pushBack(debugSink);
        privateSink = debugSink;
        logger =  LoggerWithSinks(sinks);
        context = daq::Context(daq::Scheduler(logger, 1), logger, nullptr, nullptr, nullptr);
        scheduler = context.getScheduler();
        signal = daq::Signal(context, nullptr, "sig");
    }

public:

    void sendPacket(const daq::PacketPtr& packet, bool wait = true) const
    {
        signal.sendPacket(packet);

        if (wait)
            scheduler.waitAll();
    }

    void sendPacketToSignal(const daq::SignalConfigPtr& receiver, const daq::PacketPtr& packet, bool wait = true) const
    {
        receiver.sendPacket(packet);

        if (wait)
            scheduler.waitAll();
    }

    daq::DataPacketPtr createDataPacket(daq::SizeT numSamples, daq::Int offset, daq::SignalPtr signal = nullptr) const
    {
        auto domainPacket = daq::DataPacket(
            setupDescriptor(daq::SampleType::UInt64, daq::LinearDataRule(1, 0), nullptr),
            numSamples,
            offset
        );

        if (!signal.assigned())
        {
            signal = this->signal;
        }

        return daq::DataPacketWithDomain(
            domainPacket,
            this->signal.getDescriptor(),
            numSamples
        );
    }

    auto createDomainDescriptor(std::string epoch = "",
                                daq::RatioPtr resolution = nullptr,
                                daq::DataRulePtr rule = nullptr,
                                daq::StringPtr referenceDomainId = nullptr,
                                daq::BooleanPtr referenceDomainIsAbsolute = nullptr) const
    {
        if (epoch.empty())
        {
            epoch = "2022-09-27T00:02:03+00:00";
        }

        if (!resolution.assigned())
        {
            resolution = daq::Ratio(1, 1000);
        }

        if (!rule.assigned())
        {
            rule = daq::LinearDataRule(1, 0);
        }

        return setupConfigurableDescriptor(daq::SampleTypeFromType<daq::ClockTick>::SampleType, rule, nullptr, referenceDomainId, referenceDomainIsAbsolute)
            .setOrigin(epoch)
            .setTickResolution(resolution)
            .setUnit(daq::Unit("s", -1, "seconds", "time"))
            .build();
    }

    void TearDown() override
    {
        using namespace std::chrono_literals;

        scheduler.stop();
        std::this_thread::sleep_for(10ms);
    }

protected:
    daq::LoggerPtr logger;
    daq::ContextPtr context;
    daq::SchedulerPtr scheduler;
    daq::SignalConfigPtr signal;
    daq::LastMessageLoggerSinkPrivatePtr privateSink;
};

[[nodiscard]]
inline daq::DataDescriptorBuilderPtr setupConfigurableDescriptor(daq::SampleType type,
                                                                 daq::DataRulePtr rule,
                                                                 daq::ScalingPtr scaling,
                                                                 daq::StringPtr referenceDomainId,
                                                                 daq::BooleanPtr referenceDomainIsAbsolute)
{
    auto dataDescriptor = daq::DataDescriptorBuilder().setSampleType(type).setPostScaling(scaling);

    if (rule.assigned())
        dataDescriptor.setRule(rule);

    if (referenceDomainId.assigned() || referenceDomainIsAbsolute.assigned())
        dataDescriptor.setReferenceDomainInfo(ReferenceDomainInfoBuilder()
                                                  .setReferenceDomainId(referenceDomainId)
                                                  .setReferenceDomainIsAbsolute(referenceDomainIsAbsolute)
                                                  .build());

    return dataDescriptor;
}

[[nodiscard]]
inline daq::DataDescriptorPtr setupDescriptor(daq::SampleType type,
                                              daq::DataRulePtr rule,
                                              daq::ScalingPtr scaling)
{
    return setupConfigurableDescriptor(type, rule, scaling).build();
}

namespace daq
{
    // ReSharper disable once CppInconsistentNaming
    inline void PrintTo(daq::SampleType sampleType, std::ostream* os)
    {
        // GTest pretty-print for SampleType enum
        *os << convertSampleTypeToString(sampleType);
    }
}

namespace std::chrono  // NOLINT(cert-dcl58-cpp)
{
    inline void PrintTo(std::chrono::system_clock::time_point tp, std::ostream* os)
    {
        *os << '"';
        daq::reader::operator<<(*os, tp);
        *os << '"';
    }

    inline void PrintTo(std::chrono::milliseconds ms, std::ostream* os)
    {
        *os << ms.count() << "ms";
    }

    inline void PrintTo(std::chrono::nanoseconds ns, std::ostream* os)
    {
        *os << ns.count() << "ns";
    }
}
