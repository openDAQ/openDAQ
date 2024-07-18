#include "opcuatms_server/objects/tms_server_channel.h"
#include "opcuatms_server/objects/tms_server_sync_component.h"
#include "opcuatms_server/objects/tms_server_sync_interface.h"
#include "opcuatms/converters/variant_converter.h"
#include "open62541/daqdevice_nodeids.h"
#include "opendaq/io_folder_config.h"
#include "opendaq/search_filter_factory.h"
#include <iostream>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsServerSyncComponent::TmsServerSyncComponent(const SyncComponentPtr& object, const OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext)
    : Super(object, server, context, tmsContext)
{
}

void TmsServerSyncComponent::addChildNodes()
{
    std::unordered_map<std::string, uint32_t> propOrder = 
    {
        {"Interfaces", 0},
        {"Source", 1},
        {"SyncronizationLocked", 2}
    };

    if (object.hasProperty("Interfaces"))
    {
        const auto prop = object.getProperty("Interfaces");
        const auto propName = prop.getName();
        const auto obj = object.getPropertyValue(propName);

        auto intefacesNodeId = getChildNodeId("Interfaces");
        auto interfaceNode = std::make_shared<TmsServerSyncInterfaces>(obj, server, daqContext, tmsContext, propName, prop);
        interfaceNode->setNumberInList(propOrder[propName]);
        interfaceNode->registerToExistingOpcUaNode(intefacesNodeId);

        childObjects.insert({intefacesNodeId, interfaceNode});
    }

    if (object.hasProperty("Source"))
    {
        const auto prop = object.getProperty("Source");

        auto sourceNodeId = getChildNodeId("Source");
        auto sourceNode = std::make_shared<TmsServerProperty>(prop, server, daqContext, tmsContext, object, propOrder);
        sourceNode->registerToExistingOpcUaNode(sourceNodeId);

        childProperties.insert({sourceNodeId, sourceNode});
    }

    if (object.hasProperty("SyncronizationLocked"))
    {
        const auto prop = object.getProperty("SyncronizationLocked");

        auto syncronizationLockedNodeId = getChildNodeId("SyncronizationLocked");
        auto syncronizationLockedNode = std::make_shared<TmsServerProperty>(prop, server, daqContext, tmsContext, object, propOrder);
        syncronizationLockedNode->registerToExistingOpcUaNode(syncronizationLockedNodeId);

        childProperties.insert({syncronizationLockedNodeId, syncronizationLockedNode});
    }
}

OpcUaNodeId TmsServerSyncComponent::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_SYNCCOMPONENTTYPE);
}

void TmsServerSyncComponent::triggerEvent(PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
{
    if(!this->server->getUaServer())
        return;

    EventAttributes attributes;
    attributes.setTime(UA_DateTime_now());
    attributes.setMessage("Property value changed");
    this->server->triggerEvent(OpcUaNodeId(UA_NS0ID_BASEEVENTTYPE), nodeId, attributes);
}

void TmsServerSyncComponent::bindPropertyCallbacks(const std::string& name)
{
    if (!this->object.getProperty(name).asPtr<IPropertyInternal>().getReferencedPropertyUnresolved().assigned())
    {
        addReadCallback(name, [this, name]() {
            const auto value = this->object.getPropertyValue(name);
            return VariantConverter<IBaseObject>::ToVariant(value, nullptr, daqContext);
        });

        const auto freezable = this->object.asPtrOrNull<IFreezable>();
        if (!freezable.assigned() || !this->object.isFrozen())
        {
            addWriteCallback(name, [this, name](const OpcUaVariant& variant) {
                const auto value = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
                this->object.setPropertyValue(name, value);
                return UA_STATUSCODE_GOOD;
            });
        }
    }
    else
    {
        addReadCallback(name, [this, name]() {
            const auto refProp = this->object.getProperty(name).asPtr<IPropertyInternal>().getReferencedPropertyUnresolved();
            return VariantConverter<IBaseObject>::ToVariant(refProp.getEval(), nullptr, daqContext);
        });
    }
}

void TmsServerSyncComponent::bindCallbacks()
{
    for (const auto& [id, prop] : childProperties)
    {
        this->object.getOnPropertyValueWrite(prop->getBrowseName()) += event(this, &TmsServerSyncComponent::triggerEvent);
        bindPropertyCallbacks(prop->getBrowseName());
    }
}


END_NAMESPACE_OPENDAQ_OPCUA_TMS
