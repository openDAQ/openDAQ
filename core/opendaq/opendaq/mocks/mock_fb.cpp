#include "opendaq/mock/mock_fb.h"
#include <opendaq/signal_factory.h>
#include <opendaq/data_rule_factory.h>
#include "opendaq/mock/mock_nested_fb_factory.h"
#include "opendaq/data_descriptor_factory.h"

using namespace daq;

MockFunctionBlockImpl::MockFunctionBlockImpl(
    daq::FunctionBlockTypePtr type,
    ContextPtr ctx,
    const ComponentPtr& parent,
    const StringPtr& localId)
    : FunctionBlockImpl<IFunctionBlock>(type, ctx, parent, localId)
{
    createFunctionBlocks();
    createSignals();
    createInputPorts();
    createReferencedSignals();

    this->tags.add("mock_fb");
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

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockFunctionBlock, daq::IFunctionBlock,
    daq::IFunctionBlockType*, type,
    daq::IContext*, ctx,
    daq::IComponent*, parent,
    daq::IString*, localId)
