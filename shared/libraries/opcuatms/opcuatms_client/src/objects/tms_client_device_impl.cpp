#include <opcuatms_client/objects/tms_client_device_impl.h>
#include <coreobjects/unit_factory.h>
#include <opcuaclient/browse_request.h>
#include <opcuaclient/browser/opcuabrowser.h>
#include <opcuatms_client/objects/tms_client_channel_factory.h>
#include <opcuatms_client/objects/tms_client_device_factory.h>
#include <opcuatms_client/objects/tms_client_folder_factory.h>
#include <opcuatms_client/objects/tms_client_function_block_factory.h>
#include <opcuatms_client/objects/tms_client_property_object_factory.h>
#include <opcuatms_client/objects/tms_client_signal_factory.h>
#include <opcuatms_client/objects/tms_client_component_factory.h>
#include <opcuatms_client/objects/tms_client_io_folder_factory.h>
#include <opcuatms_client/objects/tms_client_server_capability_factory.h>
#include <opcuatms_client/objects/tms_client_sync_component_factory.h>
#include <opcuatms_client/objects/tms_client_property_factory.h>
#include <open62541/daqbsp_nodeids.h>
#include <open62541/daqbt_nodeids.h>
#include <open62541/daqdevice_nodeids.h>
#include <open62541/types_daqdevice_generated.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/custom_log.h>
#include <opcuatms/core_types_utils.h>
#include <opcuatms/exceptions.h>
#include <opcuatms/converters/list_conversion_utils.h>
#include <opcuatms/converters/property_object_conversion_utils.h>
#include <opcuatms_client/objects/tms_client_function_block_type_factory.h>
#include <opendaq/device_domain_factory.h>
#include <opendaq/mirrored_device_ptr.h>
#include <opendaq/address_info_factory.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS
using namespace daq::opcua;

namespace detail
{
    static std::unordered_set<std::string> defaultComponents = {"Sig", "FB", "IO", "ServerCapabilities", "Synchronization"};

    static std::unordered_map<std::string, std::string> deviceInfoFieldMap = 
    {
        {"AssetId", "assetId"},
        {"ComponentName", "name"},
        {"DeviceClass", "deviceClass"},
        {"DeviceManual", "deviceManual"},
        {"DeviceRevision", "deviceRevision"},
        {"HardwareRevision", "hardwareRevision"},
        {"Manufacturer", "manufacturer"},
        {"ManufacturerUri", "manufacturerUri"},
        {"Model", "model"},
        {"ProductCode", "productCode"},
        {"ProductInstanceUri", "productInstanceUri"},
        {"RevisionCounter", "revisionCounter"},
        {"SerialNumber", "serialNumber"},
        {"SoftwareRevision", "softwareRevision"},
        {"MacAddress", "macAddress"},
        {"ParentMacAddress", "parentMacAddress"},
        {"Platform", "platform"},
        {"Position", "position"},
        {"SystemType", "systemType"},
        {"SystemUUID", "systemUuid"},
        {"OpenDaqPackageVersion", "sdkVersion"},
        {"Location", "location"},
        {"UserName", "userName"},
    };

    static std::unordered_map<std::string, std::function<void(const DeviceInfoConfigPtr&, const OpcUaVariant&)>> deviceInfoSetterMap = 
    {
        {"AssetId", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setAssetId(v.toString()); }},
        {"ComponentName", [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v) { info.setName(v.toString()); }},
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
        {"OpenDaqPackageVersion",
         [](const DeviceInfoConfigPtr& info, const OpcUaVariant& v)
         { info.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("sdkVersion", v.toString()); }},
    };
    }

TmsClientDeviceImpl::TmsClientDeviceImpl(const ContextPtr& ctx,
                                         const ComponentPtr& parent,
                                         const StringPtr& localId,
                                         const TmsClientContextPtr& clientContext,
                                         const opcua::OpcUaNodeId& nodeId,
                                         bool isRootDevice)
    : TmsClientComponentBaseImpl(ctx, parent, localId, clientContext, nodeId)
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
    findAndCreateSyncComponent();
    findAndCreateProporties();
}

