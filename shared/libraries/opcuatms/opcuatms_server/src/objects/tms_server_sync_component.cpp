#include "opcuatms_server/objects/tms_server_channel.h"
#include "opcuatms_server/objects/tms_server_sync_component.h"
#include "opcuatms/converters/variant_converter.h"

#include "open62541/daqdevice_nodeids.h"

#include <opendaq/io_folder_config.h>
#include <opendaq/search_filter_factory.h>

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
        interfaces = std::make_shared<TmsServerSyncInterfaces>(obj, server, daqContext, tmsContext, propName, prop);
        interfaces->setNumberInList(propOrder[propName]);
        interfaces->registerToExistingOpcUaNode(intefacesNodeId);
    }

    if (object.hasProperty("Source"))
    {
        const auto prop = object.getProperty("Source");

        auto sourceNodeId = getChildNodeId("Source");
        source = std::make_shared<TmsServerProperty>(prop, server, daqContext, tmsContext, object, propOrder);
        source->registerToExistingOpcUaNode(sourceNodeId);
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
        addReadCallback(name, [this, name] {
            const auto value = this->object.getPropertyValue(name);
            return VariantConverter<IBaseObject>::ToVariant(value, nullptr, daqContext);
        });

        const auto freezable = this->object.asPtrOrNull<IFreezable>();
        if (!freezable.assigned() || !this->object.isFrozen())
        {
            addWriteCallback(name, [this, name] (const OpcUaVariant& variant) {
                const auto value = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
                this->object.setPropertyValue(name, value);
                return UA_STATUSCODE_GOOD;
            });
        }
    }
    else
    {
        addReadCallback(name, [this, name] {
            const auto refProp = this->object.getProperty(name).asPtr<IPropertyInternal>().getReferencedPropertyUnresolved();
            return VariantConverter<IBaseObject>::ToVariant(refProp.getEval(), nullptr, daqContext);
        });
    }
}

void TmsServerSyncComponent::bindCallbacks()
{
    // Bind read callback for Interfaces property
    bindPropertyCallbacks(interfaces->getBrowseName());

    // Bind callbacks for source property
    this->object.getOnPropertyValueWrite(source->getBrowseName()) += event(this, &TmsServerSyncComponent::triggerEvent);
    bindPropertyCallbacks(source->getBrowseName());

    // Bind read callback for SyncronizationLocked property
    this->addReadCallback("SynchronizationLocked", 
        [this] { return VariantConverter<IBoolean>::ToVariant( this->object.getSyncLocked()); });

    Super::bindCallbacks();
}


END_NAMESPACE_OPENDAQ_OPCUA_TMS
