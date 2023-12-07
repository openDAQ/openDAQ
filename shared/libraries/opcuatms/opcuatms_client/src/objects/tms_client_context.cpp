#include "opcuatms_client/objects/tms_client_context.h"
#include <opcuashared/opcualog.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsClientContext::TmsClientContext(const OpcUaClientPtr& client)
    : client(client)
{
    initReferenceBrowser();
}

const opcua::OpcUaClientPtr& TmsClientContext::getClient() const
{
    return client;
}

void TmsClientContext::registerObject(const OpcUaNodeId& nodeId, const BaseObjectPtr& object)
{
    std::lock_guard guard(mutex);
    objects[nodeId] = object.getObject();
}

void TmsClientContext::unregisterObject(const OpcUaNodeId& nodeId)
{
    std::lock_guard guard(mutex);
    objects.extract(nodeId);
}

BaseObjectPtr TmsClientContext::getObject(const opcua::OpcUaNodeId& nodeId) const
{
    std::lock_guard guard(mutex);
    auto it = objects.find(nodeId);
    if (it != objects.end())
    {
        IBaseObject* obj = it->second;
        return BaseObjectPtr(obj);
    }
        
    return {};
}
opcua::OpcUaNodeId TmsClientContext::getNodeId(const BaseObjectPtr object) const
{
    for (auto pair : objects)
    {
        if (object == pair.second)
            return pair.first;
    }
    return opcua::OpcUaNodeId();
}

CachedReferenceBrowserPtr TmsClientContext::getReferenceBrowser()
{
    return referenceBrowser;
}

void TmsClientContext::initReferenceBrowser()
{
    size_t maxNodesPerBrowse = 0;

    try
    {
        const auto maxNodesPerBrowseId = OpcUaNodeId(UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERBROWSE);
        maxNodesPerBrowse = client->readValue(maxNodesPerBrowseId).toInteger();
    }
    catch (const std::exception& e)
    {
        LOGW << "Failed to read maxNodesPerBrowse variable: " << e.what(); 
    }
    
    referenceBrowser = std::make_shared<CachedReferenceBrowser>(client, maxNodesPerBrowse);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
