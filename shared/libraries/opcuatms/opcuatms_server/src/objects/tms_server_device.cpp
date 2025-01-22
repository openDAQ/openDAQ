#include <iostream>
#include <string>
#include <set>
#include <stdexcept>
#include <opendaq/device_ptr.h>
#include <opendaq/device_info_internal_ptr.h>
#include <opcuatms_server/objects/tms_server_device.h>
#include <opcuatms/core_types_utils.h>
#include <opcuatms/type_mappings.h>
#include <open62541/daqdevice_nodeids.h>
#include <opcuatms/converters/struct_converter.h>
#include <opendaq/component_ptr.h>
#include <opendaq/device_private.h>
#include <opendaq/search_filter_factory.h>
#include <open62541/types_daqesp_generated.h>
#include <opcuatms/converters/variant_converter.h>
#include <coreobjects/property_object_factory.h>
#include <opcuatms/converters/property_object_conversion_utils.h>
#include <opcuatms/converters/list_conversion_utils.h>
#include <opcuatms_server/objects/tms_server_function_block_type.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

namespace detail
{
    static OpcUaVariant createLocalizedTextVariant(ConstCharPtr text)
    {
        OpcUaVariant v;
        OpcUaObject localizedText = UA_LOCALIZEDTEXT_ALLOC("", text);
        v.setScalar<UA_LocalizedText>(std::move(localizedText.getValue()));
        return v;
    }

    static std::unordered_map<std::string, std::string> componentFieldToDeviceInfo = {
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
    };
    
    static std::unordered_map<std::string, std::function<OpcUaVariant(const DeviceInfoPtr&)>> componentFieldToVariant = {
        {"ComponentName", [](const DeviceInfoPtr& info) { return createLocalizedTextVariant(info.getName().getCharPtr()); }},
        {"Manufacturer", [](const DeviceInfoPtr& info) { return createLocalizedTextVariant(info.getManufacturer().getCharPtr()); }},
        {"Model", [](const DeviceInfoPtr& info) { return createLocalizedTextVariant(info.getModel().getCharPtr()); }},
        {"RevisionCounter", [](const DeviceInfoPtr& info) { return OpcUaVariant{static_cast<int>(info.getRevisionCounter())}; }},
        {"Position", [](const DeviceInfoPtr& info) { return OpcUaVariant{static_cast<uint16_t>(info.getPosition())}; }},
    };
}

TmsServerDevice::TmsServerDevice(const DevicePtr& object, const OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext)
    : Super(object, server, context, tmsContext)
{
}

OpcUaNodeId TmsServerDevice::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_DAQDEVICETYPE);
}

bool TmsServerDevice::createOptionalNode(const OpcUaNodeId& nodeId)
{
    const auto name = server->readBrowseNameString(nodeId);

    if (name == "AssetId" && object.getInfo().getAssetId() != "")
        return true;
    if (name == "ComponentName" && object.getInfo().getName() != "")
        return true;
    if (name == "DeviceClass" && object.getInfo().getDeviceClass() != "")
        return true;
    if (name == "ManufacturerUri" && object.getInfo().getManufacturerUri() != "")
        return true;
    if (name == "ProductCode" && object.getInfo().getProductCode() != "")
        return true;
    if (name == "ProductInstanceUri" && object.getInfo().getProductInstanceUri() != "")
        return true;
    if (name == "Synchronization" && object.getSyncComponent().assigned())
        return true;

    return Super::createOptionalNode(nodeId);
}

void TmsServerDevice::bindCallbacks()
{
    this->addReadCallback("Domain", [this]
    {
        const auto deviceDomain = object.getDomain();
        if (!deviceDomain.assigned())
            return OpcUaVariant{};

        const auto functionBlockNodeId = getChildNodeId("Domain");
        try
        {
            OpcUaObject<UA_DeviceDomainStructure> uaDeviceDomain;
            uaDeviceDomain->resolution.numerator = deviceDomain.getTickResolution().getNumerator();
            uaDeviceDomain->resolution.denominator = deviceDomain.getTickResolution().getDenominator();
            uaDeviceDomain->origin = ConvertToOpcUaString(deviceDomain.getOrigin()).getDetachedValue();
            uaDeviceDomain->ticksSinceOrigin = object.getTicksSinceOrigin();
            auto unit = StructConverter<IUnit, UA_EUInformationWithQuantity>::ToTmsType(deviceDomain.getUnit());
            uaDeviceDomain->unit = unit.getDetachedValue();

            OpcUaVariant v;
            v.setScalar(*uaDeviceDomain);
            return v;
        }
        catch (...)
        {
            return OpcUaVariant{};
        }
    });

    Super::bindCallbacks();
}

