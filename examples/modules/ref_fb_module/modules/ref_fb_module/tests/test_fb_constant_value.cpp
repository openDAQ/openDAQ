/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

#include <cmath>
#include <cstdint>
#include <limits>

#include <opendaq/context_factory.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>

#include <testutils/memcheck_listener.h>

using namespace daq;

using ConstantValueFbTest = testing::Test;

static const char* FB_TYPE_ID = "RefFBModuleConstantValue";

static ModulePtr createModule(const ContextPtr& context)
{
    ModulePtr module;
    createModule(&module, context);
    return module;
}

static FunctionBlockPtr createFb(const ModulePtr& module)
{
    return module.createFunctionBlock(FB_TYPE_ID, nullptr, "id");
}

// Connects a port to the function block output; the packets waiting afterwards are what a fresh listener sees.
static InputPortConfigPtr connectPort(const ContextPtr& ctx, const FunctionBlockPtr& fb)
{
    const auto port = InputPort(ctx, nullptr, "ip");
    port.connect(fb.getSignals()[0]);
    return port;
}

static ListPtr<IPacket> dequeue(const InputPortConfigPtr& port)
{
    return port.getConnection().dequeueAll();
}

static void assertDescriptorChangedEvent(const PacketPtr& packet)
{
    ASSERT_EQ(packet.getType(), PacketType::Event);
    ASSERT_EQ(packet.asPtr<IEventPacket>().getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
}

template <typename T>
static void assertConstantSample(const PacketPtr& packet, T expected)
{
    ASSERT_EQ(packet.getType(), PacketType::Data);

    const auto dataPacket = packet.asPtr<IDataPacket>();
    ASSERT_EQ(dataPacket.getSampleCount(), 1u);
    ASSERT_FALSE(dataPacket.getDomainPacket().assigned());
    ASSERT_EQ(*static_cast<T*>(dataPacket.getData()), expected);
}

TEST_F(ConstantValueFbTest, CreateConstantSignalWithoutDomain)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));

    // One port for a domain to drive the output. The domain signal it carries is reached through the
    // output signal rather than listed beside it.
    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);
    ASSERT_EQ(fb.getSignals().getCount(), 1u);
    ASSERT_EQ(fb.getSignals(search::Any()).getCount(), 2u);

    ASSERT_TRUE(fb.hasProperty("Value"));
    ASSERT_EQ(fb.getPropertyValue("Value"), "0");
    ASSERT_EQ(fb.getAllProperties().getCount(), 1u);

    const auto signal = fb.getSignals()[0];
    ASSERT_FALSE(signal.getDomainSignal().assigned());

    const auto descriptor = signal.getDescriptor();
    ASSERT_TRUE(descriptor.assigned());
    ASSERT_EQ(descriptor.getRule().getType(), DataRuleType::Constant);
    ASSERT_EQ(descriptor.getSampleType(), SampleType::Int32);
}

TEST_F(ConstantValueFbTest, ConnectReceivesCurrentValue)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));

    const auto packets = dequeue(connectPort(ctx, fb));
    ASSERT_EQ(packets.getCount(), 2u);
    assertDescriptorChangedEvent(packets[0]);
    assertConstantSample<int32_t>(packets[1], 0);
}

TEST_F(ConstantValueFbTest, ValueChangeSendsSinglePacket)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto port = connectPort(ctx, fb);
    dequeue(port);

    fb.setPropertyValue("Value", "5");

    const auto packets = dequeue(port);
    ASSERT_EQ(packets.getCount(), 1u);
    assertConstantSample<int32_t>(packets[0], 5);
}

TEST_F(ConstantValueFbTest, IdenticalValueWriteSendsNothing)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto port = connectPort(ctx, fb);

    fb.setPropertyValue("Value", "5");
    dequeue(port);

    fb.setPropertyValue("Value", "5");
    ASSERT_EQ(port.getConnection().getPacketCount(), 0u);
}

TEST_F(ConstantValueFbTest, SampleTypeChangeSendsDescriptorAndValue)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto port = connectPort(ctx, fb);

    fb.setPropertyValue("Value", "7");
    dequeue(port);

    // Written as a real number, the same quantity becomes a Float64 signal.
    fb.setPropertyValue("Value", "7.0");

    const auto packets = dequeue(port);
    ASSERT_EQ(packets.getCount(), 2u);
    assertDescriptorChangedEvent(packets[0]);
    assertConstantSample<double>(packets[1], 7.0);

    const auto descriptor = fb.getSignals()[0].getDescriptor();
    ASSERT_EQ(descriptor.getSampleType(), SampleType::Float64);
    ASSERT_EQ(descriptor.getRule().getType(), DataRuleType::Constant);
}

