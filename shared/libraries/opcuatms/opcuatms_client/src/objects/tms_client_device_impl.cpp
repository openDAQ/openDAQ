#include "opcuatms_client/objects/tms_client_device_impl.h"
#include <coreobjects/unit_factory.h>
#include <opcuaclient/browse_request.h>
#include <opcuaclient/browser/opcuabrowser.h>
#include <opcuatms_client/objects/tms_client_channel_factory.h>
#include <opcuatms_client/objects/tms_client_device_factory.h>
#include <opcuatms_client/objects/tms_client_folder_factory.h>
#include <opcuatms_client/objects/tms_client_function_block_factory.h>
#include <opcuatms_client/objects/tms_client_property_object_factory.h>
#include <opcuatms_client/objects/tms_client_signal_factory.h>
#include "opcuatms_client/objects/tms_client_component_factory.h"
#include "opcuatms_client/objects/tms_client_io_folder_factory.h"
#include "opcuatms_client/objects/tms_client_streaming_info_factory.h"
#include <open62541/tmsbsp_nodeids.h>
#include "open62541/tmsbt_nodeids.h"
#include <open62541/tmsdevice_nodeids.h>
#include <open62541/types_tmsdevice_generated.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/custom_log.h>
#include "opcuatms/core_types_utils.h"
#include "opcuatms/exceptions.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS
using namespace daq::opcua;

namespace detail
{
    static std::unordered_set<std::string> defaultComponents = {"Signals", "FunctionBlocks", "InputsOutputs", "StreamingOptions"};

    static std::unordered_map<std::string, std::function<void (const DeviceInfoConfigPtr&, const OpcUaVariant&)>> deviceInfoSetterMap = {
        {"AssetId", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setAssetId(v.toString()); }},
        {"ComponentName", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v){ info.setName(v.toString()); }},
        {"DeviceClass", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setDeviceClass(v.toString()); }},
        {"DeviceManual", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setDeviceManual(v.toString()); }},
        {"DeviceRevision", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setDeviceRevision(v.toString()); }},
        {"HardwareRevision", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setHardwareRevision(v.toString()); }},
        {"Manufacturer", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setManufacturer(v.toString()); }},
        {"ManufacturerUri", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setManufacturerUri(v.toString()); }},
        {"Model", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setModel(v.toString()); }},
        {"ProductCode", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setProductCode(v.toString()); }},
        {"ProductInstanceUri", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setProductInstanceUri(v.toString()); }},
        {"RevisionCounter", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setRevisionCounter(v.toInteger()); }},
        {"SerialNumber", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setSerialNumber(v.toString()); }},
        {"SoftwareRevision", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setSoftwareRevision(v.toString()); }},
        {"MacAddress", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setMacAddress(v.toString()); }},
        {"ParentMacAddress", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setParentMacAddress(v.toString()); }},
        {"Platform", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setPlatform(v.toString()); }},
        {"Position", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setPosition(v.toInteger()); }},
        {"SystemType", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setSystemType(v.toString()); }},
        {"SystemUUID", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setSystemUuid(v.toString()); }}
    };
}

TmsClientDeviceImpl::TmsClientDeviceImpl(const ContextPtr& ctx,
                                         const ComponentPtr& parent,
                                         const StringPtr& localId,
                                         const TmsClientContextPtr& clientContext,
                                         const opcua::OpcUaNodeId& nodeId,
                                         const FunctionPtr& createStreamingCallback,
                                         bool isRootDevice)
    : TmsClientComponentBaseImpl(ctx, parent, localId, clientContext, nodeId)
    , createStreamingCallback(createStreamingCallback)
    , isRootDevice(isRootDevice)
    , logger(ctx.getLogger())
    , loggerComponent( this->logger.assigned()
                          ? this->logger.getOrAddComponent("TmsClientDevice")
                          : throw ArgumentNullException("Logger must not be null"))
{
    findAndCreateSubdevices();
    findAndCreateFunctionBlocks();
    findAndCreateSignals();
    findAndCreateInputsOutputs();
    findAndCreateCustomComponents();

    findAndCreateStreamingOptions();
    connectToStreamings();
    setUpStreamings();
}

void TmsClientDeviceImpl::findAndCreateSubdevices()
{
    auto subdeviceNodeIds = this->getChildNodes(client, nodeId, OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_DAQDEVICETYPE));
    for (const auto& subdeviceNodeId : subdeviceNodeIds)
    {
        auto browseName = client->readBrowseName(subdeviceNodeId);
        auto clientSubdevice = TmsClientDevice(context, devices, browseName, clientContext, subdeviceNodeId, createStreamingCallback);
        addSubDevice(clientSubdevice);
    }
}

DevicePtr TmsClientDeviceImpl::onAddDevice(const StringPtr& /*connectionString*/, const PropertyObjectPtr& /*config*/)
{
    throw OpcUaClientCallNotAvailableException();
}

