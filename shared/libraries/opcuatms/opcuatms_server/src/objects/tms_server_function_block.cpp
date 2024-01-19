#include <opendaq/channel_ptr.h>
#include "opcuatms_server/objects/tms_server_function_block.h"
#include "opcuatms/converters/variant_converter.h"
#include "open62541/statuscodes.h"
#include "open62541/daqbsp_nodeids.h"
#include "open62541/di_nodeids.h"
#include "opendaq/search_filter_factory.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

template <typename T>
TmsServerFunctionBlock<T>::TmsServerFunctionBlock(const FunctionBlockPtr& object, const OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext)
    : Super(object, server, context, tmsContext)
{
}

template <typename T>
OpcUaNodeId TmsServerFunctionBlock<T>::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_FUNCTIONBLOCKTYPE);
}

template <typename T>
bool TmsServerFunctionBlock<T>::createOptionalNode(const OpcUaNodeId& nodeId)
{
    auto browseName = this->server->readBrowseName(nodeId);
    auto create = browseName->name == "Sig" || browseName->name == "IP";
    return create || Super::createOptionalNode(nodeId);
}

template <typename T>
void TmsServerFunctionBlock<T>::bindCallbacks()
{
    this->addReadCallback("FunctionBlockInfo", [this]() {
        auto info = this->object.getFunctionBlockType();
        if (info != nullptr)
            return VariantConverter<IFunctionBlockType>::ToVariant(info);
        else
            return OpcUaVariant();
    });

    Super::bindCallbacks();
}

template <typename T>
void TmsServerFunctionBlock<T>::addChildNodes()
{
    auto signalsNodeId = this->getChildNodeId("Sig");
    assert(!signalsNodeId.isNull());
    
    uint32_t numberInList = 0;
    for (const auto& signal : this->object.getSignals(search::Any()))
    {
        auto tmsSignal = this->template registerTmsObjectOrAddReference<TmsServerSignal>(signalsNodeId, signal, numberInList++);
        signals.push_back(std::move(tmsSignal));
    }

    auto inputPortsNodeId = this->getChildNodeId("IP");
    assert(!inputPortsNodeId.isNull());
    
    numberInList = 0;
    for (const auto& inputPort : this->object.getInputPorts(search::Any()))
    {
        auto tmsInputPort = this->template registerTmsObjectOrAddReference<TmsServerInputPort>(inputPortsNodeId, inputPort, numberInList++);
        inputPorts.push_back(std::move(tmsInputPort));
    }
    
    numberInList = 0;
    for (const auto& fb : this->object.getFunctionBlocks(search::Any()))
    {
        auto tmsFunctionBlock = this->template registerTmsObjectOrAddReference<TmsServerFunctionBlock<>>(this->nodeId, fb, numberInList++);
        functionBlocks.push_back(std::move(tmsFunctionBlock));
    }

    Super::addChildNodes();
}

template <typename T>
void TmsServerFunctionBlock<T>::createNonhierarchicalReferences()
{
    this->createChildNonhierarchicalReferences(signals);
    this->createChildNonhierarchicalReferences(inputPorts);
    this->createChildNonhierarchicalReferences(functionBlocks);
}

// To force the compiler to generate the template classes that are used elsewhere
// If this is not done, then you will get linker errors if using it outside the static library
template class TmsServerFunctionBlock<daq::FunctionBlockPtr>;
template class TmsServerFunctionBlock<daq::ChannelPtr>;

END_NAMESPACE_OPENDAQ_OPCUA_TMS