TEST_F(ConstantValueFbTest, SampleTypeChangeWithUnchangedValueSendsValue)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto port = connectPort(ctx, fb);
    dequeue(port);

    // The number is the same, but the sample it is carried in is not.
    fb.setPropertyValue("Value", "0.0");

    const auto packets = dequeue(port);
    ASSERT_EQ(packets.getCount(), 2u);
    assertDescriptorChangedEvent(packets[0]);
    assertConstantSample<double>(packets[1], 0.0);
}

TEST_F(ConstantValueFbTest, LateConnectReceivesLatestValue)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));

    fb.setPropertyValue("Value", "1");
    fb.setPropertyValue("Value", "2");
    fb.setPropertyValue("Value", "3");

    const auto packets = dequeue(connectPort(ctx, fb));
    ASSERT_EQ(packets.getCount(), 2u);
    assertDescriptorChangedEvent(packets[0]);
    assertConstantSample<int32_t>(packets[1], 3);
}

TEST_F(ConstantValueFbTest, VectorValueEmitsEveryElement)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto port = connectPort(ctx, fb);
    dequeue(port);

    fb.setPropertyValue("Value", "1;2;3");

    const auto packets = dequeue(port);
    ASSERT_EQ(packets.getCount(), 2u);
    assertDescriptorChangedEvent(packets[0]);

    const auto dataPacket = packets[1].asPtr<IDataPacket>();
    ASSERT_EQ(dataPacket.getSampleCount(), 1u);

    const auto* data = static_cast<const int32_t*>(dataPacket.getData());
    ASSERT_EQ(data[0], 1);
    ASSERT_EQ(data[1], 2);
    ASSERT_EQ(data[2], 3);
}

TEST_F(ConstantValueFbTest, VectorValueSetsDimensions)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto signal = fb.getSignals()[0];

    ASSERT_EQ(signal.getDescriptor().getDimensions().getCount(), 0u);

    fb.setPropertyValue("Value", "1;2;3;4");

    const auto descriptor = signal.getDescriptor();
    ASSERT_EQ(descriptor.getDimensions().getCount(), 1u);
    ASSERT_EQ(descriptor.getDimensions()[0].getSize(), 4u);
    ASSERT_EQ(descriptor.getSampleSize(), 4u * sizeof(int32_t));
    ASSERT_EQ(descriptor.getRule().getType(), DataRuleType::Constant);

    // Back to a single element leaves a scalar descriptor behind, not a vector of one.
    fb.setPropertyValue("Value", "5");
    ASSERT_EQ(signal.getDescriptor().getDimensions().getCount(), 0u);
}

TEST_F(ConstantValueFbTest, VectorValueLastValueIsAList)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));

    fb.setPropertyValue("Value", "1.5; 2.5; 3.5");

    const ListPtr<IBaseObject> lastValue = fb.getSignals()[0].getLastValue();
    ASSERT_EQ(lastValue.getCount(), 3u);
    ASSERT_EQ(lastValue[0], 1.5);
    ASSERT_EQ(lastValue[2], 3.5);
}

TEST_F(ConstantValueFbTest, VectorSizeChangeSendsDescriptorAndValue)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto port = connectPort(ctx, fb);

    fb.setPropertyValue("Value", "1;2");
    dequeue(port);

    fb.setPropertyValue("Value", "1;2;3");

    const auto packets = dequeue(port);
    ASSERT_EQ(packets.getCount(), 2u);
    assertDescriptorChangedEvent(packets[0]);
    ASSERT_EQ(packets[1].asPtr<IDataPacket>().getRawDataSize(), 3u * sizeof(int32_t));
}

TEST_F(ConstantValueFbTest, IdenticalVectorWriteSendsNothing)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto port = connectPort(ctx, fb);

    fb.setPropertyValue("Value", "1;2;3");
    dequeue(port);

    // Same values, different spelling.
    fb.setPropertyValue("Value", " 1 ; 2 ; 3 ");
    ASSERT_EQ(port.getConnection().getPacketCount(), 0u);
}

