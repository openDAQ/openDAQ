/*
 * Copyright 2022-2024 Blueberry d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <opcuashared/opcua.h>
#include <opcuashared/opcuaobject.h>
#include <open62541/client_subscriptions.h>
#include <open62541/types_generated.h>
#include <functional>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaClient;
class Subscription;
class MonitoredItem;

using StatusChangeNotificationCallbackType =
    std::function<void(OpcUaClient* client, Subscription* subContext, UA_StatusChangeNotification* notification)>;

using EventNotificationCallbackType = std::function<void(
    OpcUaClient* client, Subscription* subContext, MonitoredItem* monContext, size_t nEventFields, UA_Variant* eventFields)>;

using DataChangeNotificationCallbackType =
    std::function<void(OpcUaClient* client, Subscription* subContext, MonitoredItem* monContext, UA_DataValue* value)>;

class Subscription
{
protected:
    Subscription(OpcUaClient* client, const StatusChangeNotificationCallbackType& statusChangeNotificationCallback = nullptr);

public:
    UA_UInt32 getSubscriptionId() const;
    UA_Double getRevisedPublishingInterval() const;
    UA_UInt32 getRevisedLifetimeCount() const;
    UA_UInt32 getRevisedMaxKeepAliveCount() const;

    MonitoredItem* monitoredItemsCreateEvent(UA_TimestampsToReturn timestampsToReturn,
                                             const UA_MonitoredItemCreateRequest& item,
                                             const EventNotificationCallbackType& eventNotificationCallback);

    MonitoredItem* monitoredItemsCreateDataChange(UA_TimestampsToReturn timestampsToReturn,
                                                  const UA_MonitoredItemCreateRequest& item,
                                                  const DataChangeNotificationCallbackType& dataChangeNotificationCallback);

    const StatusChangeNotificationCallbackType& getStatusChangeNotificationCallback() const;

    static Subscription* CreateSubscription(OpcUaClient* client,
                                            const OpcUaObject<UA_CreateSubscriptionRequest>& request,
                                            const StatusChangeNotificationCallbackType& statusChangeCallback);

protected:
    OpcUaClient* client;
    OpcUaObject<UA_CreateSubscriptionResponse> subscriptionResponse;
    StatusChangeNotificationCallbackType statusChangeNotificationCallback;
};

class MonitoredItem
{
public:
    MonitoredItem(OpcUaClient* client, const EventNotificationCallbackType& eventNotificationCallback = nullptr);
    MonitoredItem(OpcUaClient* client, const DataChangeNotificationCallbackType& dataChangeNotificationCallback = nullptr);

    UA_UInt32 getMonitoredItemId() const;

    const EventNotificationCallbackType& getEventNotificationCallback() const;
    const DataChangeNotificationCallbackType& getDataChangeNotificationCallback() const;

protected:
    OpcUaClient* client;
    OpcUaObject<UA_MonitoredItemCreateResult> response;
    EventNotificationCallbackType eventNotificationCallback;
    DataChangeNotificationCallbackType dataChangeNotificationCallback;

    friend class Subscription;
};

END_NAMESPACE_OPENDAQ_OPCUA
