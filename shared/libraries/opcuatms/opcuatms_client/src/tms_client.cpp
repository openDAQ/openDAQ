#include <opendaq/packet_factory.h>
#include <opcuaclient/browse_request.h>
#include <opcuaclient/browser/opcuabrowser.h>
#include <opcuatms_client/tms_client.h>
#include <open62541/daq_opcua_nodesets.h>
#include <open62541/di_nodeids.h>
#include <open62541/daqdevice_nodeids.h>
#include <open62541/types_di_generated.h>
#include <open62541/types_daqbsp_generated.h>
#include <open62541/types_daqdevice_generated.h>
#include <open62541/types_daqesp_generated.h>
#include <open62541/types_daqhbk_generated.h>


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

    client = std::make_shared<OpcUaClient>(endpoint);
    if (!client->connect())
        throw NotFoundException();
    client->runIterate();

    // A first connect is needed to read from the server the available namespaces out from the server
    auto namespaces = VariantConverter<IString>::ToDaqList(client->readValue(OpcUaNodeId(0,UA_NS0ID_SERVER_NAMESPACEARRAY)));

    client->stopIterate();
    client->disconnect();

    // After a disconnect, we need to register the data types, but only these which are available on server side.
    registerDaqTypes(endpoint, namespaces);
    client = std::make_shared<OpcUaClient>(endpoint);
    if (!client->connect())
        throw NotFoundException();
    client->runIterate();

    tmsClientContext = std::make_shared<TmsClientContext>(client);

    auto rootDeviceNodeId = getRootDeviceNodeId();

    BrowseRequest request(rootDeviceNodeId, OpcUaNodeClass::Variable);
    auto localId = getUniqueLocalId(client->readBrowseName(rootDeviceNodeId));

    auto device = TmsClientRootDevice(context, parent, localId, tmsClientContext, rootDeviceNodeId, createStreamingCallback);

    const auto deviceInfo = device.getInfo();
    if (deviceInfo.hasProperty("OpenDaqPackageVersion"))
    {
        const std::string packageVersion = deviceInfo.getPropertyValue("OpenDaqPackageVersion");
        if (packageVersion != OPENDAQ_PACKAGE_VERSION)
        {
            const auto logger = context.getLogger();
            if (logger.assigned())
            {
                const auto loggerComponent = logger.getOrAddComponent("OpcUaClient");
                LOG_I("Connected to openDAQ OPC UA server with different version. Client version: {}, server version: {}", OPENDAQ_PACKAGE_VERSION, packageVersion)
            }
        }
    }

    return device;
}

OpcUaNodeId TmsClient::getRootDeviceNodeId()
{
    const OpcUaNodeId rootNodeId(NAMESPACE_DI, UA_DIID_DEVICESET);
    BrowseRequest br(rootNodeId, OpcUaNodeClass::Object, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));

    OpcUaBrowser browser(br, client);
    auto results = browser.browse();

    const OpcUaNodeId daqDeviceTypeNodeId(  NAMESPACE_DAQDEVICE,            // TODO NAMESPACE_DAQDEVICE is server namespace id. You need
                                            UA_DAQDEVICEID_DAQDEVICETYPE);  // to map it to client. Or use namespace URI.
    
    
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
