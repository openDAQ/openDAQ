/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opcuashared/opcuanodeid.h>
#include <opcuashared/opcuaobject.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class MonitoredItemCreateRequest : public OpcUaObject<UA_MonitoredItemCreateRequest>
{
public:
    using OpcUaObject<UA_MonitoredItemCreateRequest>::OpcUaObject;
    MonitoredItemCreateRequest();
};

class EventMonitoredItemCreateRequest : public MonitoredItemCreateRequest
{
public:
    using MonitoredItemCreateRequest::MonitoredItemCreateRequest;
    EventMonitoredItemCreateRequest();
    EventMonitoredItemCreateRequest(const OpcUaNodeId& nodeId);

    void setItemToMonitor(const OpcUaNodeId& nodeId);

    void setEventFilter(const OpcUaObject<UA_EventFilter>& eventFilter);
    void setEventFilter(OpcUaObject<UA_EventFilter>&& eventFilter);
    void setEventFilter(UA_EventFilter* eventFilter);
};

END_NAMESPACE_OPENDAQ_OPCUA
