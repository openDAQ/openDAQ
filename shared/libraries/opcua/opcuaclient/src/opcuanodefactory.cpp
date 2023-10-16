#include <utility>

#include "opcuaclient/opcuanodefactory.h"
#include "opcuashared/node/opcuanodeobject.h"
#include "opcuashared/node/opcuadatatype.h"
#include <opcuaclient/browser/opcuabrowser.h>
#include <open62541/client_highlevel.h>
#include <opcuashared/node/opcuanodevariable.h>
#include <opcuaclient/browse_request.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

/*IOpcUaNodeFactory*/

OpcUaNodeId IOpcUaNodeFactory::GetOpcUaNodeId(const UA_ReferenceDescription& referenceDescription)
{
    return OpcUaNodeId(referenceDescription.nodeId.nodeId);
}

/*OpcUaNodeFactory*/

OpcUaNodeFactory::OpcUaNodeFactory(const OpcUaClientPtr& client)
    : client(client)
{
}

OpcUaNodePtr OpcUaNodeFactory::instantiateNode(const UA_ReferenceDescription& reference,
                                               const OpcUaNodeId& parentNodeId,
                                               bool& traverseChild)
{
    traverseChild = true;
    switch (reference.nodeClass)
    {
        case UA_NODECLASS_VARIABLE:
        {
            OpcUaNodeId uaTypeNodeId;
            UA_StatusCode status =
                UA_Client_readDataTypeAttribute(client->getLockedUaClient(), reference.nodeId.nodeId, uaTypeNodeId.get());

            if (OPCUA_STATUSCODE_SUCCEEDED(status))
                return std::make_shared<OpcUaNodeVariable>(reference, uaTypeNodeId);
            else
                return nullptr;
        }
        case UA_NODECLASS_METHOD:
        {
            assert(!parentNodeId.isNull());
            OpcUaNodeMethodPtr nodeMethod = std::make_shared<OpcUaNodeMethod>(reference, parentNodeId);

            prepareMethodParams(nodeMethod);

            return nodeMethod;
        }
        case UA_NODECLASS_OBJECT:
            return std::make_shared<OpcUaNodeObject>(reference);
        case UA_NODECLASS_DATATYPE:
            return std::make_shared<OpcUaDataType>(reference);
        default:
            return nullptr;
    }
}

void OpcUaNodeFactory::prepareMethodParams(OpcUaNodeMethodPtr nodeMethod)
{
    // conditions to browse the method argument nodes: referencetype = UA_NS0ID_HASPROPERTY (=46) && brosweName =  InputArguments  |
    // OutputArguments

    BrowseRequest request(nodeMethod->getNodeId(), OpcUaNodeClass::Variable, OpcUaNodeId(UA_NS0ID_HASPROPERTY));

    OpcUaBrowser browser(request, client);
    browseAndApplyMethodParams(browser, nodeMethod);

    nodeMethod->initTypeDescription();
}

void OpcUaNodeFactory::browseAndApplyMethodParams(OpcUaBrowser& browser, const OpcUaNodeMethodPtr& nodeMethod)
{
    try
    {
        auto& references = browser.browse();

        for (auto& ref : references)
            applyMethodParam(ref, nodeMethod);
    }
    catch (const OpcUaException& ex)
    {
        throw OpcUaException(ex.getStatusCode(),
                             "Method initialization: " + std::string(ex.what()) + ". Node: " + nodeMethod->getNodeId().toString());
    }
}

void OpcUaNodeFactory::applyMethodParam(const UA_ReferenceDescription& reference, const OpcUaNodeMethodPtr& nodeMethod)
{
    auto browseName = OpcUaNode::GetBrowseName(reference.browseName);

    if (browseName == "InputArguments")
        nodeMethod->addInputParameter("", OpcUaNodeId());
    else if (browseName == "OutputArguments")
        nodeMethod->addOutputParameter("", OpcUaNodeId());
}

END_NAMESPACE_OPENDAQ_OPCUA
