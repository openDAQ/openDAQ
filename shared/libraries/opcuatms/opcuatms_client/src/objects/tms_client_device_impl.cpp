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
#include "opcuatms_client/objects/tms_client_server_capability_factory.h"
#include <open62541/daqbsp_nodeids.h>
#include "open62541/daqbt_nodeids.h"
#include <open62541/daqdevice_nodeids.h>
#include <open62541/types_daqdevice_generated.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/custom_log.h>
#include "opcuatms/core_types_utils.h"
#include "opcuatms/exceptions.h"
#include "opcuatms/converters/list_conversion_utils.h"
#include "opcuatms/converters/property_object_conversion_utils.h"
#include <opcuatms_client/objects/tms_client_function_block_type_factory.h>
#include <opendaq/device_domain_factory.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS
using namespace daq::opcua;

namespace detail
{
    static std::unordered_set<std::string> defaultComponents = {"Sig", "FB", "IO", "ServerCapabilities"};

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
        {"SystemUUID", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setSystemUuid(v.toString()); }},
        {"OpenDaqPackageVersion",[](const DeviceInfoConfigPtr& info, const OpcUaVariant& v){ info.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("sdkVersion", v.toString()); }},
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
    clientContext->readObjectAttributes(nodeId);

    if (isRootDevice)
        clientContext->registerRootDevice(thisInterface());
    
    findAndCreateSubdevices();
    findAndCreateFunctionBlocks();
    findAndCreateSignals();
    findAndCreateInputsOutputs();
    findAndCreateCustomComponents();

    connectToStreamings();
    setUpStreamings();
}

ErrCode TmsClientDeviceImpl::getDomain(IDeviceDomain** deviceDomain)
{
    fetchTimeDomain();
    return TmsClientComponentBaseImpl<GenericDevice<IDevice, ITmsClientComponent>>::getDomain(deviceDomain);
}

void TmsClientDeviceImpl::findAndCreateSubdevices()
{
    std::map<uint32_t, DevicePtr> orderedDevices;
    std::vector<DevicePtr> unorderedDevices;

    const auto& references = getChildReferencesOfType(nodeId, OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_DAQDEVICETYPE));

    for (const auto& [browseName, ref] : references.byBrowseName)
    {
        try
        {
            auto subdeviceNodeId = OpcUaNodeId(ref->nodeId.nodeId);
            auto clientSubdevice = TmsClientDevice(context, devices, browseName, clientContext, subdeviceNodeId, createStreamingCallback);
                        
            auto numberInList = this->tryReadChildNumberInList(subdeviceNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedDevices.count(numberInList))
                orderedDevices.insert(std::pair<uint32_t, ComponentPtr>(numberInList, clientSubdevice));
            else
                unorderedDevices.emplace_back(clientSubdevice);
        }
        catch(...)
        {
            LOG_W("Failed to create subdevice \"{}\" in OpcUA client device \"{}\"", browseName, this->globalId);
        }
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
    auto deviceInfo = DeviceInfo("", "OpcUa Client");

    auto browseFilter = BrowseFilter();
    browseFilter.nodeClass = UA_NODECLASS_VARIABLE;
    const auto& references = clientContext->getReferenceBrowser()->browseFiltered(nodeId, browseFilter);

    auto reader = AttributeReader(client, clientContext->getMaxNodesPerRead());

    for (const auto& [browseName, ref] : references.byBrowseName)
        reader.addAttribute({ref->nodeId.nodeId, UA_ATTRIBUTEID_VALUE});

    reader.read();

    for (const auto& [browseName, ref] : references.byBrowseName)
    {
        const auto refNodeId = OpcUaNodeId(ref->nodeId.nodeId);
        const auto value = reader.getValue(refNodeId, UA_ATTRIBUTEID_VALUE);

        if (detail::deviceInfoSetterMap.count(browseName))
        {
            detail::deviceInfoSetterMap[browseName](deviceInfo, value);
            continue;
        }

        if (browseName == "NumberInList")
            continue;

        try
        {
            if (value.isScalar())
            {
                if (value.isString())
                    deviceInfo.addProperty(StringProperty(browseName, value.toString()));
                else if (value.isBool())
                    deviceInfo.addProperty(BoolProperty(browseName, value.toBool()));
                else if (value.isDouble())
                    deviceInfo.addProperty(FloatProperty(browseName, value.toDouble()));
                else if (value.isInteger())
                    deviceInfo.addProperty(IntProperty(browseName, value.toInteger()));
            }
        }
        catch (const std::exception& e)
        {
            LOG_W("Failed to read device info attribute on OpcUa client device \"{}\": {}", this->globalId, e.what());
        }
    }
    
    findAndCreateServerCapabilities(deviceInfo);

    deviceInfo.freeze();
    return deviceInfo;
}

