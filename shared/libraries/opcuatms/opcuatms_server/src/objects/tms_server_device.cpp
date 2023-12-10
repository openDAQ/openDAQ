#include <iostream>
#include <string>
#include <stdexcept>
#include <opendaq/device_ptr.h>
#include "opcuatms_server/objects/tms_server_device.h"
#include "opcuatms/core_types_utils.h"
#include "opcuatms/type_mappings.h"
#include <open62541/daqdevice_nodeids.h>
#include "opcuatms/converters/struct_converter.h"
#include <opendaq/component_ptr.h>
#include <opendaq/device_private.h>
#include <opendaq/streaming_info_ptr.h>

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

    static std::unordered_map<std::string, std::function<OpcUaVariant(const DeviceInfoPtr&)>> componentFieldToVariant = {
        {"AssetId", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getAssetId().getCharPtr()}; }},
        {"ComponentName", [](const DeviceInfoPtr& info){ return createLocalizedTextVariant(info.getName().getCharPtr()); }},
        {"DeviceClass", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getDeviceClass().getCharPtr()}; }},
        {"DeviceManual", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getDeviceManual().getCharPtr()}; }},
        {"DeviceRevision", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getDeviceRevision().getCharPtr()}; }},
        {"HardwareRevision", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getHardwareRevision().getCharPtr()}; }},
        {"Manufacturer", [](const DeviceInfoPtr& info) { return createLocalizedTextVariant(info.getManufacturer().getCharPtr()); }},
        {"ManufacturerUri", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getManufacturerUri().getCharPtr()}; }},
        {"Model", [](const DeviceInfoPtr& info) { return createLocalizedTextVariant(info.getModel().getCharPtr()); }},
        {"ProductCode", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getProductCode().getCharPtr()}; }},
        {"ProductInstanceUri", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getProductInstanceUri().getCharPtr()}; }},
        {"RevisionCounter", [](const DeviceInfoPtr& info) { return OpcUaVariant{static_cast<int>(info.getRevisionCounter())}; }},
        {"SerialNumber", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getSerialNumber().getCharPtr()}; }},
        {"SoftwareRevision", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getSoftwareRevision().getCharPtr()}; }},
        {"MacAddress", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getMacAddress().getCharPtr()}; }},
        {"ParentMacAddress", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getParentMacAddress().getCharPtr()}; }},
        {"Platform", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getPlatform().getCharPtr()}; }},
        {"Position", [](const DeviceInfoPtr& info) { return OpcUaVariant{static_cast<uint16_t>(info.getPosition())}; }},
        {"SystemType", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getSystemType().getCharPtr()}; }},
        {"SystemUUID", [](const DeviceInfoPtr& info) { return OpcUaVariant{info.getSystemUuid().getCharPtr()}; }}
    };
}

TmsServerDevice::TmsServerDevice(const DevicePtr& object, const OpcUaServerPtr& server, const ContextPtr& context)
    : Super(object, server, context)
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

    return Super::createOptionalNode(nodeId);
}

