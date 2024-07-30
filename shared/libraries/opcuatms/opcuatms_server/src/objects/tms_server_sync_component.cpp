#include <opcuatms_server/objects/tms_server_channel.h>
#include <opcuatms_server/objects/tms_server_sync_component.h>
#include <opcuatms/converters/variant_converter.h>
#include <open62541/daqdevice_nodeids.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

TmsServerSyncComponent::TmsServerSyncComponent(const SyncComponentPtr& object, const OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext)
    : Super(object, server, context, tmsContext)
{
}

void TmsServerSyncComponent::addChildNodes()
{
    if (object.hasProperty("Interfaces"))
    {
        const auto prop = object.getProperty("Interfaces");
        const auto propName = prop.getName();
        const auto obj = object.getPropertyValue(propName);

        auto intefacesNodeId = getChildNodeId("Interfaces");
        interfaces = std::make_shared<TmsServerSyncInterfaces>(obj, server, daqContext, tmsContext, propName, prop);
        interfaces->setNumberInList(0);
        interfaces->registerToExistingOpcUaNode(intefacesNodeId);
    }

    tmsPropertyObject->ignoredProps.emplace("Interfaces");
    tmsPropertyObject->ignoredProps.emplace("InterfaceNames");
    tmsPropertyObject->ignoredProps.emplace("SynchronizationLocked");

    Super::addChildNodes();
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

void TmsServerSyncComponent::bindCallbacks()
{
    // Bind read callback for SynchronizationLocked property
    this->addReadCallback("SynchronizationLocked", 
        [this] { return VariantConverter<IBoolean>::ToVariant( this->object.getSyncLocked()); });

    Super::bindCallbacks();
}


END_NAMESPACE_OPENDAQ_OPCUA_TMS