ErrCode TmsClientDeviceImpl::getDomain(IDeviceDomain** deviceDomain)
{
    if (this->isComponentRemoved)
        return OPENDAQ_ERR_COMPONENT_REMOVED;

    fetchTimeDomain();
    return Super::getDomain(deviceDomain);
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
            auto clientSubdevice = TmsClientDevice(context, devices, browseName, clientContext, subdeviceNodeId);
                        
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

static bool IsVersionHigher(const std::string &version, int major, int minor)
{
    int majorVersion = 0, minorVersion = 0;
    std::stringstream ss(version);
    ss >> majorVersion;
    ss.ignore();
    ss >> minorVersion;
    return majorVersion > major || (majorVersion == major && minorVersion >= minor);
}

DeviceInfoPtr TmsClientDeviceImpl::onGetInfo()
{
    auto browseFilter = BrowseFilter();
    browseFilter.nodeClass = UA_NODECLASS_VARIABLE;
    const auto& references = clientContext->getReferenceBrowser()->browseFiltered(nodeId, browseFilter);

    auto reader = AttributeReader(client, clientContext->getMaxNodesPerRead());

    for (const auto& [browseName, ref] : references.byBrowseName)
    {
        reader.addAttribute({ref->nodeId.nodeId, UA_ATTRIBUTEID_VALUE});
        reader.addAttribute({ref->nodeId.nodeId, UA_ATTRIBUTEID_ACCESSLEVEL});
    }
    reader.read();

    bool serverSupportsEditableProperties = true;
    if (references.byBrowseName.contains("OpenDaqPackageVersion"))
    {
        const auto refNodeId = OpcUaNodeId(references.byBrowseName.at("OpenDaqPackageVersion")->nodeId.nodeId);
        const auto sdkVersion = reader.getValue(refNodeId, UA_ATTRIBUTEID_VALUE).toString();
        serverSupportsEditableProperties = sdkVersion.empty() || IsVersionHigher(sdkVersion, 3, 11);
    }

    auto changeableProperties = List<IString>();
    if (serverSupportsEditableProperties)
    {
        for (const auto& [browseName, ref] : references.byBrowseName)
        {
            const auto refNodeId = OpcUaNodeId(ref->nodeId.nodeId);
            const auto accessLevel = reader.getValue(refNodeId, UA_ATTRIBUTEID_ACCESSLEVEL).toInteger();
            const bool isReadOnly = !(accessLevel & UA_ACCESSLEVELMASK_WRITE);

            if (isReadOnly)
                continue;
            if (browseName == "NumberInList")
                continue;
            if (browseName == "Active")
                continue;
            if (browseName == "Visible")
                continue;
            if (browseName == "Tags")
                continue;
            
            std::string propertyName = browseName;
            if (detail::deviceInfoFieldMap.count(propertyName))
                propertyName = detail::deviceInfoFieldMap[propertyName];

            deviceInfoChangeableFields.emplace(propertyName, refNodeId);
        }
        for (const auto& [name, _] : deviceInfoChangeableFields)
            changeableProperties.pushBack(String(name));
    }
    else
    {
        changeableProperties = {"userName", "location"};
    }

    auto deviceInfo = DeviceInfoWithChanegableFields(changeableProperties);
    deviceInfo.setName(this->client->readDisplayName(this->nodeId));

    for (const auto& [browseName, ref] : references.byBrowseName)
    {
        const auto refNodeId = OpcUaNodeId(ref->nodeId.nodeId);
        const auto value = reader.getValue(refNodeId, UA_ATTRIBUTEID_VALUE);

        if (!value.isScalar())
            continue;
        if (browseName == "NumberInList")
            continue;
        if (browseName == "Active")
            continue;
        if (browseName == "Visible")
            continue;
        if (browseName == "Tags")
            continue;
       
        try
        {
            if (auto it = detail::deviceInfoSetterMap.find(browseName); it != detail::deviceInfoSetterMap.end())
            {
                it->second(deviceInfo, value);
                continue;
            }

            std::string propertyName = browseName;
            if (auto it = detail::deviceInfoFieldMap.find(propertyName); it != detail::deviceInfoFieldMap.end())
                propertyName = it->second;
            
            if (deviceInfo.hasProperty(propertyName))
            {
                if (value.isString())
                    deviceInfo.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propertyName, value.toString());
                else if (value.isBool())
                    deviceInfo.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propertyName, value.toBool());
                else if (value.isDouble())
                    deviceInfo.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propertyName, value.toDouble());
                else if (value.isInteger())
                    deviceInfo.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propertyName, value.toInteger());
            }
            else
            {
                PropertyBuilderPtr propertyBuilder;
                if (value.isString())
                    propertyBuilder = StringPropertyBuilder(propertyName, value.toString());
                else if (value.isBool())
                    propertyBuilder = BoolPropertyBuilder(propertyName, value.toBool());
                else if (value.isDouble())
                    propertyBuilder = FloatPropertyBuilder(propertyName, value.toDouble());
                else if (value.isInteger())
                    propertyBuilder = IntPropertyBuilder(propertyName, value.toInteger());

                if (propertyBuilder.assigned())
                {
                    const bool isReadOnly = deviceInfoChangeableFields.count(propertyName) == 0;
                    deviceInfo.addProperty(propertyBuilder.setReadOnly(isReadOnly).build());
                }
            }
        }
        catch (const std::exception& e)
        {
            LOG_W("Failed to read device info attribute on OpcUa client device \"{}\": {}", this->globalId, e.what());
        }
    }

    findAndCreateServerCapabilities(deviceInfo);
    return deviceInfo;
}