void TmsClientDeviceImpl::onRemoveDevice(const DevicePtr& /*device*/)
{
    throw OpcUaClientCallNotAvailableException();
}

DeviceInfoPtr TmsClientDeviceImpl::onGetInfo()
{
    if (deviceInfo.assigned())
        return deviceInfo;

    deviceInfo = DeviceInfo("");

    BrowseRequest request(nodeId, OpcUaNodeClass::Variable);
    OpcUaBrowser browser(request, client);
    const auto& browseResult = browser.browse();

    for (const UA_ReferenceDescription& reference : browseResult)
    {
        std::string browseName = daq::opcua::utils::ToStdString(reference.browseName.name);
        if (detail::deviceInfoSetterMap.count(browseName))
            detail::deviceInfoSetterMap[browseName](deviceInfo, client->readValue(OpcUaNodeId(reference.nodeId.nodeId)));
        else if (browseName != "NumberInList" && browseName != "OpenDaqPackageVersion")
        {
            // TODO: Group requests for data type and scalar/array checks and only read required values
            try
            {
                auto value = client->readValue(OpcUaNodeId(reference.nodeId.nodeId));
                if (value.isScalar())
                {
                    if (value.isString())
                        deviceInfo.addProperty(StringProperty(browseName, value.toString()));
                    else if(value.isBool())
                        deviceInfo.addProperty(BoolProperty(browseName, value.toBool()));
                    else if(value.isDouble())
                        deviceInfo.addProperty(FloatProperty(browseName, value.toDouble()));
                    else if(value.isInteger())
                        deviceInfo.addProperty(IntProperty(browseName, value.toInteger()));
                }
            }
            catch(...)
            {
            }
        }
    }

    deviceInfo.freeze();
    return deviceInfo;
}

void TmsClientDeviceImpl::fetchTimeDomain()
{
    if (timeDomainFetched)
        return;
    auto timeDomainNodeId = getNodeId("Domain");
    auto variant = client->readValue(timeDomainNodeId);

    UA_DeviceDomainStructure* deviceDomain;
    deviceDomain = (UA_DeviceDomainStructure*) variant.getValue().data;

    auto numerator = deviceDomain->resolution.numerator;
    auto denominator = deviceDomain->resolution.denominator;
    if (denominator == 0)
        denominator = 1;
    resolution = Ratio(numerator, denominator);
    origin = ConvertToDaqCoreString(deviceDomain->origin);
    if (deviceDomain->unit.unitId > 0)
        domainUnit = Unit(ConvertToDaqCoreString(deviceDomain->unit.displayName.text),
                          deviceDomain->unit.unitId,
                          ConvertToDaqCoreString(deviceDomain->unit.description.text),
                          ConvertToDaqCoreString(deviceDomain->unit.quantity));
    else
        domainUnit = Unit("");
    timeDomainFetched = true;
}

void TmsClientDeviceImpl::fetchTicksSinceOrigin()
{
    auto timeDomainNodeId = getNodeId("Domain");
    auto variant = client->readValue(timeDomainNodeId);

    UA_DeviceDomainStructure* deviceDomain;
    deviceDomain = (UA_DeviceDomainStructure*) variant.getValue().data;
    ticksSinceOrigin = deviceDomain->ticksSinceOrigin;
}

RatioPtr TmsClientDeviceImpl::onGetResolution()
{
    fetchTimeDomain();
    return resolution;
}

uint64_t TmsClientDeviceImpl::onGetTicksSinceOrigin()
{
    if(!timeDomainFetched)
        fetchTimeDomain();
    else
        fetchTicksSinceOrigin();

    return ticksSinceOrigin;
}

std::string TmsClientDeviceImpl::onGetOrigin()
{
    fetchTimeDomain();
    return origin;
}

UnitPtr TmsClientDeviceImpl::onGetDomainUnit()
{
    fetchTimeDomain();
    return domainUnit;
}

void TmsClientDeviceImpl::findAndCreateFunctionBlocks()
{
    auto functionBlocksNodeId = getNodeId("FunctionBlocks");
    auto functionBlockNodeIds =
        this->getChildNodes(client, functionBlocksNodeId, OpcUaNodeId(NAMESPACE_TMSBSP, UA_TMSBSPID_FUNCTIONBLOCKTYPE));
    for (const auto& functionBlockNodeId : functionBlockNodeIds)
    {
        auto browseName = client->readBrowseName(functionBlockNodeId);
        auto clientFunctionBlock = TmsClientFunctionBlock(context, this->functionBlocks, browseName, clientContext, functionBlockNodeId);
        this->addNestedFunctionBlock(clientFunctionBlock);
    }
}

