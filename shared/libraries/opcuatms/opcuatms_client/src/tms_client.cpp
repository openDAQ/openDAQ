#include <opendaq/packet_factory.h>
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
#include <opcuatms_client/tms_attribute_collector.h>

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
    , loggerComponent(context.getLogger().assigned() ? context.getLogger().getOrAddComponent("OpcUaClient")
                                                     : throw ArgumentNullException("Logger must not be null"))
{
}

daq::DevicePtr TmsClient::connect()
{
    const auto startTime = std::chrono::steady_clock::now();

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

    tmsClientContext = std::make_shared<TmsClientContext>(client, context);
    tmsClientContext->addEnumerationTypesToTypeManager();

    OpcUaNodeId rootDeviceNodeId;
    std::string rootDeviceBrowseName;
    getRootDeviceNodeAttributes(rootDeviceNodeId, rootDeviceBrowseName);

    auto device = TmsClientRootDevice(context, parent, rootDeviceBrowseName, tmsClientContext, rootDeviceNodeId, createStreamingCallback);

    const auto deviceInfo = device.getInfo();
    if (deviceInfo.hasProperty("OpenDaqPackageVersion"))
    {
        const std::string packageVersion = deviceInfo.getPropertyValue("OpenDaqPackageVersion");

        if (packageVersion != OPENDAQ_PACKAGE_VERSION)
        {
            LOG_D("Connected to openDAQ OPC UA server with different version. Client version: {}, server version: {}",
                  OPENDAQ_PACKAGE_VERSION,
                  packageVersion);
        }
    }

    const auto endTime = std::chrono::steady_clock::now();
    const auto connectTime = std::chrono::duration<double>(endTime - startTime);
    LOG_D("Connected to openDAQ OPC UA server {}. Connect took {:.2f} s.", opcUaUrl, connectTime.count());
    return device;
}

void TmsClient::getRootDeviceNodeAttributes(OpcUaNodeId& nodeIdOut, std::string& browseNameOut)
{
    const OpcUaNodeId rootNodeId(NAMESPACE_DI, UA_DIID_DEVICESET);

    auto filter = BrowseFilter();
    filter.referenceTypeId = OpcUaNodeId(0, UA_NS0ID_HASCOMPONENT);
    filter.typeDefinition = OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_DAQDEVICETYPE);

    const auto& references = tmsClientContext->getReferenceBrowser()->browseFiltered(rootNodeId, filter);

    if (references.byNodeId.empty())
        throw NotFoundException();

    nodeIdOut = OpcUaNodeId(references.byBrowseName.begin().value()->nodeId.nodeId);
    browseNameOut = references.byBrowseName.begin().key();
}

END_NAMESPACE_OPENDAQ_OPCUA
