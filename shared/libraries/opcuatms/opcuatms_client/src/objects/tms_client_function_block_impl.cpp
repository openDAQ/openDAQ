#include <opendaq/function_block_ptr.h>
#include <opendaq/channel_impl.h>
#include "opcuatms_client/objects/tms_client_function_block_impl.h"
#include "opcuatms_client/objects/tms_client_signal_factory.h"
#include "opcuatms_client/objects/tms_client_function_block_factory.h"
#include "opcuatms_client/objects/tms_client_input_port_factory.h"
#include "open62541/daqbsp_nodeids.h"


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
    clientContext->readObjectAttributes(nodeId);

    readFbType();
    findAndCreateFunctionBlocks();
    findAndCreateSignals();
    findAndCreateInputPorts();
}

template <typename Impl>
CachedReferences TmsClientFunctionBlockBaseImpl<Impl>::getFunctionBlockReferences()
{
    auto filter = BrowseFilter();
    filter.referenceTypeId = OpcUaNodeId(UA_NS0ID_HASCOMPONENT);
    filter.typeDefinition = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_FUNCTIONBLOCKTYPE);
    filter.direction = UA_BROWSEDIRECTION_FORWARD;
    return this->clientContext->getReferenceBrowser()->browseFiltered(this->nodeId, filter);
}

template <typename Impl>
void TmsClientFunctionBlockBaseImpl<Impl>::findAndCreateFunctionBlocks()
{
    std::map<uint32_t, FunctionBlockPtr> orderedFunctionBlocks;
    std::vector<FunctionBlockPtr> unorderedFunctionBlocks;

    const auto& references = getFunctionBlockReferences();
    for (const auto& [browseName, ref] : references.byBrowseName)
    {
        const auto functionBlockNodeId = OpcUaNodeId(ref->nodeId.nodeId);

        try
        {
            // TODO: If there is no access to the nodes within a function blocks an exeption 
            // is thrown which results that the application stops. However, this block should
            // just be ignored. It is not an error at all.
            auto clientFunctionBlock = TmsClientFunctionBlock(this->context, this->functionBlocks, browseName, this->clientContext, functionBlockNodeId);

            const auto numberInList = this->tryReadChildNumberInList(functionBlockNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedFunctionBlocks.count(numberInList))
                orderedFunctionBlocks.insert(std::pair<uint32_t, FunctionBlockPtr>(numberInList, clientFunctionBlock));
            else
                unorderedFunctionBlocks.emplace_back(clientFunctionBlock);
        }
        catch(...)
        {
            // TODO: Log failure to add function/procedure.
            continue;
        }
    }

    for (const auto& val : orderedFunctionBlocks)
        this->addNestedFunctionBlock(val.second);
    for (const auto& val : unorderedFunctionBlocks)
        this->addNestedFunctionBlock(val);
}

template <typename Impl> 
void TmsClientFunctionBlockBaseImpl<Impl>::findAndCreateSignals()
{
    std::map<uint32_t, SignalPtr> orderedSignals;
    std::vector<SignalPtr> unorderedSignals;

    const auto& references = getOutputSignalReferences();

    for (const auto& [signalNodeId, ref] : references.byNodeId)
    {
        auto clientSignal = FindOrCreateTmsClientSignal(this->context, this->signals, this->clientContext, signalNodeId);
        const auto numberInList = this->tryReadChildNumberInList(signalNodeId);
        if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedSignals.count(numberInList))
            orderedSignals.insert(std::pair<uint32_t, SignalPtr>(numberInList, clientSignal));
        else
            unorderedSignals.emplace_back(clientSignal);
    }
    
    for (const auto& val : orderedSignals)
        this->addSignal(val.second);
    for (const auto& val : unorderedSignals)
        this->addSignal(val);
}

template <typename Impl>
CachedReferences TmsClientFunctionBlockBaseImpl<Impl>::getOutputSignalReferences()
{
    auto filter = BrowseFilter();
    filter.referenceTypeId = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASVALUESIGNAL);
    filter.direction = UA_BROWSEDIRECTION_FORWARD;

    auto signalsNodeId = this->getNodeId("Sig");

    return this->clientContext->getReferenceBrowser()->browseFiltered(signalsNodeId, filter);
}

template <typename Impl>
CachedReferences TmsClientFunctionBlockBaseImpl<Impl>::getInputPortBlockReferences()
{
    auto filter = BrowseFilter();
    filter.referenceTypeId = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASINPUTPORT);
    filter.direction = UA_BROWSEDIRECTION_FORWARD;

    auto inputPortsNodeId = this->getNodeId("IP");

    return this->clientContext->getReferenceBrowser()->browseFiltered(inputPortsNodeId, filter);
}

template <typename Impl> 
void TmsClientFunctionBlockBaseImpl<Impl>::findAndCreateInputPorts()
{
    std::map<uint32_t, InputPortPtr> orderedInputPorts;
    std::vector<InputPortPtr> unorderedInputPorts;

    const auto& references = getInputPortBlockReferences();

    for (const auto& [browseName, ref] : references.byBrowseName)
    {
        const auto inputPortNodeId = OpcUaNodeId(ref->nodeId.nodeId);

        auto clientInputPort = TmsClientInputPort(this->context, this->inputPorts, browseName,  this->clientContext, inputPortNodeId);

        const auto numberInList = this->tryReadChildNumberInList(inputPortNodeId);
        if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedInputPorts.count(numberInList))
            orderedInputPorts.insert(std::pair<uint32_t, InputPortPtr>(numberInList, clientInputPort));
        else
            unorderedInputPorts.emplace_back(clientInputPort);
    }
    
    for (const auto& val : orderedInputPorts)
        this->addInputPort(val.second);
    for (const auto& val : unorderedInputPorts)
        this->addInputPort(val);
}

template <typename Impl>
void TmsClientFunctionBlockBaseImpl<Impl>::readFbType()
{
    auto infoNodeId = this->getNodeId("FunctionBlockInfo");
    auto variant = this->clientContext->getAttributeReader()->getValue(infoNodeId, UA_ATTRIBUTEID_VALUE);
    this->type = VariantConverter<IFunctionBlockType>::ToDaqObject(variant).detach();
}

template <typename Impl> 
SignalPtr TmsClientFunctionBlockBaseImpl<Impl>::onGetStatusSignal()
{
    auto filter = BrowseFilter();
    filter.referenceTypeId = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASSTATUSSIGNAL);
    filter.direction = UA_BROWSEDIRECTION_FORWARD;

    const auto& references = this->clientContext->getReferenceBrowser()->browseFiltered(this->nodeId, filter);
    assert(references.byNodeId.size() <= 1);

    if (!references.byNodeId.empty())
    {
        auto signalNodeId = references.byNodeId.begin().key();
        return this->findSignal(signalNodeId);
    }

    return nullptr;
}

// To force the compiler to generate the template classes that are used elsewhere
// If this is not done, then you will get linker errors if using it outside the static library
template class TmsClientFunctionBlockBaseImpl<FunctionBlock>;
template class TmsClientFunctionBlockBaseImpl<Channel>;

END_NAMESPACE_OPENDAQ_OPCUA_TMS
