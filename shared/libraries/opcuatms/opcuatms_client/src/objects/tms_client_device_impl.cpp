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
#include <open62541/daqbsp_nodeids.h>
#include "open62541/daqbt_nodeids.h"
#include <open62541/daqdevice_nodeids.h>
#include <open62541/types_daqdevice_generated.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/custom_log.h>
#include "opcuatms/core_types_utils.h"
#include "opcuatms/exceptions.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS
using namespace daq::opcua;

namespace detail
{
    static std::unordered_set<std::string> defaultComponents = {"Sig", "FB", "IO", "StreamingOptions"};

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
    std::map<uint32_t, DevicePtr> orderedDevices;
    std::vector<DevicePtr> unorderedDevices;

    const auto& references = getChildReferencesOfType(nodeId, OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_DAQDEVICETYPE));

    for (const auto& [browseName, ref] : references.byBrowseName)
    {
        auto subdeviceNodeId = OpcUaNodeId(ref->nodeId.nodeId);
        auto clientSubdevice = TmsClientDevice(context, devices, browseName, clientContext, subdeviceNodeId, createStreamingCallback);
                    
        auto numberInList = this->tryReadChildNumberInList(subdeviceNodeId);
        if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedDevices.count(numberInList))
            orderedDevices.insert(std::pair<uint32_t, ComponentPtr>(numberInList, clientSubdevice));
        else
            unorderedDevices.emplace_back(clientSubdevice);
    }

    for (const auto& val : orderedDevices)
        addSubDevice(val.second);
    for (const auto& val : unorderedDevices)
        addSubDevice(val);
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
        else if (browseName != "NumberInList")
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

    std::map<uint32_t, FunctionBlockPtr> orderedFunctionBlocks;
    std::vector<FunctionBlockPtr> unorderedFunctionBlocks;

    auto functionBlocksNodeId = getNodeId("FB");
    const auto& references = getChildReferencesOfType(functionBlocksNodeId, OpcUaNodeId(NAMESPACE_TMSBSP, UA_TMSBSPID_FUNCTIONBLOCKTYPE));

    for (const auto& [browseName, ref] : references.byBrowseName)
    {
        const auto functionBlockNodeId = OpcUaNodeId(ref->nodeId.nodeId);

        try
        {
            auto clientFunctionBlock = TmsClientFunctionBlock(context, this->functionBlocks, browseName, clientContext, functionBlockNodeId);
            const auto numberInList = this->tryReadChildNumberInList(functionBlockNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedFunctionBlocks.count(numberInList))
                orderedFunctionBlocks.insert(std::pair<uint32_t, FunctionBlockPtr>(numberInList, clientFunctionBlock));
            else
                unorderedFunctionBlocks.emplace_back(clientFunctionBlock);
        }
        catch(...)
        {
            // TODO: Log failure to add fb
        }
    }

    for (const auto& val : orderedFunctionBlocks)
        this->addNestedFunctionBlock(val.second);
    for (const auto& val : unorderedFunctionBlocks)
        this->addNestedFunctionBlock(val);
}

void TmsClientDeviceImpl::findAndCreateSignals()
{
    std::map<uint32_t, SignalPtr> orderedSignals;
    std::vector<SignalPtr> unorderedSignals;
    
    const auto signalsNodeId = getNodeId("Sig");
    const auto& references = getChildReferencesOfType(signalsNodeId, OpcUaNodeId(NAMESPACE_TMSBSP, UA_TMSBSPID_SIGNALTYPE));

    for (const auto& [signalNodeId, ref] : references.byNodeId)
    {
        auto clientSignal = FindOrCreateTmsClientSignal(context, signals, clientContext, signalNodeId);
        const auto numberInList = this->tryReadChildNumberInList(signalNodeId);
        if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedSignals.count(numberInList))
            orderedSignals.insert(std::pair<uint32_t, SignalPtr>(numberInList, clientSignal));
        else
            unorderedSignals.emplace_back(clientSignal);
    }
    
    for (const auto& val : orderedSignals)
        this->addSignal(val.second);
    for (const auto& val : unorderedSignals)
        this->addSignal(val);
}

