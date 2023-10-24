/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include "opcuaclient/monitored_item_create_request.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/opcuatms.h"
#include "opcuaclient/reference_utils.h"
#include <opendaq/signal_ptr.h>
#include <opcuatms_client/objects/tms_client_context.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsClientObjectImpl
{
public:
    using ReferenceMap = std::unordered_map<std::string, opcua::OpcUaObject<UA_ReferenceDescription>>;

protected:
    explicit TmsClientObjectImpl(const ContextPtr& daqContext, const TmsClientContextPtr& ctx, const opcua::OpcUaNodeId& nodeId);
    virtual ~TmsClientObjectImpl();

    void registerObject(const BaseObjectPtr& obj);
    SignalPtr findSignal(const opcua::OpcUaNodeId& nodeId) const;
    bool hasReference(const std::string& name);
    opcua::OpcUaNodeId getNodeId(const std::string& nodeName);
    void writeValue(const std::string& nodeName, const opcua::OpcUaVariant& value);
    opcua::OpcUaVariant readValue(const std::string& nodeName);
    virtual void subscriptionStatusChangeCallback(UA_StatusChangeNotification* notification);
    uint32_t tryReadChildNumberInList(const std::string& nodeName);
    uint32_t tryReadChildNumberInList(const opcua::OpcUaNodeId& nodeId);
    
    /*!
    * @brief Returns child nodes of a specific type. By default it returns also the nodes which are a
    *        a subtype of the specific type. It can be disabled.
    * @param client The opc-ua client
    * @param nodeId The root nodeId
    * @param typeId The type of child nodes to return
    * @param subTypeEnabled Allows also that nodes of the sub types of the request type are returned.  
    */
    std::vector<daq::opcua::OpcUaNodeId> getChildNodes(const opcua::OpcUaClientPtr& client,
                                                       const opcua::OpcUaNodeId& nodeId,
                                                       const opcua::OpcUaNodeId& typeId,
                                                       const bool subTypeEnabled = true);

    opcua::MonitoredItem* monitoredItemsCreateEvent(
        const opcua::EventMonitoredItemCreateRequest& item,
        const opcua::EventNotificationCallbackType& eventNotificationCallback);

    opcua::MonitoredItem* monitoredItemsCreateDataChange(
        const UA_MonitoredItemCreateRequest& item,
        const opcua::DataChangeNotificationCallbackType& dataChangeNotificationCallback);

    template <typename CoreType, class CoreTypePtr = typename InterfaceToSmartPtr<CoreType>::SmartPtr>
    void writeValue(const std::string& nodeName, const CoreTypePtr& value)
    {
        writeValue(nodeName, VariantConverter<CoreType>::ToVariant(value));
    }

    template <typename CoreType, class CoreTypePtr = typename InterfaceToSmartPtr<CoreType>::SmartPtr>
    CoreTypePtr readValue(const std::string& nodeName)
    {
        const auto variant = readValue(nodeName);
        return VariantConverter<CoreType>::ToDaqObject(variant);
    }

    template <typename CoreType, class CoreTypePtr = typename InterfaceToSmartPtr<CoreType>::SmartPtr>
    ListPtr<CoreType> readList(const std::string& nodeName)
    {
        const auto variant = readValue(nodeName);
        return VariantConverter<CoreType>::ToDaqList(variant);
    }

    TmsClientContextPtr clientContext;

    opcua::OpcUaClientPtr client;
    opcua::OpcUaNodeId nodeId;
    opcua::ReferenceUtils referenceUtils;
    ContextPtr daqContext;

private:
    opcua::Subscription* getSubscription();
    opcua::Subscription* subscription = nullptr;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
