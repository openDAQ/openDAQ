#include <opendaq/function_block_ptr.h>
#include <opendaq/channel_impl.h>
#include "opcuatms_client/objects/tms_client_function_block_impl.h"
#include "opcuatms_client/objects/tms_client_signal_factory.h"
#include "opcuatms_client/objects/tms_client_function_block_factory.h"
#include "opcuatms_client/objects/tms_client_input_port_factory.h"
#include "open62541/tmsbsp_nodeids.h"


BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

template <typename Impl> 
TmsClientFunctionBlockBaseImpl<Impl>::TmsClientFunctionBlockBaseImpl(
    const ContextPtr& context,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const TmsClientContextPtr& clientContext,
    const OpcUaNodeId& nodeId
)
    : TmsClientComponentBaseImpl<Impl>(context, parent, localId, clientContext, nodeId, nullptr)
{
    readFbType();
    findAndCreateFunctionBlocks();
    findAndCreateSignals();
    findAndCreateInputPorts();
}

template <typename Impl>
tsl::ordered_set<daq::opcua::OpcUaNodeId> TmsClientFunctionBlockBaseImpl<Impl>::getFunctionBlockNodeIds()
{
    const OpcUaNodeId referenceTypeId(UA_NS0ID_HASCOMPONENT);
    const OpcUaNodeId functionBlockType(NAMESPACE_TMSBSP, UA_TMSBSPID_FUNCTIONBLOCKTYPE); // TODO subtypes
    return this->referenceUtils.getReferencedNodes(this->nodeId, referenceTypeId, true, functionBlockType);
}

template <typename Impl> 
tsl::ordered_set<daq::opcua::OpcUaNodeId> TmsClientFunctionBlockBaseImpl<Impl>::getOutputSignalNodeIds()
{
    auto signalsNodeId = this->getNodeId("OutputSignals");
    OpcUaNodeId referenceTypeId(NAMESPACE_TMSBSP, UA_TMSBSPID_HASVALUESIGNAL);

    return this->referenceUtils.getReferencedNodes(signalsNodeId, referenceTypeId, true);
}

template <typename Impl>
void TmsClientFunctionBlockBaseImpl<Impl>::findAndCreateFunctionBlocks()
{
    auto functionBlockNodeIds = getFunctionBlockNodeIds();
    for (const auto& functionBlockNodeId : functionBlockNodeIds)
    {
        auto browseName = this->client->readBrowseName(functionBlockNodeId);
        try
        {
            // TODO: If there is no access to the nodes within a function blocks an exeption 
            // is thrown which results that the application stops. However, this block should
            // just be ignored. It is not an error at all.
            auto clientFunctionBlock = TmsClientFunctionBlock(this->context, this->functionBlocks, browseName, this->clientContext, functionBlockNodeId);
            this->addNestedFunctionBlock(clientFunctionBlock);
        }
        catch(...)
        {
            // TODO: Log failure to add function/procedure.
            continue;
        }
    }
}

template <typename Impl> 
void TmsClientFunctionBlockBaseImpl<Impl>::findAndCreateSignals()
{
    auto signalNodeIds = getOutputSignalNodeIds();
    for (const auto& signalNodeId : signalNodeIds)
    {
        auto clientSignal = FindOrCreateTmsClientSignal(this->context, this->signals, this->clientContext, signalNodeId);
        this->addSignal(clientSignal);
    }
}

template <typename Impl> 
tsl::ordered_set<daq::opcua::OpcUaNodeId> TmsClientFunctionBlockBaseImpl<Impl>::getInputPortNodeIds()
{
    auto inputPortsNodeId = this->getNodeId("InputPorts");
    OpcUaNodeId referenceTypeId(NAMESPACE_TMSBSP, UA_TMSBSPID_HASINPUTPORT);

    return this->referenceUtils.getReferencedNodes(inputPortsNodeId, referenceTypeId, true);
}

template <typename Impl> 
void TmsClientFunctionBlockBaseImpl<Impl>::findAndCreateInputPorts()
{
    auto inputPortNodeIds = getInputPortNodeIds();
    for (const auto& inputPortNodeId : inputPortNodeIds)
    {
        auto browseName = this->client->readBrowseName(inputPortNodeId);
        auto clientInputPort = TmsClientInputPort(this->context, this->inputPorts, browseName,  this->clientContext, inputPortNodeId);

        this->addInputPort(clientInputPort);
    }
}

template <typename Impl>
void TmsClientFunctionBlockBaseImpl<Impl>::readFbType()
{
    auto variant = this->readValue("FunctionBlockInfo");
    this->type = VariantConverter<IFunctionBlockType>::ToDaqObject(variant).detach();
}

template <typename Impl> 
SignalPtr TmsClientFunctionBlockBaseImpl<Impl>::onGetStatusSignal()
{
    OpcUaNodeId referenceTypeId(NAMESPACE_TMSBSP, UA_TMSBSPID_HASSTATUSSIGNAL);

    auto nodeIds = this->referenceUtils.getReferencedNodes(this->nodeId, referenceTypeId, true);

    assert(nodeIds.size() <= 1);
    if (!nodeIds.empty())
    {
        auto signalNodeId = *nodeIds.begin();
        return this->findSignal(signalNodeId);
    }

    // Not Found
    return nullptr;
}

// To force the compiler to generate the template classes that are used elsewhere
// If this is not done, then you will get linker errors if using it outside the static library
template class TmsClientFunctionBlockBaseImpl<FunctionBlock>;
template class TmsClientFunctionBlockBaseImpl<Channel>;

END_NAMESPACE_OPENDAQ_OPCUA_TMS