void TmsClientDeviceImpl::fetchTimeDomain()
{
    auto timeDomainNodeId = getNodeId("Domain");
    auto variant = client->readValue(timeDomainNodeId);

    UA_DeviceDomainStructure* deviceDomain;
    deviceDomain = (UA_DeviceDomainStructure*) variant.getValue().data;
    if (!deviceDomain)
        return;

    if (deviceDomain == nullptr)
        return;
    
    auto numerator = deviceDomain->resolution.numerator;
    auto denominator = deviceDomain->resolution.denominator;
    if (denominator == 0)
        denominator = 1;
    const auto resolution = Ratio(numerator, denominator);
    const auto origin = ConvertToDaqCoreString(deviceDomain->origin);
    UnitPtr domainUnit;
    if (deviceDomain->unit.unitId > 0)
        domainUnit = Unit(ConvertToDaqCoreString(deviceDomain->unit.displayName.text),
                          deviceDomain->unit.unitId,
                          ConvertToDaqCoreString(deviceDomain->unit.description.text),
                          ConvertToDaqCoreString(deviceDomain->unit.quantity));
    else
        domainUnit = Unit("");

    setDeviceDomainNoCoreEvent(DeviceDomain(resolution, origin, domainUnit));
    ticksSinceOrigin = deviceDomain->ticksSinceOrigin;
}

void TmsClientDeviceImpl::fetchTicksSinceOrigin()
{
    auto timeDomainNodeId = getNodeId("Domain");
    auto variant = client->readValue(timeDomainNodeId);

    UA_DeviceDomainStructure* deviceDomain;
    deviceDomain = (UA_DeviceDomainStructure*) variant.getValue().data;
    ticksSinceOrigin = deviceDomain->ticksSinceOrigin;
}
uint64_t TmsClientDeviceImpl::onGetTicksSinceOrigin()
{
    fetchTicksSinceOrigin();
    return ticksSinceOrigin;
}

void TmsClientDeviceImpl::findAndCreateFunctionBlocks()
{

    std::map<uint32_t, FunctionBlockPtr> orderedFunctionBlocks;
    std::vector<FunctionBlockPtr> unorderedFunctionBlocks;

    auto functionBlocksNodeId = getNodeId("FB");
    const auto& references = getChildReferencesOfType(functionBlocksNodeId, OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_FUNCTIONBLOCKTYPE));

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
            LOG_W("Failed to create function block \"{}\" to OpcUA client device \"{}\"", browseName, this->globalId);
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
    const auto& references = getChildReferencesOfType(signalsNodeId, OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_SIGNALTYPE));

    for (const auto& [signalNodeId, ref] : references.byNodeId)
    {
        try
        {
            auto clientSignal = FindOrCreateTmsClientSignal(context, signals, clientContext, signalNodeId);
            const auto numberInList = this->tryReadChildNumberInList(signalNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedSignals.count(numberInList))
                orderedSignals.insert(std::pair<uint32_t, SignalPtr>(numberInList, clientSignal));
            else
                unorderedSignals.emplace_back(clientSignal);
        }
        catch (...)
        {
            LOG_W("Failed to find signal to OpcUA client device \"{}\"", this->globalId);
        }
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
    const auto& channelreferences = getChildReferencesOfType(inputsOutputsNodeId, OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_CHANNELTYPE));

    for (const auto& [browseName, ref] : channelreferences.byBrowseName)
    {
        try
        {
            const auto channelNodeId = OpcUaNodeId(ref->nodeId.nodeId);
            auto tmsClientChannel = TmsClientChannel(context, this->ioFolder, browseName, clientContext, channelNodeId);

            auto numberInList = this->tryReadChildNumberInList(channelNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedComponents.count(numberInList))
                orderedComponents.insert(std::pair<uint32_t, ComponentPtr>(numberInList, tmsClientChannel));
            else
                unorderedComponents.emplace_back(tmsClientChannel);
        }
        catch (...)
        {
            LOG_W("Failed to find channel \"{}\" to OpcUA client device \"{}\"", browseName, this->globalId);
        }
    }

    const auto& folderReferences = getChildReferencesOfType(inputsOutputsNodeId, OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_IOCOMPONENTTYPE));

    for (const auto& [browseName, ref] : folderReferences.byBrowseName)
    {
        try
        {
            const auto folderNodeId = OpcUaNodeId(ref->nodeId.nodeId);
            auto tmsClientFolder = TmsClientIoFolder(context, this->ioFolder, browseName, clientContext, folderNodeId);

            auto numberInList = this->tryReadChildNumberInList(folderNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max())
                orderedComponents.insert(std::pair<uint32_t, ComponentPtr>(numberInList, tmsClientFolder));
            else
                unorderedComponents.emplace_back(tmsClientFolder);
        }
        catch (...)
        {
            LOG_W("Failed to find io folder \"{}\" to OpcUA client device \"{}\"", browseName, this->globalId);
        }
    }

    for (const auto& val : orderedComponents)
        this->ioFolder.addItem(val.second);
    for (const auto& val : unorderedComponents)
        this->ioFolder.addItem(val);
}

