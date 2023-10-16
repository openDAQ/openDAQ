#include <opcuaclient/opcuaclient.h>
#include <opcuaclient/subscriptions.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

/*Subscription*/

Subscription::Subscription(OpcUaClient* client, const StatusChangeNotificationCallbackType& statusChangeNotificationCallback)
    : client(client)
    , statusChangeNotificationCallback(statusChangeNotificationCallback)
{
}

UA_UInt32 Subscription::getSubscriptionId() const
{
    return subscriptionResponse->subscriptionId;
}

UA_Double Subscription::getRevisedPublishingInterval() const
{
    return subscriptionResponse->revisedPublishingInterval;
}

UA_UInt32 Subscription::getRevisedLifetimeCount() const
{
    return subscriptionResponse->revisedLifetimeCount;
}

UA_UInt32 Subscription::getRevisedMaxKeepAliveCount() const
{
    return subscriptionResponse->revisedMaxKeepAliveCount;
}

const StatusChangeNotificationCallbackType& Subscription::getStatusChangeNotificationCallback() const
{
    return statusChangeNotificationCallback;
}

static void DeleteSubscriptionCallback(UA_Client* client, UA_UInt32 subId, void* subContext)
{
    delete (Subscription*) subContext;
}

static void StatusChangeNotificationCallback(UA_Client* client,
                                             UA_UInt32 subId,
                                             void* subContext,
                                             UA_StatusChangeNotification* notification)
{
    auto subscription = (Subscription*) subContext;
    if (subscription->getStatusChangeNotificationCallback())
    {
        OpcUaClient* opcUaClient = static_cast<OpcUaClient*>(UA_Client_getContext(client));
        subscription->getStatusChangeNotificationCallback()(opcUaClient, subscription, notification);
    }
}

Subscription* Subscription::CreateSubscription(OpcUaClient* client,
                                               const OpcUaObject<UA_CreateSubscriptionRequest>& request,
                                               const StatusChangeNotificationCallbackType& statusChangeCallback)
{
    auto subscription = new Subscription(client, statusChangeCallback);

    OpcUaObject<UA_CreateSubscriptionResponse> response = UA_Client_Subscriptions_create(
        client->getLockedUaClient(), *request, subscription, StatusChangeNotificationCallback, DeleteSubscriptionCallback);

    subscription->subscriptionResponse = response;

    CheckStatusCodeException(response->responseHeader.serviceResult, "Failed to create subscription");

    return subscription;
}

/*MonitoredItem*/

static void DeleteMonitoredItemCallback(UA_Client* client, UA_UInt32 subId, void* subContext, UA_UInt32 monId, void* monContext)
{
    delete (MonitoredItem*) monContext;
}

static void EventNotificationCallback(
    UA_Client* client, UA_UInt32 subId, void* subContext, UA_UInt32 monId, void* monContext, size_t nEventFields, UA_Variant* eventFields)
{
    auto monitoredItem = (MonitoredItem*) monContext;
    if (monitoredItem->getEventNotificationCallback())
    {
        OpcUaClient* opcUaClient = static_cast<OpcUaClient*>(UA_Client_getContext(client));
        auto subscription = (Subscription*) subContext;
        monitoredItem->getEventNotificationCallback()(opcUaClient, subscription, monitoredItem, nEventFields, eventFields);
    }
}

static void DataChangeNotificationCallback(
    UA_Client* client, UA_UInt32 subId, void* subContext, UA_UInt32 monId, void* monContext, UA_DataValue* value)
{
    auto monitoredItem = (MonitoredItem*) monContext;
    if (monitoredItem->getDataChangeNotificationCallback())
    {
        OpcUaClient* opcUaClient = static_cast<OpcUaClient*>(UA_Client_getContext(client));
        auto subscription = (Subscription*) subContext;
        monitoredItem->getDataChangeNotificationCallback()(opcUaClient, subscription, monitoredItem, value);
    }
}

MonitoredItem* Subscription::monitoredItemsCreateEvent(UA_TimestampsToReturn timestampsToReturn,
                                                       const UA_MonitoredItemCreateRequest& item,
                                                       const EventNotificationCallbackType& eventNotificationCallback)
{
    auto monitoredItem = new MonitoredItem(client, eventNotificationCallback);

    UA_MonitoredItemCreateResult result = UA_Client_MonitoredItems_createEvent(client->getLockedUaClient(),
                                                                               getSubscriptionId(),
                                                                               UA_TIMESTAMPSTORETURN_BOTH,
                                                                               item,
                                                                               monitoredItem,
                                                                               EventNotificationCallback,
                                                                               DeleteMonitoredItemCallback);

    monitoredItem->response = result;

    CheckStatusCodeException(result.statusCode, "Failed to create monitored item");

    return monitoredItem;
}

MonitoredItem* Subscription::monitoredItemsCreateDataChange(UA_TimestampsToReturn timestampsToReturn,
                                                            const UA_MonitoredItemCreateRequest& item,
                                                            const DataChangeNotificationCallbackType& dataChangeNotificationCallback)
{
    auto monitoredItem = new MonitoredItem(client, dataChangeNotificationCallback);

    UA_MonitoredItemCreateResult result = UA_Client_MonitoredItems_createDataChange(client->getLockedUaClient(),
                                                                                    getSubscriptionId(),
                                                                                    UA_TIMESTAMPSTORETURN_BOTH,
                                                                                    item,
                                                                                    monitoredItem,
                                                                                    DataChangeNotificationCallback,
                                                                                    DeleteMonitoredItemCallback);

    monitoredItem->response = result;

    CheckStatusCodeException(result.statusCode, "Failed to create monitored item");

    return monitoredItem;
}

/*MonitoredItem*/

MonitoredItem::MonitoredItem(OpcUaClient* client, const EventNotificationCallbackType& eventNotificationCallback)
    : client(client)
    , eventNotificationCallback(eventNotificationCallback)
{
}

MonitoredItem::MonitoredItem(OpcUaClient* client, const DataChangeNotificationCallbackType& dataChangeNotificationCallback)
    : client(client)
    , dataChangeNotificationCallback(dataChangeNotificationCallback)
{
}

UA_UInt32 MonitoredItem::getMonitoredItemId() const
{
    return response->monitoredItemId;
}

const EventNotificationCallbackType& MonitoredItem::getEventNotificationCallback() const
{
    return eventNotificationCallback;
}

const DataChangeNotificationCallbackType& MonitoredItem::getDataChangeNotificationCallback() const
{
    return dataChangeNotificationCallback;
}

END_NAMESPACE_OPENDAQ_OPCUA