void TmsClientDeviceImpl::findAndCreateSignals()
{
    auto signalsNodeId = getNodeId("Signals");
    auto signalNodeIds = this->getChildNodes(client, signalsNodeId, OpcUaNodeId(NAMESPACE_TMSBSP, UA_TMSBSPID_SIGNALTYPE));
    for (const auto& signalNodeId : signalNodeIds)
    {
        auto clientSignal = FindOrCreateTmsClientSignal(context, signals, clientContext, signalNodeId);
        this->addSignal(clientSignal);
    }
}

void TmsClientDeviceImpl::findAndCreateInputsOutputs()
{
    this->ioFolder.clear();
    auto inputsOutputsNodeId = getNodeId("InputsOutputs");

    auto channelNodeIds = this->getChildNodes(client, inputsOutputsNodeId, OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_CHANNELTYPE));
    for (const auto& channelNodeId : channelNodeIds)
    {
        auto browseName = client->readBrowseName(channelNodeId);
        auto tmsClientChannel = TmsClientChannel(context, this->ioFolder, browseName, clientContext, channelNodeId);
        this->ioFolder.addItem(tmsClientChannel);
    }

    auto folderNodeIds = this->getChildNodes(client, inputsOutputsNodeId, OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_IOCOMPONENTTYPE));
    for (const auto& folderNodeId : folderNodeIds)
    {
        auto browseName = client->readBrowseName(folderNodeId);
        auto tmsClientFolder = TmsClientIoFolder(context, this->ioFolder, browseName, clientContext, folderNodeId);
        this->ioFolder.addItem(tmsClientFolder);
    }
}

void TmsClientDeviceImpl::findAndCreateStreamingOptions()
{
    this->streamingOptions.clear();
    auto streamingOptionsNodeId = getNodeId("StreamingOptions");
    try
    {
        auto streamingOptionNodeIds =
            this->getChildNodes(client, streamingOptionsNodeId, OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_VARIABLEBLOCKTYPE));
        for (const auto& streamingOptionNodeId : streamingOptionNodeIds)
        {
            auto browseName = client->readBrowseName(streamingOptionNodeId);
            auto clientStreamingInfo = TmsClientStreamingInfo(daqContext, browseName, clientContext, streamingOptionNodeId);
            this->streamingOptions.push_back(clientStreamingInfo);
        }
    }
    catch (const std::exception& e)
    {
        LOG_E("Failed to find 'StreamingOptions' OpcUa node: {}", e.what());
        // FIXME this is a temporary workaround for compability with legacy simulator
        if (isRootDevice)
        {
            auto streamingInfo = StreamingInfo("daq.wss");
            streamingInfo.addProperty(IntProperty("Port", 7414));
            this->streamingOptions.push_back(streamingInfo);
        }
    }
}

void TmsClientDeviceImpl::findAndCreateCustomComponents()
{
    auto componentId = OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_DAQCOMPONENTTYPE);
    auto folderNodeIds = this->getChildNodes(client, nodeId, componentId);
    for (const auto& folderNodeId : folderNodeIds)
    {
        auto browseName = client->readBrowseName(folderNodeId);
        if (detail::defaultComponents.count(browseName))
            continue;

        auto childComponents = this->getChildNodes(client, folderNodeId, componentId);
        if (childComponents.size())
        {
            this->components.push_back(TmsClientFolder(context, this->thisPtr<ComponentPtr>(), browseName, clientContext, folderNodeId));
        }
        else
        {
            this->components.push_back(TmsClientComponent(context, this->thisPtr<ComponentPtr>(), browseName, clientContext, folderNodeId));
        }
    }
}

FunctionBlockPtr TmsClientDeviceImpl::onAddFunctionBlock(const StringPtr& /*typeId*/, const PropertyObjectPtr& /*config*/)
{
    throw OpcUaClientCallNotAvailableException();
}

void TmsClientDeviceImpl::onRemoveFunctionBlock(const FunctionBlockPtr& /*functionBlock*/)
{
    throw OpcUaClientCallNotAvailableException();
}

void TmsClientDeviceImpl::setUpStreamings()
{
    auto self = this->borrowPtr<DevicePtr>();
    const auto signals = self.getSignalsRecursive();
    LOG_I("Device \"{}\" has established {} streaming connections", globalId, streamings.size());
    for (const auto& streaming : streamings)
    {
        LOG_I("Device \"{}\" adding signals to a streaming connection on url: {}", globalId, streaming.getConnectionString());
        streaming.addSignals(signals);
        streaming.setActive(true);
    }
}

void TmsClientDeviceImpl::connectToStreamings()
{
    if (createStreamingCallback.assigned())
    {
        for (const auto& option : streamingOptions)
        {
            StreamingPtr streaming;
            ErrCode errCode = wrapHandlerReturn(createStreamingCallback, streaming, option, isRootDevice);

            if (OPENDAQ_FAILED(errCode) || !streaming.assigned())
            {
                LOG_W("Device \"{}\" had not connected to published streaming protocol \"{}\".", globalId, option.getProtocolId());
            }
            else
            {
                streamings.push_back(streaming);
            }
        }
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
