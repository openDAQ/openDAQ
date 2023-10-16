#include "opendaq/mock/mock_nested_fb.h"
#include <opendaq/signal_factory.h>
#include <opendaq/data_rule_factory.h>

using namespace daq;

MockNestedFunctionBlockImpl::MockNestedFunctionBlockImpl(
    daq::FunctionBlockTypePtr type,
    ContextPtr ctx,
    const ComponentPtr& parent,
    const StringPtr& localId)
    : FunctionBlockImpl<IFunctionBlock>(type, ctx, parent, localId)
{
    createFunctionBlocks();
    createSignals();
    createInputPorts();
}

void MockNestedFunctionBlockImpl::createFunctionBlocks()
{
    auto childFB = daq::createWithImplementation<daq::IFunctionBlock, FunctionBlockImpl<>>(
        FunctionBlockType("NestedFBId", "NestedFBName", "NestedFBDesc"), context, functionBlocks, "Nested function block");
    this->addNestedFunctionBlock(childFB);
}

void MockNestedFunctionBlockImpl::createInputPorts()
{
    createAndAddInputPort("NestedTestInputPort1", PacketReadyNotification::SameThread);
}

void MockNestedFunctionBlockImpl::createSignals()
{
    auto createDescriptor = [](std::string name) {
        return DataDescriptorBuilder()
            .setSampleType(SampleType::Float32)
            .setName(name)
            .setDimensions(daq::ListPtr<daq::IDimension>())
            .setRule(ConstantDataRule(1.0))
            .build();
    };

    createAndAddSignal("NestedUniqueId_1", createDescriptor("NestedSignal1"));
    createAndAddSignal("NestedUniqueId_2", createDescriptor("NestedSignal2"));
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockNestedFunctionBlock, daq::IFunctionBlock,
    daq::IFunctionBlockType*, type,
    daq::IContext*, ctx,
    daq::IComponent*, parent,
    daq::IString*, localId)