TEST_F(ConstantValueFbTest, MalformedValueIsRejected)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto port = connectPort(ctx, fb);

    fb.setPropertyValue("Value", "5");
    dequeue(port);

    ASSERT_ANY_THROW(fb.setPropertyValue("Value", "1;abc"));
    ASSERT_ANY_THROW(fb.setPropertyValue("Value", "1;;2"));
    ASSERT_ANY_THROW(fb.setPropertyValue("Value", "1;2;"));
    ASSERT_ANY_THROW(fb.setPropertyValue("Value", ""));
    ASSERT_ANY_THROW(fb.setPropertyValue("Value", "1,5"));

    // The signal keeps the value that was last accepted, and nothing was emitted.
    ASSERT_EQ(fb.getPropertyValue("Value"), "5");
    ASSERT_EQ(port.getConnection().getPacketCount(), 0u);
}

TEST_F(ConstantValueFbTest, SampleTypeIsInferredFromValue)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto signal = fb.getSignals()[0];

    const auto sampleTypeOf = [&fb, &signal](const std::string& value)
    {
        fb.setPropertyValue("Value", value);
        return signal.getDescriptor().getSampleType();
    };

    ASSERT_EQ(sampleTypeOf("5"), SampleType::Int32);
    ASSERT_EQ(sampleTypeOf("-5; 7"), SampleType::Int32);
    ASSERT_EQ(sampleTypeOf("+5"), SampleType::Int32);

    // Outside the range of an Int32, so the whole vector widens.
    ASSERT_EQ(sampleTypeOf("3000000000"), SampleType::Int64);
    ASSERT_EQ(sampleTypeOf("1; 3000000000"), SampleType::Int64);

    // Written as a real number, or beyond what an integer can hold.
    ASSERT_EQ(sampleTypeOf("5.0"), SampleType::Float64);
    ASSERT_EQ(sampleTypeOf("1e3"), SampleType::Float64);
    ASSERT_EQ(sampleTypeOf("1;2.5;3"), SampleType::Float64);
    ASSERT_EQ(sampleTypeOf("99999999999999999999"), SampleType::Float64);
}

TEST_F(ConstantValueFbTest, LargeIntegerValueIsExact)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto port = connectPort(ctx, fb);
    dequeue(port);

    // Beyond 2^53, so a detour through a double would lose the last digits.
    fb.setPropertyValue("Value", "9223372036854775807");

    const auto packets = dequeue(port);
    ASSERT_EQ(packets.getCount(), 2u);
    assertDescriptorChangedEvent(packets[0]);
    assertConstantSample<int64_t>(packets[1], 9223372036854775807LL);
}

TEST_F(ConstantValueFbTest, NotFiniteValueIsRejected)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));

    fb.setPropertyValue("Value", "5");

    ASSERT_ANY_THROW(fb.setPropertyValue("Value", "nan"));
    ASSERT_ANY_THROW(fb.setPropertyValue("Value", "inf"));
    ASSERT_ANY_THROW(fb.setPropertyValue("Value", "-inf"));
    ASSERT_ANY_THROW(fb.setPropertyValue("Value", "1;nan"));
    ASSERT_ANY_THROW(fb.setPropertyValue("Value", "0x10"));

    ASSERT_EQ(fb.getPropertyValue("Value"), "5");
    ASSERT_EQ(fb.getSignals()[0].getDescriptor().getSampleType(), SampleType::Int32);
}

// Domain mode: a signal connected to the input port drives the output

static SignalConfigPtr createDomainSignal(const ContextPtr& ctx)
{
    const auto signal = Signal(ctx, nullptr, "time");
    signal.setDescriptor(DataDescriptorBuilder()
                             .setSampleType(SampleType::Int64)
                             .setRule(LinearDataRule(1, 0))
                             .setTickResolution(Ratio(1, 1000))
                             .setUnit(Unit("s", -1, "seconds", "time"))
                             .build());
    return signal;
}

TEST_F(ConstantValueFbTest, DomainConnectAttachesDomainSignal)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto signal = fb.getSignals()[0];

    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);
    ASSERT_FALSE(signal.getDomainSignal().assigned());

    const auto domainSignal = createDomainSignal(ctx);
    fb.getInputPorts()[0].connect(domainSignal);

    ASSERT_TRUE(signal.getDomainSignal().assigned());
    ASSERT_EQ(signal.getDomainSignal().getDescriptor().getRule().getType(), DataRuleType::Linear);
}

