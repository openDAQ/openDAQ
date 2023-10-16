#include <opendaq/packet_factory.h>
#include <opcuaclient/browse_request.h>
#include <opcuaclient/browser/opcuabrowser.h>
#include <opcuatms_client/tms_client.h>
#include <open62541/di_nodeids.h>
#include <open62541/tmsdevice_nodeids.h>
#include <open62541/types_di_generated.h>
#include <open62541/types_tmsbsp_generated.h>
#include <open62541/types_tmsdevice_generated.h>
#include <open62541/types_tmsesp_generated.h>

#include <iostream>

using namespace daq::opcua;
using namespace daq::opcua::tms;

BEGIN_NAMESPACE_OPENDAQ_OPCUA

TmsClient::TmsClient(const ContextPtr& context,
                     const ComponentPtr& parent,
                     const std::string& opcUaUrl,
                     const FunctionPtr& createStreamingCallback)
    : context(context)
    , opcUaUrl(opcUaUrl)
    , createStreamingCallback(createStreamingCallback)
    , parent(parent)
{
}

daq::DevicePtr TmsClient::connect()
{
    OpcUaEndpoint endpoint("TmsClient", opcUaUrl);
    endpoint.registerCustomTypes(UA_TYPES_DI_COUNT, UA_TYPES_DI);
    endpoint.registerCustomTypes(UA_TYPES_TMSBT_COUNT, UA_TYPES_TMSBT);
    endpoint.registerCustomTypes(UA_TYPES_TMSBSP_COUNT, UA_TYPES_TMSBSP);
    endpoint.registerCustomTypes(UA_TYPES_TMSDEVICE_COUNT, UA_TYPES_TMSDEVICE);
    endpoint.registerCustomTypes(UA_TYPES_TMSESP_COUNT, UA_TYPES_TMSESP);

    client = std::make_shared<OpcUaClient>(endpoint);
    if (!client->connect())
        throw NotFoundException();
    client->runIterate();

    tmsClientContext = std::make_shared<TmsClientContext>(client);

    auto rootDeviceNodeId = getRootDeviceNodeId();

    BrowseRequest request(rootDeviceNodeId, OpcUaNodeClass::Variable);
    auto localId = getUniqueLocalId(client->readBrowseName(rootDeviceNodeId));

    auto device = TmsClientRootDevice(context, parent, localId, tmsClientContext, rootDeviceNodeId, createStreamingCallback);
    return device;
}

OpcUaNodeId TmsClient::getRootDeviceNodeId()
{
    const OpcUaNodeId rootNodeId(NAMESPACE_DI, UA_DIID_DEVICESET);
    BrowseRequest br(rootNodeId, OpcUaNodeClass::Object, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));

    OpcUaBrowser browser(br, client);
    auto results = browser.browse();

    const OpcUaNodeId daqDeviceTypeNodeId(  NAMESPACE_TMSDEVICE,            // TODO NAMESPACE_TMSDEVICE is server namespace id. You need
                                            UA_TMSDEVICEID_DAQDEVICETYPE);  // to map it to client. Or use namespace URI.
    
    
    for (const auto& result : results)
    {   
        ReferenceUtils referenceUtilities(client);
        if (daqDeviceTypeNodeId == result.typeDefinition.nodeId)
            return result.nodeId.nodeId;
        else
        {
            // Else check if this is a subtype of the openDAQ device.
            if (referenceUtilities.isInstanceOf(result.typeDefinition.nodeId, daqDeviceTypeNodeId))
                return result.nodeId.nodeId;
        }
    }
    throw NotFoundException();
}

StringPtr TmsClient::getUniqueLocalId(const StringPtr& localId, int iteration)
{
    if (!parent.assigned())
        return localId;

    StringPtr uniqueId = localId;
    if (iteration != 0)
        uniqueId = uniqueId + "_" + std::to_string(iteration);

    const auto parentFolder = parent.asPtrOrNull<IFolder>();
    if (parentFolder.assigned())
    {
        for (auto item : parentFolder.getItems())
        {
            if (item.getLocalId() == uniqueId)
                return getUniqueLocalId(localId, iteration + 1);
        }
    }
    
    return uniqueId;
}

END_NAMESPACE_OPENDAQ_OPCUA