void TmsClientDeviceImpl::findAndCreateServerCapabilities(const DeviceInfoPtr& deviceInfo)
{
    std::map<uint32_t, PropertyObjectPtr> orderedCaps;
    std::vector<PropertyObjectPtr> unorderedCaps;

    auto serverCapabilitiesNodeId = getNodeId("ServerCapabilities");

    try
    {
        const auto& serverCapabilitiesReferences =
            getChildReferencesOfType(serverCapabilitiesNodeId, OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_VARIABLEBLOCKTYPE));

        for (const auto& [browseName, ref] : serverCapabilitiesReferences.byBrowseName)
        {
            const auto optionNodeId = OpcUaNodeId(ref->nodeId.nodeId);
            auto clientServerCapability = TmsClientServerCapability(daqContext, browseName, "", clientContext, optionNodeId);

            auto capabilityCopy = ServerCapability("", "", ProtocolType::Unknown);
            for (const auto& prop : clientServerCapability.getAllProperties())
            {
                const auto name = prop.getName();
                if (!capabilityCopy.hasProperty(name))
                    capabilityCopy.addProperty(prop.asPtr<IPropertyInternal>().clone());
                capabilityCopy.setPropertyValue(name, clientServerCapability.getPropertyValue(name));
            }

            auto numberInList = this->tryReadChildNumberInList(optionNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max())
                orderedCaps.insert(std::pair<uint32_t, PropertyObjectPtr>(numberInList, capabilityCopy));
            else
                unorderedCaps.emplace_back(capabilityCopy);
        }
    }
    catch (const std::exception& e)
    {
        LOG_W("Failed to find 'ServerCapabilities' OpcUA node on OpcUA client device \"{}\": {}", this->globalId, e.what());
    }

    auto deviceInfoInternal = deviceInfo.asPtr<IDeviceInfoInternal>();
    deviceInfoInternal.clearServerStreamingCapabilities();
    for (const auto& [_, val] : orderedCaps)
        deviceInfoInternal.addServerCapability(val);
    for (const auto& val : unorderedCaps)
        deviceInfoInternal.addServerCapability(val);
}

void TmsClientDeviceImpl::findAndCreateCustomComponents()
{
    std::map<uint32_t, ComponentPtr> orderedComponents;
    std::vector<ComponentPtr> unorderedComponents;

    auto componentId = OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_DAQCOMPONENTTYPE);
    const auto& folderReferences = getChildReferencesOfType(nodeId, componentId);

    for (const auto& [browseName, ref] : folderReferences.byBrowseName)
    {
        try
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
        catch (...)
        {
            LOG_W("Failed to find channel \"{}\" to OpcUA client device \"{}\"", browseName, this->globalId);
        }
    }

    for (const auto& val : orderedComponents)
        this->components.push_back(val.second);
    for (const auto& val : unorderedComponents)
        this->components.push_back(val);
}