void TmsClientDeviceImpl::findAndCreateInputsOutputs()
{
    std::map<uint32_t, ComponentPtr> orderedComponents;
    std::vector<ComponentPtr> unorderedComponents;

    this->ioFolder.clear();
    auto inputsOutputsNodeId = getNodeId("IO");
    const auto& channelreferences = getChildReferencesOfType(inputsOutputsNodeId, OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_CHANNELTYPE));

    for (const auto& [browseName, ref] : channelreferences.byBrowseName)
    {
        const auto channelNodeId = OpcUaNodeId(ref->nodeId.nodeId);
        auto tmsClientChannel = TmsClientChannel(context, this->ioFolder, browseName, clientContext, channelNodeId);

        auto numberInList = this->tryReadChildNumberInList(channelNodeId);
        if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedComponents.count(numberInList))
            orderedComponents.insert(std::pair<uint32_t, ComponentPtr>(numberInList, tmsClientChannel));
        else
            unorderedComponents.emplace_back(tmsClientChannel);
    }

    const auto& folderReferences = getChildReferencesOfType(inputsOutputsNodeId, OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_IOCOMPONENTTYPE));

    for (const auto& [browseName, ref] : folderReferences.byBrowseName)
    {
        const auto folderNodeId = OpcUaNodeId(ref->nodeId.nodeId);
        auto tmsClientFolder = TmsClientIoFolder(context, this->ioFolder, browseName, clientContext, folderNodeId);

        auto numberInList = this->tryReadChildNumberInList(folderNodeId);
        if (numberInList != std::numeric_limits<uint32_t>::max())
            orderedComponents.insert(std::pair<uint32_t, ComponentPtr>(numberInList, tmsClientFolder));
        else
            unorderedComponents.emplace_back(tmsClientFolder);
    }

    for (const auto& val : orderedComponents)
        this->ioFolder.addItem(val.second);
    for (const auto& val : unorderedComponents)
        this->ioFolder.addItem(val);
}

void TmsClientDeviceImpl::findAndCreateStreamingOptions()
{
    std::map<uint32_t, PropertyObjectPtr> orderedStreamings;
    std::vector<PropertyObjectPtr> unorderedStreamings;

    this->streamingOptions.clear();
    auto streamingOptionsNodeId = getNodeId("StreamingOptions");

    try
    {
        const auto& streamingOptionsReferences =
            getChildReferencesOfType(streamingOptionsNodeId, OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_VARIABLEBLOCKTYPE));

        for (const auto& [browseName, ref] : streamingOptionsReferences.byBrowseName)
        {
            const auto optionNodeId = OpcUaNodeId(ref->nodeId.nodeId);
            auto clientStreamingInfo = TmsClientStreamingInfo(daqContext, browseName, clientContext, optionNodeId);

            auto numberInList = this->tryReadChildNumberInList(optionNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max())
                orderedStreamings.insert(std::pair<uint32_t, PropertyObjectPtr>(numberInList, clientStreamingInfo));
            else
                unorderedStreamings.emplace_back(clientStreamingInfo);
        }
    }
    catch (const std::exception& e)
    {
        LOG_E("Failed to find 'StreamingOptions' OpcUa node: {}", e.what());
    }

    for (const auto& val : orderedStreamings)
        this->streamingOptions.push_back(val.second);
    for (const auto& val : unorderedStreamings)
        this->streamingOptions.push_back(val);
}

void TmsClientDeviceImpl::findAndCreateCustomComponents()
{
    std::map<uint32_t, ComponentPtr> orderedComponents;
    std::vector<ComponentPtr> unorderedComponents;

    auto componentId = OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_DAQCOMPONENTTYPE);
    const auto& folderReferences = getChildReferencesOfType(nodeId, componentId);

    for (const auto& [browseName, ref] : folderReferences.byBrowseName)
    {
        const auto folderNodeId = OpcUaNodeId(ref->nodeId.nodeId);

        if (detail::defaultComponents.count(browseName))
            continue;

        const auto& componentReferences = getChildReferencesOfType(folderNodeId, componentId);

        ComponentPtr child;
        if (!componentReferences.byNodeId.empty())
            child = TmsClientFolder(context, this->thisPtr<ComponentPtr>(), browseName, clientContext, folderNodeId);
        else
            child = TmsClientComponent(context, this->thisPtr<ComponentPtr>(), browseName, clientContext, folderNodeId);
    
        auto numberInList = this->tryReadChildNumberInList(folderNodeId);
        if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedComponents.count(numberInList))
            orderedComponents.insert(std::pair<uint32_t, ComponentPtr>(numberInList, child));
        else
            unorderedComponents.push_back(child);
    }

    for (const auto& val : orderedComponents)
        this->components.push_back(val.second);
    for (const auto& val : unorderedComponents)
        this->components.push_back(val);
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