void TmsServerDevice::bindCallbacks()
{
    this->addReadCallback("Domain",[this]()
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
                uaDeviceDomain->ticksSinceOrigin = deviceDomain.getTicksSinceOrigin();
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

    auto deviceInfo = object.getInfo();

    const auto customInfoNames = deviceInfo.getCustomInfoPropertyNames();
    std::unordered_set<std::string> customInfoNamesSet;

    for (auto propName : customInfoNames)
    {
        try
        {
            createNode(propName, deviceInfo.getProperty(propName).getValueType());
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

        if (detail::componentFieldToVariant.count(browseName))
        {
            auto v = detail::componentFieldToVariant[browseName](deviceInfo);
            server->writeValue(reference.nodeId.nodeId, *v);
        }
        else if (customInfoNamesSet.count(browseName))
        {
            const auto valueType = deviceInfo.getProperty(browseName).getValueType();
            OpcUaVariant v;
            if (valueType == ctBool)
            {
                bool val = deviceInfo.getPropertyValue(browseName);
                v = OpcUaVariant(val);
            }
            else if (valueType == ctInt)
            {
                int64_t val = deviceInfo.getPropertyValue(browseName);
                v = OpcUaVariant(val);
            }
            else if (valueType == ctFloat)
            {
                double val = deviceInfo.getPropertyValue(browseName);
                v = OpcUaVariant(val);
            }
            else if (valueType == ctString)
            {
                v = OpcUaVariant(deviceInfo.getPropertyValue(browseName).asPtr<IString>().getCharPtr());
            }
            else
            {
                continue;
            }
            
            server->writeValue(reference.nodeId.nodeId, *v);
        }
    }
}

void TmsServerDevice::populateStreamingOptions()
{
    auto params = AddObjectNodeParams(UA_NODEID_NULL, nodeId);
    params.setBrowseName("StreamingOptions");
    auto streamingOptionsNodeId = server->addObjectNode(params);

    auto devicePrivatePtr = object.asPtrOrNull<IDevicePrivate>();
    if (devicePrivatePtr == nullptr) // Instance does not implement IDevicePrivate
        return;

    ListPtr<IStreamingInfo> streamingOptions;
    devicePrivatePtr->getStreamingOptions(&streamingOptions);

    uint32_t numberInList = 0;
    for (const auto& streamingOption : streamingOptions)
    {
        auto tmsStreamingOption = registerTmsObjectOrAddReference<TmsServerPropertyObject>(
            streamingOptionsNodeId, streamingOption.asPtr<IPropertyObject>(), numberInList++, streamingOption.getProtocolId());
        this->streamingOptions.push_back(std::move(tmsStreamingOption));
    }
}

void TmsServerDevice::addChildNodes()
{
    populateDeviceInfo();
    populateStreamingOptions();
    auto methodSetNodeId = getChildNodeId("MethodSet");
    tmsPropertyObject->setMethodParentNodeId(methodSetNodeId);

    uint32_t numberInList = 0;
    for (const auto& device : object.getDevices())
    {
        auto tmsDevice = registerTmsObjectOrAddReference<TmsServerDevice>(nodeId, device, numberInList++);
        devices.push_back(std::move(tmsDevice));
    }

    auto functionBlockNodeId = getChildNodeId("FB");
    assert(!functionBlockNodeId.isNull());
    numberInList = 0;
    for (const auto& functionBlock : object.getFunctionBlocks())
    {
        auto tmsFunctionBlock = registerTmsObjectOrAddReference<TmsServerFunctionBlock<>>(functionBlockNodeId, functionBlock, numberInList++);
        functionBlocks.push_back(std::move(tmsFunctionBlock));
    }

    auto signalsNodeId = getChildNodeId("Sig");
    assert(!signalsNodeId.isNull());
    numberInList = 0;
    for (const auto& signal : object.getSignals())
    {
        auto tmsSignal = registerTmsObjectOrAddReference<TmsServerSignal>(signalsNodeId, signal, numberInList++);
        signals.push_back(std::move(tmsSignal));
    }

    auto inputsOutputsNodeId = getChildNodeId("IO");
    assert(!inputsOutputsNodeId.isNull());

    auto topFolder = object.getInputsOutputsFolder();
    auto inputsOutputsNode = std::make_unique<TmsServerFolder>(topFolder, server, daqContext);
    inputsOutputsNode->registerToExistingOpcUaNode(inputsOutputsNodeId);
    folders.push_back(std::move(inputsOutputsNode));
    
    numberInList = 0;
    for (auto component : object.getItems())
    {
        auto id = component.getLocalId();
        if (id == "Dev" || id == "FB" || id == "IO" || id == "Sig")
            continue;

        if (component.asPtrOrNull<IFolder>().assigned())
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

