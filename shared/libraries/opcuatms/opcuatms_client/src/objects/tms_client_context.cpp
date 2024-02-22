#include "opcuatms_client/objects/tms_client_context.h"
#include <opcuatms_client/tms_attribute_collector.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace daq;
using namespace opcua;

TmsClientContext::TmsClientContext(const opcua::OpcUaClientPtr& client, const ContextPtr& context)
    : client(client)
    , context(context)
    , loggerComponent(context.getLogger().assigned() ? context.getLogger().getOrAddComponent("TmsClientContext")
                                                     : throw ArgumentNullException("Logger must not be null"))
{
    initReferenceBrowser();
    initAttributeReader();
}

const opcua::OpcUaClientPtr& TmsClientContext::getClient() const
{
    return client;
}

void TmsClientContext::registerRootDevice(const DevicePtr& rootDevice)
{
    this->rootDevice = rootDevice;
}

DevicePtr TmsClientContext::getRootDevice()
{
    return this->rootDevice.getRef();
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

AttributeReaderPtr TmsClientContext::getAttributeReader()
{
    return attributeReader;
}

void TmsClientContext::readObjectAttributes(const OpcUaNodeId& nodeId, bool forceRead)
{
    if (!forceRead && attributeReader->hasAnyValue(nodeId))
        return;

    auto collector = TmsAttributeCollector(referenceBrowser);
    auto attributes = collector.collectAttributes(nodeId);

    attributeReader->setAttibutes(attributes);
    attributeReader->read();
}

size_t TmsClientContext::getMaxNodesPerBrowse()
{
    return maxNodesPerBrowse;
}

size_t TmsClientContext::getMaxNodesPerRead()
{
    return maxNodesPerRead;
}

void TmsClientContext::initReferenceBrowser()
{
    try
    {
        const auto maxNodesPerBrowseId = OpcUaNodeId(UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERBROWSE);
        maxNodesPerBrowse = client->readValue(maxNodesPerBrowseId).toInteger();
    }
    catch (const std::exception& e)
    {
        LOG_W("Failed to read maxNodesPerBrowse variable: {}", e.what());
    }
    
    referenceBrowser = std::make_shared<CachedReferenceBrowser>(client, maxNodesPerBrowse);
}

void TmsClientContext::initAttributeReader()
{
    try
    {
        const auto maxNodesPerReadId = OpcUaNodeId(UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERREAD);
        maxNodesPerRead = client->readValue(maxNodesPerReadId).toInteger();
    }
    catch (const std::exception& e)
    {
        LOG_W("Failed to read maxNodesPerRead variable: {}", e.what());
    }

    attributeReader = std::make_shared<AttributeReader>(client, maxNodesPerRead);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
