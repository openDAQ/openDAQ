#include "opcuatms_client/objects/tms_client_context.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsClientContext::TmsClientContext(const OpcUaClientPtr& client)
    : client(client)
    , referenceBrowser(std::make_shared<CachedReferenceBrowser>(client))
{
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

END_NAMESPACE_OPENDAQ_OPCUA_TMS
