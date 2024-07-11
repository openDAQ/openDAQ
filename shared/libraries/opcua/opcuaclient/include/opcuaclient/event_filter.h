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

#include <opcuashared/opcuaobject.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class EventFilter : public OpcUaObject<UA_EventFilter>
{
public:
    using OpcUaObject<UA_EventFilter>::OpcUaObject;

    EventFilter(size_t selectClausesSize);

    void resizeSelectClauses(size_t selectClausesSize);

    void setSelectClause(size_t index, const OpcUaObject<UA_SimpleAttributeOperand>& simpleAttributeOperand);
    void setSelectClause(size_t index, OpcUaObject<UA_SimpleAttributeOperand>&& simpleAttributeOperand);
};

class SimpleAttributeOperand
{
public:
    static OpcUaObject<UA_SimpleAttributeOperand> CreateEventIdValue()
    {
        return CreateStandardEventValue("EventId");
    }

    static OpcUaObject<UA_SimpleAttributeOperand> CreateEventTypeValue()

    {
        return CreateStandardEventValue("EventType");
    }

    static OpcUaObject<UA_SimpleAttributeOperand> CreateSourceNodeValue()
    {
        return CreateStandardEventValue("SourceNode");
    }

    static OpcUaObject<UA_SimpleAttributeOperand> CreateSourceNameValue()
    {
        return CreateStandardEventValue("SourceName");
    }

    static OpcUaObject<UA_SimpleAttributeOperand> CreateTimeValue()
    {
        return CreateStandardEventValue("Time");
    }

    static OpcUaObject<UA_SimpleAttributeOperand> CreateReceiveTimeValue()
    {
        return CreateStandardEventValue("ReceiveTime");
    }

    static OpcUaObject<UA_SimpleAttributeOperand> CreateMessageValue()
    {
        return CreateStandardEventValue("Message");
    }

    static OpcUaObject<UA_SimpleAttributeOperand> CreateSeverityValue()
    {
        return CreateStandardEventValue("Severity");
    }

    static OpcUaObject<UA_SimpleAttributeOperand> CreateStandardEventValue(const char* attributeName);
};

END_NAMESPACE_OPENDAQ_OPCUA
