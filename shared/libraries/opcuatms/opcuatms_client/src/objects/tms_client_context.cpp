#include <opcuatms_client/objects/tms_client_context.h>
#include <opcuatms_client/tms_attribute_collector.h>
#include <opcuatms/converters/variant_converter.h>
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

void TmsClientContext::addEnumerationTypesToTypeManager()
{
    if (enumerationTypesAdded)
        return;

    if (!context.assigned() || !context.getTypeManager().assigned())
        return; // TypeManager required. Do nothing.

    auto typeManager = context.getTypeManager();

    const auto DataTypeEnumerationNodeId = OpcUaNodeId(UA_NS0ID_ENUMERATION);
    const auto& references = referenceBrowser->browse(DataTypeEnumerationNodeId);
    StructPtr enumValuesStruct;
    std::vector<OpcUaNodeId> vecEnumerationsNodeIds;

    for (auto [browseName, ref] : references.byBrowseName)
        vecEnumerationsNodeIds.push_back(ref->nodeId.nodeId);

    //Cache NodeIds
    referenceBrowser->browseMultiple(vecEnumerationsNodeIds);

    auto listEnumValues = List<IString>();

    for (auto [browseName, ref] : references.byBrowseName)
    {
        //If type already exists, skip
        if(typeManager.hasType(browseName))
            continue;

        const auto& references1 = referenceBrowser->browse(ref->nodeId.nodeId);
        for (auto [childBrowseName, ChildRef] : references1.byBrowseName)
        {
            const auto childNodeValue = client->readValue(ChildRef->nodeId.nodeId);
            const auto childNodeObject = VariantConverter<IBaseObject>::ToDaqObject(childNodeValue, context);

            if (childBrowseName == "EnumStrings")
            {
                for (auto value : childNodeObject.asPtr<IList>())
                    listEnumValues.pushBack(value);
            }
            else if (childBrowseName == "EnumValues")
            {
                for (const auto& value : childNodeObject.asPtr<IList>())
                {
                    if (enumValuesStruct = value.asPtrOrNull<IStruct>(true); enumValuesStruct.assigned())
                        listEnumValues.pushBack(enumValuesStruct.get("DisplayName"));
                }
            }
        }

        auto enumType = EnumerationType(browseName, listEnumValues);

        try
        {
            typeManager.addType(enumType);
        }
        catch (...)
        {
            LOG_I("Failed to add OPC UA type {} to type manager.", enumType.getName());
        }

        listEnumValues.clear();
    }

    enumerationTypesAdded = true;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
