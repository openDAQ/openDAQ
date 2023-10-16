#include <opendaq/packet_factory.h>
#include <opendaq/event_packet.h>
#include "opendaq/mock/mock_channel.h"
#include <opendaq/signal_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/data_rule_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/scaling_factory.h>

#define _USE_MATH_DEFINES
#include <math.h>

BEGIN_NAMESPACE_OPENDAQ

MockChannelImpl::MockChannelImpl(ContextPtr ctx, const ComponentPtr& parent, const StringPtr& localId)
    : ChannelImpl<IMockChannel>(FunctionBlockType("mock_ch", "mock_ch", ""), std::move(ctx), parent, localId)
{
    createSignals();
    createInputPorts();
}

void MockChannelImpl::createInputPorts()
{
    createAndAddInputPort("TestInputPort1", PacketReadyNotification::SameThread);
    createAndAddInputPort("TestInputPort2", PacketReadyNotification::SameThread);
}

void MockChannelImpl::createSignals()
{
    auto createDescriptor = [](std::string name)
    {
        return DataDescriptorBuilder()
            .setSampleType(SampleType::Float32)
            .setName(name)
            .setDimensions(daq::ListPtr<daq::IDimension>())
            .setRule(ConstantDataRule(1.0))
            .build();
    };

    createAndAddSignal("UniqueId_1", createDescriptor("Signal1"));
    createAndAddSignal("UniqueId_2", createDescriptor("Signal2"));
    createAndAddSignal("UniqueId_3", createDescriptor("Signal3"));
    createAndAddSignal("UniqueId_4", createDescriptor("Signal4"));

    addTimeSignal();
    addChangingTimeSignal();
    addByteStepSignal();
    addIntStepSignal();
    addSineSignal();
    addChangingSignal();
}

void MockChannelImpl::generateSamplesUntil(std::chrono::milliseconds currentTime)
{
    for (const auto& signal : generatedSignals)
        signal->generateSamplesTo(currentTime);
}

uint64_t MockChannelImpl::getOutputRate()
{
    return 1000;
}

void MockChannelImpl::addTimeSignal()
{
    const size_t nanosecondsInSecond = 1000000000;
    auto delta = nanosecondsInSecond / getOutputRate();

    auto dataDescriptor = DataDescriptorBuilder()
                          .setSampleType(SampleType::UInt64)
                          .setRule(LinearDataRule(delta, 100))
                          .setTickResolution(Ratio(1, nanosecondsInSecond))
                          .setOrigin("1970-01-01T00:00:00")
                          .setName("Time")
                          .build();

    timeSignal = createAndAddSignal(dataDescriptor.getName(), dataDescriptor);
}

void MockChannelImpl::addByteStepSignal()
{
    auto meta = Dict<IString, IString>();
    meta["color"] = "red";
    meta["used"] = "1";

    auto dataDescriptor = DataDescriptorBuilder()
                              .setSampleType(SampleType::Int8)
                              .setUnit(Unit("V", 1, "voltage", "quantity"))
                              .setValueRange(Range(0, 10))
                              .setRule(ExplicitDataRule())
                              .setOrigin("1970-01-01T00:00:00")
                              .setName("ByteStep")
                              .setMetadata(meta)
                              .build();

    auto signal = createAndAddSignal(dataDescriptor.getName(), dataDescriptor);
    
    signal.setDomainSignal(timeSignal);

    auto generatedSignal = std::make_shared<SignalGenerator>(signal);

    generatedSignal->setFunction([](uint64_t tick, void* sampleOut) {
        uint8_t* byteOut = (uint8_t*) sampleOut;
        *byteOut = tick % 10;
    });

    generatedSignals.push_back(generatedSignal);
}

void MockChannelImpl::addIntStepSignal()
{
    auto dataDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Int32)
                                                 .setUnit(Unit("V", 1, "voltage", "quantity"))
                                                 .setValueRange(Range(0, 10))
                                                 .setRule(ExplicitDataRule())
                                                 .setOrigin("1970-01-01T00:00:00")
                                                 .setName("IntStep")
                                                 .build();

    auto signal = createAndAddSignal(dataDescriptor.getName(), dataDescriptor);
    signal.setDomainSignal(timeSignal);

    auto generatedSignal = std::make_shared<SignalGenerator>(signal);

    generatedSignal->setFunction([this](uint64_t /*tick*/, void* sampleOut) {
        int32_t* intOut = (int32_t*) sampleOut;
        *intOut = intStepValue;
        intStepValue++;
        if (intStepValue > 10)
            intStepValue = -10;
    });

    generatedSignals.push_back(generatedSignal);
}

