#include "opcuatms_client/objects/tms_client_object_impl.h"
#include "opcuaclient/browse_request.h"
#include "opcuaclient/browser/opcuabrowser.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;
using namespace opcua::utils;

TmsClientObjectImpl::TmsClientObjectImpl(const ContextPtr& daqContext, const TmsClientContextPtr& clientContext, const OpcUaNodeId& nodeId)
    : clientContext(clientContext)
    , client(clientContext->getClient())
    , nodeId(nodeId)
    , daqContext(daqContext)
{
}

TmsClientObjectImpl::~TmsClientObjectImpl()
{
    clientContext->unregisterObject(nodeId);
}

void TmsClientObjectImpl::registerObject(const BaseObjectPtr& obj)
{
    clientContext->registerObject(nodeId, obj);
}

SignalPtr TmsClientObjectImpl::findSignal(const opcua::OpcUaNodeId& nodeId) const
{
    return clientContext->getObject<ISignal>(nodeId);
}

bool TmsClientObjectImpl::hasReference(const std::string& name)
{
    return clientContext->getReferenceBrowser()->hasReference(nodeId, name);
}

OpcUaNodeId TmsClientObjectImpl::getNodeId(const std::string& nodeName)
{
    return clientContext->getReferenceBrowser()->getChildNodeId(nodeId, nodeName);
}

void TmsClientObjectImpl::writeValue(const std::string& nodeName, const OpcUaVariant& value)
{
    const auto nodeId = getNodeId(nodeName);
    client->writeValue(nodeId, value);
}

OpcUaVariant TmsClientObjectImpl::readValue(const std::string& nodeName)
{
    const auto nodeId = getNodeId(nodeName);
    return client->readValue(nodeId);
}

MonitoredItem* TmsClientObjectImpl::monitoredItemsCreateEvent(const EventMonitoredItemCreateRequest& item,
                                                              const EventNotificationCallbackType& eventNotificationCallback)
{
    return getSubscription()->monitoredItemsCreateEvent(UA_TIMESTAMPSTORETURN_BOTH, *item, eventNotificationCallback);
}

MonitoredItem* TmsClientObjectImpl::monitoredItemsCreateDataChange(const UA_MonitoredItemCreateRequest& item,
                                                                   const DataChangeNotificationCallbackType& dataChangeNotificationCallback)
{
    return getSubscription()->monitoredItemsCreateDataChange(UA_TIMESTAMPSTORETURN_BOTH, item, dataChangeNotificationCallback);
}

Subscription* TmsClientObjectImpl::getSubscription()
{
    if (!subscription)
        subscription = client->createSubscription(UA_CreateSubscriptionRequest_default(), std::bind(&TmsClientObjectImpl::subscriptionStatusChangeCallback, this, std::placeholders::_3));

    return subscription;
}

void TmsClientObjectImpl::subscriptionStatusChangeCallback(UA_StatusChangeNotification* notification)
{
    //TODO report on disconnect
}

uint32_t TmsClientObjectImpl::tryReadChildNumberInList(const std::string& nodeName)
{
    try
    {
        const auto childId = this->getNodeId(nodeName);
        return tryReadChildNumberInList(childId);
    }
    catch(...)
    {
    }

    return std::numeric_limits<uint32_t>::max();
}

uint32_t TmsClientObjectImpl::tryReadChildNumberInList(const opcua::OpcUaNodeId& nodeId)
{
    try
    {
        const auto numberInListId = clientContext->getReferenceBrowser()->getChildNodeId(nodeId, "NumberInList");
        return VariantConverter<IInteger>::ToDaqObject(client->readValue(numberInListId));
    }
    catch(...)
    {
    }

    return std::numeric_limits<uint32_t>::max();
}

CachedReferences TmsClientObjectImpl::getChildReferencesOfType(const opcua::OpcUaNodeId& nodeId, const opcua::OpcUaNodeId& typeId)
{
    BrowseFilter filter;
    filter.typeDefinition = typeId;
    return clientContext->getReferenceBrowser()->browseFiltered(nodeId, filter);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
