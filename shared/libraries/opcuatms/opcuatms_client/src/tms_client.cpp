#include <opendaq/packet_factory.h>
#include <opcuaclient/browser/opcuabrowser.h>
#include <open62541/daq_opcua_nodesets.h>
#include <open62541/di_nodeids.h>
#include <open62541/daqdevice_nodeids.h>
#include <open62541/types_di_generated.h>
#include <open62541/types_daqbsp_generated.h>
#include <open62541/types_daqdevice_generated.h>
#include <open62541/types_daqesp_generated.h>
#include <open62541/types_daqhbk_generated.h>

#include <opcuatms_client/tms_client.h>
#include <opcuatms_client/tms_attribute_collector.h>
#include <opcuatms_client/objects/tms_client_device_factory.h>

using namespace daq::opcua;
using namespace daq::opcua::tms;

BEGIN_NAMESPACE_OPENDAQ_OPCUA

TmsClient::TmsClient(const ContextPtr& context,
                     const ComponentPtr& parent,
                     const std::string& opcUaUrl)
    : TmsClient(context, parent, OpcUaEndpoint(opcUaUrl))
{
}

TmsClient::TmsClient(const ContextPtr& context,
                     const ComponentPtr& parent,
                     const OpcUaEndpoint& endpoint)
    : context(context)
    , endpoint(endpoint)
    , parent(parent)
    , loggerComponent(context.getLogger().assigned() ? context.getLogger().getOrAddComponent("OpenDAQOPCUAClientModule")
                                                     : throw ArgumentNullException("Logger must not be null"))
{
}

daq::DevicePtr TmsClient::connect()
{
    const auto startTime = std::chrono::steady_clock::now();

    createAndConectClient();
    client->runIterate();

    // A first connect is needed to read from the server the available namespaces out from the server
    auto namespaces = VariantConverter<IString>::ToDaqList(client->readValue(OpcUaNodeId(0,UA_NS0ID_SERVER_NAMESPACEARRAY)));

    client->stopIterate();
    client->disconnect();

    // After a disconnect, we need to register the data types, but only these which are available on server side.
    registerDaqTypes(endpoint, namespaces);

    createAndConectClient();
    client->runIterate();

    tmsClientContext = std::make_shared<TmsClientContext>(client, context);
    tmsClientContext->addEnumerationTypesToTypeManager();

    OpcUaNodeId rootDeviceNodeId;
    std::string rootDeviceBrowseName;
    getRootDeviceNodeAttributes(rootDeviceNodeId, rootDeviceBrowseName);

    auto device = TmsClientRootDevice(context, parent, rootDeviceBrowseName, tmsClientContext, rootDeviceNodeId);

    const auto deviceInfo = device.getInfo();
    deviceInfo.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("connectionString", endpoint.getUrl());

    const std::string packageVersion = deviceInfo.getSdkVersion();
    if (!packageVersion.empty() && packageVersion != OPENDAQ_PACKAGE_VERSION)
    {
        LOG_D("Connected to openDAQ OPC UA server with different version. Client version: {}, server version: {}",
                OPENDAQ_PACKAGE_VERSION,
                packageVersion);
    }

    const auto endTime = std::chrono::steady_clock::now();
    const auto connectTime = std::chrono::duration<double>(endTime - startTime);
    LOG_D("Connected to openDAQ OPC UA server {}. Connect took {:.2f} s.", endpoint.getUrl(), connectTime.count());
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
        DAQ_THROW_EXCEPTION(NotFoundException);

    nodeIdOut = OpcUaNodeId(references.byBrowseName.begin().value()->nodeId.nodeId);
    browseNameOut = references.byBrowseName.begin().key();
}

void TmsClient::createAndConectClient()
{
    try
    {
        client = std::make_shared<OpcUaClient>(endpoint);
        client->connect();
    }
    catch (const OpcUaException& e)
    {
        switch (e.getStatusCode())
        {
            case UA_STATUSCODE_BADUSERACCESSDENIED:
            case UA_STATUSCODE_BADIDENTITYTOKENINVALID:
                DAQ_THROW_EXCEPTION(AuthenticationFailedException, e.what());
            default:
                DAQ_THROW_EXCEPTION(NotFoundException, e.what());
        }
    }
}

END_NAMESPACE_OPENDAQ_OPCUA