void TmsServerDevice::populateDeviceInfo()
{
    auto deviceInfo = object.getInfo();

    auto createNode = [this](std::string name, CoreType type)
    {
        OpcUaNodeId newNodeId(0);
        AddVariableNodeParams params(newNodeId, nodeId);
        params.setBrowseName(name);
        switch (type)
        {
            case ctBool:
                params.setDataType(OpcUaNodeId(UA_TYPES[UA_TYPES_BOOLEAN].typeId));
                break;
            case ctInt:
                params.setDataType(OpcUaNodeId(UA_TYPES[UA_TYPES_INT64].typeId));
                break;
            case ctFloat:
                params.setDataType(OpcUaNodeId(UA_TYPES[UA_TYPES_DOUBLE].typeId));
                break;
            case ctString:
                params.setDataType(OpcUaNodeId(UA_TYPES[UA_TYPES_STRING].typeId));
                break;
            default:
                throw;
        }
        
        params.typeDefinition = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));
        server->addVariableNode(params);
    };

    createNode("OpenDaqPackageVersion", ctString);

    std::unordered_set<std::string> customInfoNamesSet;
    for (auto propName : deviceInfo.getCustomInfoPropertyNames())
    {
        try
        {
            auto prop = deviceInfo.getProperty(propName);
            createNode(propName, prop.getValueType());
            customInfoNamesSet.insert(propName);
        }
        catch(...)
        {
        }
    }

    OpcUaObject<UA_BrowseDescription> bd;
    bd->nodeId = nodeId.copyAndGetDetachedValue();
    bd->resultMask = UA_BROWSERESULTMASK_ALL;
    auto result = server->browse(bd);

    for (size_t i = 0; i < result->referencesSize; i++)
    {
        const auto& reference = result->references[i];
        std::string browseName = opcua::utils::ToStdString(reference.browseName.name);

        std::string propName;
        if (const auto it = detail::componentFieldToDeviceInfo.find(browseName); it != detail::componentFieldToDeviceInfo.end())
            propName = it->second;
        else if (customInfoNamesSet.count(browseName))
            propName = browseName;
        else
            continue;

        const auto & nodeId = reference.nodeId.nodeId;
        const auto prop = deviceInfo.getProperty(propName);

        if (prop.getReadOnly())
        {
            server->setAccessLevel(nodeId, UA_ACCESSLEVELMASK_READ);
            OpcUaVariant value;
            if (const auto it = detail::componentFieldToVariant.find(browseName); it != detail::componentFieldToVariant.end())
            {
                value = it->second(deviceInfo);
            }
            else
            {
                const auto daqValue = deviceInfo.getPropertyValue(propName);
                value = VariantConverter<IBaseObject>::ToVariant(daqValue, nullptr, daqContext);
            }
            server->writeValue(nodeId, *value);
            continue;
        }

        server->setAccessLevel(nodeId, UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);

        if (const auto it = detail::componentFieldToVariant.find(browseName); it != detail::componentFieldToVariant.end())
            this->addReadCallback(nodeId, std::bind(it->second, deviceInfo));
        else
            this->addReadCallback(nodeId, [this, name = propName]
            {
                const auto value = object.getInfo().getPropertyValue(name);
                return VariantConverter<IBaseObject>::ToVariant(value, nullptr, daqContext);
            });

        this->addWriteCallback(nodeId, [this, name = propName](const OpcUaVariant& variant)
        {
            const auto value = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
            this->object.getInfo().setPropertyValue(name, value);
            return UA_STATUSCODE_GOOD;
        });
    }

    std::map<std::string, std::string> theBestProportiesEver = 
    {
        {"userName", "UserName"},
        {"location", "Location"}
    };

    for (const auto& [propName, browseName] : theBestProportiesEver)
    {
        const auto& prop = deviceInfo.getProperty(propName);
        const auto nodeId = getChildNodeId(browseName);
        auto tmsProperty = std::make_shared<TmsServerProperty>(prop, server, daqContext, tmsContext, browseName);
        tmsProperty->registerToExistingOpcUaNode(nodeId);
        deviceInfoProperties.push_back(tmsProperty);

        this->addReadCallback(nodeId, [this, name = propName]
        {
            const auto value = object.getInfo().getPropertyValue(name);
            return VariantConverter<IBaseObject>::ToVariant(value, nullptr, daqContext);
        });

        if (!prop.getReadOnly())
        {
            this->addWriteCallback(nodeId, [this, name = propName](const OpcUaVariant& variant)
            {
                const auto value = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
                this->object.getInfo().setPropertyValue(name, value);
                return UA_STATUSCODE_GOOD;
            });
        }
    }
}

