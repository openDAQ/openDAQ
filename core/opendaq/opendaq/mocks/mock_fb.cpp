#include "opendaq/mock/mock_fb.h"
#include <opendaq/signal_factory.h>
#include <opendaq/data_rule_factory.h>
#include "opendaq/mock/mock_nested_fb_factory.h"
#include "opendaq/data_descriptor_factory.h"
#include "coreobjects/property_object_factory.h"

using namespace daq;

MockFunctionBlockImpl::MockFunctionBlockImpl(daq::FunctionBlockTypePtr type,
                                             daq::ContextPtr ctx,
                                             const daq::ComponentPtr& parent,
                                             const daq::StringPtr& localId,
                                             const daq::PropertyObjectPtr& config)
    : FunctionBlockImpl<IFunctionBlock>(type, ctx, parent, localId, nullptr, config)
{
    this->tags.add("mock_fb");

    createFunctionBlocks();
    createSignals();
    createInputPorts();
    createReferencedSignals();
    createTestConfigProperties();
}

void MockFunctionBlockImpl::createFunctionBlocks()
{
    auto childFB = MockNestedFunctionBlock(
        FunctionBlockType("NestedFBId", "NestedFBName", "NestedFBDesc"), context, functionBlocks, "Nested function block");
    this->addNestedFunctionBlock(childFB);
}

void MockFunctionBlockImpl::createInputPorts()
{
    createAndAddInputPort("TestInputPort1", PacketReadyNotification::SameThread);
    createAndAddInputPort("TestInputPort2", PacketReadyNotification::SameThread);
}

void MockFunctionBlockImpl::createSignals()
{
    auto createDescriptor = [](std::string name) {

        return DataDescriptorBuilder()
            .setSampleType(SampleType::Float32)
            .setName(name)
            .setDimensions(daq::List<daq::IDimension>())
            .setRule(ConstantDataRule(1.0))
            .build();
    };

    createAndAddSignal("UniqueId_1", createDescriptor("Signal1"));
    createAndAddSignal("UniqueId_2", createDescriptor("Signal2"));
    createAndAddSignal("UniqueId_3", createDescriptor("Signal3"));
    createAndAddSignal("UniqueId_4", createDescriptor("Signal4"));
}

void MockFunctionBlockImpl::createReferencedSignals()
{
    auto thisPtr = this->borrowPtr<FunctionBlockPtr>();
    assert(thisPtr.getFunctionBlocks().getCount() > 0);
    assert(thisPtr.getFunctionBlocks()[0].getSignals().getCount() > 0);

    addSignal(thisPtr.getFunctionBlocks()[0].getSignals()[0]); // add first signal from nested function block
}

void MockFunctionBlockImpl::createTestConfigProperties()
{
    addProperty(IntProperty("TestConfigInt", 0));
    addProperty(StringProperty("TestConfigString", ""));

    if (config.hasProperty("TestConfigInt"))
        setPropertyValue((StringPtr) "TestConfigInt", config.getPropertyValue("TestConfigInt"));

    if (config.hasProperty("TestConfigString"))
        setPropertyValue((StringPtr) "TestConfigString", config.getPropertyValue("TestConfigString"));
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockFunctionBlock, daq::IFunctionBlock,
    daq::IFunctionBlockType*, type,
    daq::IContext*, ctx,
    daq::IComponent*, parent,
    daq::IString*, localId,
    daq::IPropertyObject*, config)