TEST_F(ConstantValueFbTest, DomainPacketDrivesOutput)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto domainSignal = createDomainSignal(ctx);

    fb.setPropertyValue("Value", "7");
    fb.getInputPorts()[0].connect(domainSignal);

    const auto port = connectPort(ctx, fb);
    dequeue(port);

    domainSignal.sendPacket(DataPacket(domainSignal.getDescriptor(), 100, 0));

    const auto packets = dequeue(port);
    ASSERT_EQ(packets.getCount(), 1u);

    const auto dataPacket = packets[0].asPtr<IDataPacket>();
    ASSERT_EQ(dataPacket.getSampleCount(), 100u);
    ASSERT_TRUE(dataPacket.getDomainPacket().assigned());
    ASSERT_EQ(dataPacket.getDomainPacket().getSampleCount(), 100u);

    // The constant expands across the whole span the domain packet covers.
    const auto* data = static_cast<const int32_t*>(dataPacket.getData());
    ASSERT_EQ(data[0], 7);
    ASSERT_EQ(data[99], 7);
}

TEST_F(ConstantValueFbTest, ValueChangeTakesEffectOnNextDomainPacket)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto domainSignal = createDomainSignal(ctx);

    fb.setPropertyValue("Value", "1");
    fb.getInputPorts()[0].connect(domainSignal);

    const auto port = connectPort(ctx, fb);
    dequeue(port);

    // Writing the value emits nothing by itself while a domain drives the output.
    fb.setPropertyValue("Value", "2");
    ASSERT_EQ(port.getConnection().getPacketCount(), 0u);

    domainSignal.sendPacket(DataPacket(domainSignal.getDescriptor(), 10, 0));

    const auto packets = dequeue(port);
    ASSERT_EQ(packets.getCount(), 1u);
    ASSERT_EQ(*static_cast<const int32_t*>(packets[0].asPtr<IDataPacket>().getData()), 2);
}

TEST_F(ConstantValueFbTest, ValueSignalOnInputPortUsesItsDomain)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));

    const auto domainSignal = createDomainSignal(ctx);
    const auto valueSignal = Signal(ctx, nullptr, "value");
    valueSignal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setRule(ExplicitDataRule()).build());
    valueSignal.setDomainSignal(domainSignal);

    fb.setPropertyValue("Value", "3");
    fb.getInputPorts()[0].connect(valueSignal);

    const auto port = connectPort(ctx, fb);
    dequeue(port);

    const auto domainPacket = DataPacket(domainSignal.getDescriptor(), 5, 0);
    valueSignal.sendPacket(DataPacketWithDomain(domainPacket, valueSignal.getDescriptor(), 5));

    const auto packets = dequeue(port);
    ASSERT_EQ(packets.getCount(), 1u);

    const auto dataPacket = packets[0].asPtr<IDataPacket>();
    ASSERT_EQ(dataPacket.getSampleCount(), 5u);
    ASSERT_EQ(dataPacket.getDomainPacket(), domainPacket);
}

TEST_F(ConstantValueFbTest, DomainDisconnectReturnsToReplay)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto domainSignal = createDomainSignal(ctx);
    const auto signal = fb.getSignals()[0];

    fb.setPropertyValue("Value", "9");
    fb.getInputPorts()[0].connect(domainSignal);
    ASSERT_TRUE(signal.getDomainSignal().assigned());

    fb.getInputPorts()[0].disconnect();
    ASSERT_FALSE(signal.getDomainSignal().assigned());

    // The value is the block's own again, so a port connecting now is handed it.
    const auto packets = dequeue(connectPort(ctx, fb));
    ASSERT_EQ(packets.getCount(), 2u);
    assertDescriptorChangedEvent(packets[0]);
    assertConstantSample<int32_t>(packets[1], 9);
}

TEST_F(ConstantValueFbTest, NoReplayWhileDomainDrivesOutput)
{
    const auto ctx = NullContext();
    const auto fb = createFb(createModule(ctx));
    const auto domainSignal = createDomainSignal(ctx);

    fb.setPropertyValue("Value", "4");
    fb.getInputPorts()[0].connect(domainSignal);

    // Replay on connect is for signals without a domain; here the next domain packet carries the value.
    const auto packets = dequeue(connectPort(ctx, fb));
    ASSERT_EQ(packets.getCount(), 1u);
    assertDescriptorChangedEvent(packets[0]);
}