ErrCode TmsClientDeviceImpl::addProperty(IProperty* property)
{
    if (property == nullptr)
        return OPENDAQ_ERR_INVALID_ARGUMENT;
    
    auto propPtr = PropertyPtr::Borrow(property);
    if (deviceInfoChangeableFields.count(propPtr.getName()))
        return Impl::addProperty(property);

    return Super::addProperty(property);
}

ErrCode TmsClientDeviceImpl::getPropertyValue(IString* propertyName, IBaseObject** value)
{
    if (propertyName == nullptr)
    {
        LOG_W("Failed to get value for property with nullptr name on OpcUA client property object");
        return OPENDAQ_SUCCESS;
    }
    auto propertyNamePtr = StringPtr::Borrow(propertyName);

    if (auto it = deviceInfoChangeableFields.find(propertyNamePtr); it != deviceInfoChangeableFields.end())
    {
        const auto variant = client->readValue(it->second);
        auto object = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
        Impl::setProtectedPropertyValue(propertyName, object);
        *value = object.detach();
        return OPENDAQ_SUCCESS;
    }

    return Super::getPropertyValue(propertyName, value);
}

ErrCode TmsClientDeviceImpl::setPropertyValueInternal(IString* propertyName, IBaseObject* value, bool protectedWrite)
{
    if (propertyName == nullptr)
    {
        LOG_W("Failed to set value for property with nullptr name on OpcUA client property object");
        return OPENDAQ_SUCCESS;
    }
    auto propertyNamePtr = StringPtr::Borrow(propertyName);

    if (auto it = deviceInfoChangeableFields.find(propertyNamePtr); it != deviceInfoChangeableFields.end())
    {
        PropertyPtr prop;
        ErrCode errCode = getProperty(propertyName, &prop);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        if (!protectedWrite && prop.getReadOnly())
            return OPENDAQ_ERR_ACCESSDENIED;

        auto valuePtr = BaseObjectPtr(value);
        const auto ct = prop.getValueType();
        const auto valueCt = valuePtr.getCoreType();
        if (ct != valueCt)
            valuePtr = valuePtr.convertTo(ct);

        const auto variant = VariantConverter<IBaseObject>::ToVariant(valuePtr, nullptr, daqContext);
        client->writeValue(it->second, variant);
        return OPENDAQ_SUCCESS;
    }

    return Super::setPropertyValueInternal(propertyName, value, protectedWrite);
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

void TmsClientDeviceImpl::findAndCreateSyncComponent()
{
    this->removeComponentById("Synchronization");
    auto syncComponentNodeId = getNodeId("Synchronization");
    syncComponent = this->addExistingComponent(TmsClientSyncComponent(context,
                                       this->thisPtr<ComponentPtr>(),
                                       "Synchronization",
                                       clientContext,
                                       syncComponentNodeId));
}

void TmsClientDeviceImpl::findAndCreateProporties()
{
    if (auto it = this->introspectionVariableIdMap.find("UserName"); it != this->introspectionVariableIdMap.end())
    {
        introspectionVariableIdMap.emplace("userName", it->second);
    }
    
    if (auto it = this->introspectionVariableIdMap.find("Location"); it != this->introspectionVariableIdMap.end())
    {
        introspectionVariableIdMap.emplace("location", it->second);
    }
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

DictPtr<IString, IDeviceType> TmsClientDeviceImpl::onGetAvailableDeviceTypes()
{
    return Dict<IString, IDeviceType>();
}

PropertyObjectPtr TmsClientDeviceImpl::onCreateDefaultAddDeviceConfig()
{
    return PropertyObject();
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
            auto clientServerCapability = TmsClientPropertyObject(daqContext, clientContext, optionNodeId);

            auto capabilityCopy = ServerCapability("", "", ProtocolType::Unknown);
            for (const auto& prop : clientServerCapability.getAllProperties())
            {
                const auto name = prop.getName();
                if (!capabilityCopy.hasProperty(name))
                    capabilityCopy.addProperty(prop.asPtr<IPropertyInternal>().clone());

                // AddressInfo is a special case, add it as a child object property of type IAddressInfo
                if (name == "AddressInfo")
                {
                    const auto addrInfoId = clientContext->getReferenceBrowser()->getChildNodeId(optionNodeId, "AddressInfo");
                    const auto& addrInfoRefs = getChildReferencesOfType(addrInfoId, OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_VARIABLEBLOCKTYPE));
                    
                    for (const auto& [addrInfoBrowseName, addrInfoRef] : addrInfoRefs.byBrowseName)
                    {
                        auto clientAddressInfo = TmsClientPropertyObject(daqContext, clientContext, OpcUaNodeId(addrInfoRef->nodeId.nodeId));
                        auto addrInfoCopy = AddressInfo();
                        for (const auto& addrProp : clientAddressInfo.getAllProperties())
                        {
                            const auto addrName = addrProp.getName();
                            if (!addrInfoCopy.hasProperty(addrName))
                                addrInfoCopy.addProperty(addrProp.asPtr<IPropertyInternal>().clone());
                            addrInfoCopy.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(addrName, clientAddressInfo.getPropertyValue(addrName));
                        }

                        capabilityCopy.addAddressInfo(addrInfoCopy);
                    }
                }
                else
                {
                    capabilityCopy.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(name, clientServerCapability.getPropertyValue(name));
                }
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

void TmsClientDeviceImpl::removed()
{
    if (this->clientContext->getRootDevice() == this->thisPtr<DevicePtr>())
    {
        this->client->disconnect(false);
    }

    Super::removed();
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

    auto fbSignals = clientFunctionBlock.getSignals(search::Recursive(search::Any()));
    auto deviceStreamingSources = this->thisPtr<MirroredDevicePtr>().getStreamingSources();
    for (const auto& streaming : deviceStreamingSources)
    {
        streaming.addSignals(fbSignals);
    }
    if (deviceStreamingSources.getCount() > 0)
    {
        for (const auto& signal : fbSignals)
        {
            if (signal.getPublic())
                signal.asPtr<IMirroredSignalConfig>().setActiveStreamingSource(deviceStreamingSources[0].getConnectionString());
        }
    }

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

ListPtr<ILogFileInfo> TmsClientDeviceImpl::onGetLogFileInfos()
{
    throw OpcUaClientCallNotAvailableException("getLogFileInfo is not available for OpcUA client device");
}

StringPtr TmsClientDeviceImpl::onGetLog(const StringPtr& id, Int size, Int offset)
{
    throw OpcUaClientCallNotAvailableException("GetLog is not available for OpcUA client device");
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
