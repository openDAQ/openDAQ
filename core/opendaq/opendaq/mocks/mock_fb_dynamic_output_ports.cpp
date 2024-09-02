#include <opendaq/mock/mock_fb_dynamic_output_ports.h>
#include <opendaq/signal_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/mock/mock_nested_fb_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <coreobjects/property_object_factory.h>

using namespace daq;

MockFunctionBlockDynamicOutputPortImpl::MockFunctionBlockDynamicOutputPortImpl(daq::FunctionBlockTypePtr type,
                                             daq::ContextPtr ctx,
                                             const daq::ComponentPtr& parent,
                                             const daq::StringPtr& localId,
                                             const daq::PropertyObjectPtr& /* config */)
    : FunctionBlockImpl<IFunctionBlock>(type, ctx, parent, localId)
    , outputSignals(daq::List<SignalPtr>())
{
    this->tags.add("mock_fb_dynamic_output_ports");
    createAndAddInputPort("InputPort", PacketReadyNotification::SameThread);
}

daq::FunctionBlockTypePtr MockFunctionBlockDynamicOutputPortImpl::CreateType()
{
    return FunctionBlockType("mock_fb_dynamic_output_ports_uid", "mock_fb_dynamic_output_ports", "", daq::PropertyObject());
}

void MockFunctionBlockDynamicOutputPortImpl::onConnected(const daq::InputPortPtr& /* port */)
{
    std::scoped_lock lock(sync);

    outputSignals.pushBack(createAndAddSignal("OutputSignal1"));
    outputSignals.pushBack(createAndAddSignal("OutputSignal2"));
}

void MockFunctionBlockDynamicOutputPortImpl::onDisconnected(const daq::InputPortPtr& /* port */)
{
    std::scoped_lock lock(sync);
    for (const auto& signal : outputSignals)
    {
        removeSignal(signal);
    }
    outputSignals.clear();
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockFunctionBlockDynamicOutputPort, daq::IFunctionBlock,
    daq::IFunctionBlockType*, type,
    daq::IContext*, ctx,
    daq::IComponent*, parent,
    daq::IString*, localId,
    daq::IPropertyObject*, config)
