#include <opendaq/mock/mock_fb_dynamic_input_ports.h>
#include <opendaq/signal_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/mock/mock_nested_fb_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <coreobjects/property_object_factory.h>

using namespace daq;

MockFunctionBlockDynamicInputPortImpl::MockFunctionBlockDynamicInputPortImpl(daq::FunctionBlockTypePtr type,
                                             daq::ContextPtr ctx,
                                             const daq::ComponentPtr& parent,
                                             const daq::StringPtr& localId,
                                             const daq::PropertyObjectPtr& /* config */)
    : FunctionBlockImpl<IFunctionBlock>(type, ctx, parent, localId)
{
    this->tags.add("mock_fb_dynamic_input_ports");
    updateInputPorts();
    createAndAddSignal("OutputSignal");
}

daq::FunctionBlockTypePtr MockFunctionBlockDynamicInputPortImpl::CreateType()
{
    return FunctionBlockType("mock_fb_dynamic_input_ports_uid", "mock_fb_dynamic_input_ports", "", daq::PropertyObject());
}

void MockFunctionBlockDynamicInputPortImpl::onConnected(const daq::InputPortPtr& /* port */)
{
    std::scoped_lock lock(sync);
    updateInputPorts();
}

void MockFunctionBlockDynamicInputPortImpl::onDisconnected(const daq::InputPortPtr& port)
{
    std::scoped_lock lock(sync);
    removeInputPort(port);
}

void MockFunctionBlockDynamicInputPortImpl::updateInputPorts()
{
    createAndAddInputPort(fmt::format("InputPort{}", inputPortCount++), PacketReadyNotification::SameThread);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockFunctionBlockDynamicInputPort, daq::IFunctionBlock,
    daq::IFunctionBlockType*, type,
    daq::IContext*, ctx,
    daq::IComponent*, parent,
    daq::IString*, localId,
    daq::IPropertyObject*, config)