void TmsServerDevice::populateServerCapabilities()
{
    const auto deviceInfo = object.getInfo();
    if (deviceInfo == nullptr)
        return;

    const PropertyObjectPtr serverCapabilitiesObj = deviceInfo.getPropertyValue("serverCapabilities"); 

    auto tmsServerCapability = registerTmsObjectOrAddReference<TmsServerPropertyObject>(
            nodeId, serverCapabilitiesObj.asPtr<IPropertyObject>(), numberInList++, "ServerCapabilities");
    this->serverCapabilities.push_back(std::move(tmsServerCapability));
}

void TmsServerDevice::addFunctionBlockFolderNodes()
{
    auto fbNodeId = getChildNodeId("FB");

    createFunctionBlockTypesFolder(fbNodeId);
    createAddFunctionBlockNode(fbNodeId);
    createRemoveFunctionBlockNode(fbNodeId);
}

void TmsServerDevice::createFunctionBlockTypesFolder(const OpcUaNodeId& parentId)
{
    OpcUaNodeId nodeIdOut;
    AddObjectNodeParams params(nodeIdOut, parentId);
    params.referenceTypeId = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));
    params.typeDefinition = OpcUaNodeId(UA_NS0ID_FOLDERTYPE);
    params.setBrowseName("AvailableTypes");

    const auto typesFolderId = server->addObjectNode(params);
    const auto fbTypes = this->object.getAvailableFunctionBlockTypes().getValueList();

    for (const auto& fbType : fbTypes)
    {
        auto tmsFbType = std::make_shared<TmsServerFunctionBlockType>(fbType, server, daqContext, tmsContext);
        tmsFbType->registerOpcUaNode(typesFolderId);
        functionBlockTypes.push_back(tmsFbType);
    }
}

void TmsServerDevice::createAddFunctionBlockNode(const OpcUaNodeId& parentId)
{
    OpcUaNodeId nodeIdOut;
    AddMethodNodeParams params(nodeIdOut, parentId);
    params.referenceTypeId = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));
    params.setBrowseName("Add");
    params.outputArgumentsSize = 2;
    params.outputArguments = (UA_Argument*) UA_Array_new(params.outputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
    params.inputArgumentsSize = 2;
    params.inputArguments = (UA_Argument*) UA_Array_new(params.inputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);

    params.outputArguments[0].name = UA_STRING_ALLOC("nodeId");
    params.outputArguments[0].dataType = UA_TYPES[UA_TYPES_NODEID].typeId;
    params.outputArguments[0].valueRank = UA_VALUERANK_SCALAR;

    params.outputArguments[1].name = UA_STRING_ALLOC("localId");
    params.outputArguments[1].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    params.outputArguments[1].valueRank = UA_VALUERANK_SCALAR;

    params.inputArguments[0].name = UA_STRING_ALLOC("typeId");
    params.inputArguments[0].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    params.inputArguments[0].valueRank = UA_VALUERANK_SCALAR;

    params.inputArguments[1].name = UA_STRING_ALLOC("config");
    params.inputArguments[1].dataType = UA_TYPES_DAQBT[UA_TYPES_DAQBT_DAQKEYVALUEPAIR].typeId;
    params.inputArguments[1].valueRank = UA_VALUERANK_ONE_DIMENSION;

    auto methodNodeId = server->addMethodNode(params);

    auto callback = [this](NodeEventManager::MethodArgs args) -> UA_StatusCode
    {
        try
        {
            this->onAddFunctionBlock(args);
            return UA_STATUSCODE_GOOD;
        }
        catch (const OpcUaException& e)
        {
            return e.getStatusCode();
        }
        catch (...)
        {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    };

    addEvent(methodNodeId)->onMethodCall(callback);
}

void TmsServerDevice::createRemoveFunctionBlockNode(const OpcUaNodeId& parentId)
{
    OpcUaNodeId nodeIdOut;
    AddMethodNodeParams params(nodeIdOut, parentId);
    params.referenceTypeId = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));
    params.setBrowseName("Remove");
    params.inputArgumentsSize = 1;
    params.inputArguments = (UA_Argument*) UA_Array_new(params.inputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);

    params.inputArguments[0].name = UA_STRING_ALLOC("localId");
    params.inputArguments[0].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    params.inputArguments[0].valueRank = UA_VALUERANK_SCALAR;

    auto methodNodeId = server->addMethodNode(params);

    auto callback = [this](NodeEventManager::MethodArgs args) -> UA_StatusCode
    {
        try
        {
            this->onRemoveFunctionBlock(args);
            return UA_STATUSCODE_GOOD;
        }
        catch (const OpcUaException& e)
        {
            return e.getStatusCode();
        }
        catch (...)
        {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    };

    addEvent(methodNodeId)->onMethodCall(callback);
}