void MockChannelImpl::addSineSignal()
{
    auto dataDescriptor = DataDescriptorBuilder()
                              .setSampleType(SampleType::Float64)
                              .setUnit(Unit("V", 1, "voltage", "quantity"))
                              .setValueRange(Range(0, 10))
                              .setPostScaling(LinearScaling(2, 1))
                              .setName("Sine")
                              .build();

    auto signal = createAndAddSignal(dataDescriptor.getName(), dataDescriptor);
    signal.setDomainSignal(timeSignal);

    auto generatedSignal = std::make_shared<SignalGenerator>(signal);

    generatedSignal->setFunction([this](uint64_t tick, void* sampleOut) {
        auto outputRate = getOutputRate();
        const double frequency = 10;
        double* doubleOut = (double*) sampleOut;
        *doubleOut = 1.0 * std::sin(tick * frequency / outputRate * M_PI * 2.0);
    });

    generatedSignals.push_back(generatedSignal);
}

void MockChannelImpl::addChangingSignal()
{
    auto dataDescriptor = DataDescriptorBuilder()
                              .setSampleType(SampleType::Float64)
                              .setUnit(Unit("V", 1, "voltage", "quantity"))
                              .setValueRange(Range(0, 10))
                              .setRule(ExplicitDataRule())
                              .setOrigin("1970-01-01T00:00:00")
                              .setName("ChangingSignal")
                              .build();

    auto signal = createAndAddSignal(dataDescriptor.getName(), dataDescriptor);
    signal.setDomainSignal(changingTimeSignal);
    auto generatedSignal = std::make_shared<SignalGenerator>(signal);

    auto stepFunction10 = [this](uint64_t tick, void* sampleOut) {
        double* intOut = (double*) sampleOut;
        *intOut = tick % 10;
    };

    auto stepFunction100 = [this](uint64_t tick, void* sampleOut)
    {
        double* intOut = (double*) sampleOut;
        *intOut = tick % 100;
    };

    generatedSignal->setFunction(stepFunction10);
    
    generatedSignal->setUpdateFunction([this, stepFunction100](SignalGenerator& generator, uint64_t packetOffset) {
        if (packetOffset == 0)
            return;

        SignalConfigPtr signal = generator.getSignal();
        SignalConfigPtr domainSignal = signal.getDomainSignal();

        auto meta = Dict<IString, IString>();
        meta["color"] = "red";

        auto metaDomain = Dict<IString, IString>();
        metaDomain["color"] = "green";

        auto descriptor = DataDescriptorBuilderCopy(signal.getDescriptor())
                              .setName("Changed signal " + std::to_string(packetOffset))
                              .setMetadata(meta)
                              .setUnit(Unit("mV", 10, "voltage", "quantity"))
                              .setValueRange(Range(0, 100))
                              .setOrigin("")
                              .setPostScaling(LinearScaling(1.0, 0.0))
                              .build();

        signal.setDescriptor(descriptor);

        auto domainDescriptor = DataDescriptorBuilderCopy(domainSignal.getDescriptor())
                                    .setName("Changed domain signal " + std::to_string(packetOffset))
                                    .setMetadata(metaDomain)
                                    .setRule(LinearDataRule(1000 + packetOffset, 10000))
                                    .setUnit(Unit("s", 10, "seconds", "time"))
                                    .setValueRange(Range(0, 10000000))
                                    .setOrigin("1980-01-01T00:00:00")
                                    .build();

        domainSignal.setDescriptor(domainDescriptor);

        generator.setFunction(stepFunction100);
    });
    
    generatedSignals.push_back(generatedSignal);
}

void MockChannelImpl::addChangingTimeSignal()
{
    const size_t nanosecondsInSecond = 1000000000;
    auto delta = nanosecondsInSecond / getOutputRate();

    auto dataDescriptor = DataDescriptorBuilder()
                              .setSampleType(SampleType::UInt64)
                              .setRule(LinearDataRule(delta, 100))
                              .setTickResolution(Ratio(1, nanosecondsInSecond))
                              .setOrigin("1970-01-01T00:00:00")
                              .setName("ChangingTime")
                              .build();

    changingTimeSignal = createAndAddSignal(dataDescriptor.getName(), dataDescriptor);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockChannel, IChannel,
    IContext*, ctx,
    IComponent*, parent,
    IString*, localId
)

END_NAMESPACE_OPENDAQ