DictPtr<IString, IFunctionBlockType> TmsClientDeviceImpl::onGetAvailableFunctionBlockTypes()
{
    auto browser = clientContext->getReferenceBrowser();
    auto types = Dict<IString, IFunctionBlockType>();

    const auto fbFolderNodeId = browser->getChildNodeId(nodeId, "FB");

    if (!browser->hasReference(fbFolderNodeId, "AvailableTypes"))
        return types;

    const auto availableTypesId = browser->getChildNodeId(fbFolderNodeId, "AvailableTypes");

    auto filter = BrowseFilter();
    filter.direction = UA_BROWSEDIRECTION_FORWARD;
    filter.referenceTypeId = OpcUaNodeId(UA_NS0ID_HASPROPERTY);
    filter.nodeClass = UA_NODECLASS_VARIABLE;

    auto fbTypesReferences = browser->browseFiltered(availableTypesId, filter);

    for (const auto& [refNodeId, ref] : fbTypesReferences.byNodeId)
    {
        auto tmsFbType = TmsClientFunctionBlockType(daqContext, clientContext, refNodeId);
        types.set(tmsFbType.getId(), tmsFbType);
    }

    return types;
}

FunctionBlockPtr TmsClientDeviceImpl::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    const auto fbFolderNodeId = getNodeId("FB");
    const auto methodNodeId = clientContext->getReferenceBrowser()->getChildNodeId(fbFolderNodeId, "Add");

    const auto typeIdVariant = OpcUaVariant(typeId.toStdString().c_str());
    const auto configVariant = PropertyObjectConversionUtils::ToDictVariant(config);

    auto request = OpcUaCallMethodRequest();
    request->objectId = fbFolderNodeId.copyAndGetDetachedValue();
    request->methodId = methodNodeId.copyAndGetDetachedValue();
    request->inputArgumentsSize = 2;
    request->inputArguments = (UA_Variant*) UA_Array_new(request->inputArgumentsSize, &UA_TYPES[UA_TYPES_VARIANT]);

    request->inputArguments[0] = typeIdVariant.copyAndGetDetachedValue();
    request->inputArguments[1] = configVariant.copyAndGetDetachedValue();

    auto response = this->client->callMethod(request);

    if (response->statusCode != UA_STATUSCODE_GOOD)
        throw OpcUaException(response->statusCode, "Failed to add function block");

    assert(response->outputArgumentsSize == 2);
    const auto fbNodeId = OpcUaVariant(response->outputArguments[0]).toNodeId();
    const auto localId = OpcUaVariant(response->outputArguments[1]).toString();

    auto clientFunctionBlock = TmsClientFunctionBlock(context, this->functionBlocks, localId, clientContext, fbNodeId);
    addNestedFunctionBlock(clientFunctionBlock);
    return clientFunctionBlock;
}

void TmsClientDeviceImpl::onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock)
{
    const auto fbFolderNodeId = getNodeId("FB");
    const auto methodNodeId = clientContext->getReferenceBrowser()->getChildNodeId(fbFolderNodeId, "Remove");

    const auto fbIdVariant = OpcUaVariant(functionBlock.getLocalId().toStdString().c_str());

    auto request = OpcUaCallMethodRequest();
    request->objectId = fbFolderNodeId.copyAndGetDetachedValue();
    request->methodId = methodNodeId.copyAndGetDetachedValue();
    request->inputArgumentsSize = 1;
    request->inputArguments = (UA_Variant*) UA_Array_new(request->inputArgumentsSize, &UA_TYPES[UA_TYPES_VARIANT]);
    request->inputArguments[0] = fbIdVariant.copyAndGetDetachedValue();

    auto response = this->client->callMethod(request);

    if (response->statusCode != UA_STATUSCODE_GOOD)
        throw OpcUaException(response->statusCode, "Failed to remove function block");

    removeNestedFunctionBlock(functionBlock);
}

void TmsClientDeviceImpl::setUpStreamings()
{
    auto self = this->borrowPtr<DevicePtr>();
    const auto signals = self.getSignals(search::Recursive(search::Any()));
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
    DeviceInfoPtr info;
    this->getInfo(&info);
    if (createStreamingCallback.assigned())
    {
        for (const auto& capability : info.getServerCapabilities())
        {
            if (capability.getProtocolType() != ProtocolType::Streaming)
                continue;

            StreamingPtr streaming;
            ErrCode errCode = wrapHandlerReturn(createStreamingCallback, streaming, capability, isRootDevice);

            if (OPENDAQ_FAILED(errCode) || !streaming.assigned())
            {
                LOG_W("Device \"{}\" had not connected to published streaming protocol \"{}\".", globalId, capability.getPropertyValue("protocolId").asPtr<IString>());
            }
            else
            {
                streamings.push_back(streaming);
            }
        }
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