void TmsServerDevice::onGetAvailableFunctionBlockTypes(const NodeEventManager::MethodArgs& args)
{
    assert(args.outputSize == 1);

    const auto fbTypes = object.getAvailableFunctionBlockTypes().getValueList();
    const auto variant = ListConversionUtils::ToArrayVariant<IFunctionBlockType, UA_FunctionBlockInfoStructure>(fbTypes);
    args.output[0] = variant.copyAndGetDetachedValue();
}

void TmsServerDevice::onAddFunctionBlock(const NodeEventManager::MethodArgs& args)
{
    assert(args.inputSize == 2);
    assert(args.outputSize == 2);

    const auto fbTypeId = OpcUaVariant(args.input[0]).toString();
    const auto configVariant = OpcUaVariant(args.input[1]);

    auto tmsFunctionBlock = addFunctionBlock(fbTypeId, configVariant);

    auto nodeIdOut = OpcUaVariant(tmsFunctionBlock->getNodeId());
    auto localIdOut = OpcUaVariant(tmsFunctionBlock->getBrowseName().c_str());
    args.output[0] = nodeIdOut.copyAndGetDetachedValue();
    args.output[1] = localIdOut.copyAndGetDetachedValue();
}

void TmsServerDevice::onRemoveFunctionBlock(const NodeEventManager::MethodArgs& args)
{
    assert(args.inputSize == 1);

    const auto fbLocalId = OpcUaVariant(args.input[0]).toString();
    removeFunctionBlock(fbLocalId);
}


TmsServerFunctionBlockPtr TmsServerDevice::addFunctionBlock(const StringPtr& fbTypeId, const OpcUaVariant& configVariant)
{
    const auto fbTypes = object.getAvailableFunctionBlockTypes();

    if (!fbTypes.hasKey(fbTypeId))
        throw OpcUaException(UA_STATUSCODE_BADNOTFOUND, "Function block type not found");

    auto config = fbTypes.get(fbTypeId).createDefaultConfig();
    PropertyObjectConversionUtils::ToPropertyObject(configVariant, config);
    return addFunctionBlock(fbTypeId, config);
}

TmsServerFunctionBlockPtr TmsServerDevice::addFunctionBlock(const StringPtr& fbTypeId, const PropertyObjectPtr& config)
{
    const auto fbFolderNodeId = getChildNodeId("FB");

    auto functionBlock = object.addFunctionBlock(fbTypeId, config);
    auto tmsFunctionBlock = registerTmsObjectOrAddReference<TmsServerFunctionBlock<>>(fbFolderNodeId, functionBlock, functionBlocks.size());
    functionBlocks.push_back(tmsFunctionBlock);
    tmsFunctionBlock->createNonhierarchicalReferences();
    return tmsFunctionBlock;
}

