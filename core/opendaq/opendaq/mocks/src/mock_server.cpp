#include "opendaq/mock/mock_server.h"
#include <opendaq/function_block_impl.h>
#include <opendaq/function_block_type_factory.h>
#include <coreobjects/property_factory.h>

using namespace daq;

namespace
{
    // Server function block that accepts nested blocks of its own type.
    class MockServerFbImpl final : public FunctionBlock
    {
    public:
        MockServerFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
            : FunctionBlock(FunctionBlockType("mockfb1", "MockFB1", "Mock FB1"), ctx, parent, localId)
        {
            createAndAddInputPort("ip", PacketReadyNotification::None);
            createAndAddSignal("srvSig");
            objPtr.addProperty(StringProperty("MockString", "Default"));
        }

        DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override
        {
            return Dict<IString, IFunctionBlockType>({{"mockfb1", FunctionBlockType("mockfb1", "MockFB1", "Mock FB1")}});
        }

        FunctionBlockPtr onAddFunctionBlock(const StringPtr& /*typeId*/, const PropertyObjectPtr& /*config*/) override
        {
            const auto fb = createWithImplementation<IFunctionBlock, MockServerFbImpl>(context, functionBlocks, "nestedFb");
            addNestedFunctionBlock(fb);
            return fb;
        }
    };
}

MockServerImpl::MockServerImpl(const StringPtr& id, const DevicePtr& rootDevice, const ContextPtr& context)
    : Server(id, nullptr, rootDevice, context)
{
}

DictPtr<IString, IFunctionBlockType> MockServerImpl::onGetAvailableFunctionBlockTypes()
{
    return Dict<IString, IFunctionBlockType>({{"mockfb1", FunctionBlockType("mockfb1", "MockFB1", "Mock FB1")}});
}

FunctionBlockPtr MockServerImpl::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    if (typeId != "mockfb1")
        DAQ_THROW_EXCEPTION(NotFoundException);

    StringPtr localId = "srvFb";
    if (config.assigned() && config.hasProperty("LocalId"))
        localId = config.getPropertyValue("LocalId");

    const auto fb = createWithImplementation<IFunctionBlock, MockServerFbImpl>(this->context, this->functionBlocks, localId);
    addNestedFunctionBlock(fb);
    return fb;
}

void MockServerImpl::onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock)
{
    removeNestedFunctionBlock(functionBlock);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, MockServer, IServer,
    const StringPtr&, id,
    const DevicePtr&, rootDevice,
    const ContextPtr&, context
)