void TmsServerDevice::removeFunctionBlock(const StringPtr& localId)
{
    for (auto it = functionBlocks.begin(); it != functionBlocks.end(); ++it)
    {
        auto fb = *it;

        if (fb->getBrowseName() == localId)
        {
            server->deleteNode(fb->getNodeId());
            functionBlocks.erase(it);
            break;
        }
    }

    const auto objFunctionBlocks = this->object.getFunctionBlocks(search::Any());

    for (const auto& fb : objFunctionBlocks)
    {
        if (fb.getLocalId() == localId)
        {
            this->object.removeFunctionBlock(fb);
            break;
        }
    }
}

void TmsServerDevice::addChildNodes()
{
    populateDeviceInfo();
    populateServerCapabilities();
    auto methodSetNodeId = getChildNodeId("MethodSet");
    tmsPropertyObject->setMethodParentNodeId(methodSetNodeId);

    uint32_t numberInList = 0;
    for (const auto& device : object.getDevices(search::Any()))
    {
        auto tmsDevice = registerTmsObjectOrAddReference<TmsServerDevice>(nodeId, device, numberInList++);
        devices.push_back(std::move(tmsDevice));
    }

    auto functionBlockNodeId = getChildNodeId("FB");
    assert(!functionBlockNodeId.isNull());
    numberInList = 0;
    for (const auto& functionBlock : object.getFunctionBlocks(search::Any()))
    {
        auto tmsFunctionBlock = registerTmsObjectOrAddReference<TmsServerFunctionBlock<>>(functionBlockNodeId, functionBlock, numberInList++);
        functionBlocks.push_back(std::move(tmsFunctionBlock));
    }

    auto signalsNodeId = getChildNodeId("Sig");
    assert(!signalsNodeId.isNull());
    numberInList = 0;
    for (const auto& signal : object.getSignals(search::Any()))
    {
        if (signal.getPublic())
        {
            auto tmsSignal = registerTmsObjectOrAddReference<TmsServerSignal>(signalsNodeId, signal, numberInList++);
            signals.push_back(std::move(tmsSignal));
        }
    }

    auto inputsOutputsNodeId = getChildNodeId("IO");
    assert(!inputsOutputsNodeId.isNull());

    auto topFolder = object.getInputsOutputsFolder();
    auto inputsOutputsNode = std::make_unique<TmsServerFolder>(topFolder, server, daqContext, tmsContext);
    inputsOutputsNode->registerToExistingOpcUaNode(inputsOutputsNodeId);
    folders.push_back(std::move(inputsOutputsNode));

    auto syncComponentNodeId = getChildNodeId("Synchronization");
    assert(!syncComponentNodeId.isNull());
    auto syncComponent = object.getSyncComponent();
    auto syncComponentNode = std::make_unique<TmsServerSyncComponent>(syncComponent, server, daqContext, tmsContext);
    syncComponentNode->registerToExistingOpcUaNode(syncComponentNodeId);
    syncComponents.push_back(std::move(syncComponentNode));

    tmsPropertyObject->ignoredProps.emplace("userName");
    tmsPropertyObject->ignoredProps.emplace("location");

    // TODO add "Srv" as a default node

    numberInList = 0;
    for (auto component : object.getItems(search::Any()))
    {
        auto id = component.getLocalId();
        if (id == "Dev" || id == "FB" || id == "IO" || id == "Sig" || id == "Synchronization" || id == "Srv")
            continue;

        if (component.supportsInterface<IFolder>())
        {
            auto folderNode = registerTmsObjectOrAddReference<TmsServerFolder>(nodeId, component, numberInList++);
            folders.push_back(std::move(folderNode));
        }
        else
        {
            auto componentNode = registerTmsObjectOrAddReference<TmsServerComponent<>>(nodeId, component, numberInList++);
            components.push_back(std::move(componentNode));
        }
    }

    addFunctionBlockFolderNodes();
    Super::addChildNodes();
}


void TmsServerDevice::createNonhierarchicalReferences()
{
    createChildNonhierarchicalReferences(signals);
    createChildNonhierarchicalReferences(devices);
    createChildNonhierarchicalReferences(functionBlocks);
    createChildNonhierarchicalReferences(folders);
    createChildNonhierarchicalReferences(components);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS

