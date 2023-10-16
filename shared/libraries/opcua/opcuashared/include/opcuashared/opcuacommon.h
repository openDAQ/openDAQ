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

/*
 * A bundle of static methods used in different projects accross the platform
 */

#pragma once

#include <opcuashared/opcua.h>
#include <opcuashared/opcuaexception.h>
#include <opcuashared/opcuaobject.h>
#include <chrono>

#include "open62541/util.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

namespace utils
{
    inline std::string ToStdString(const UA_String& value)
    {
        return std::string((const char*) value.data, value.length);
    }

    double ToSeconds(const UA_DateTime& time);
    UA_StatusCode ToUaVariant(double value, const UA_NodeId& dataTypeNodeId, UA_Variant* var);
    void ToUaVariant(const std::string& value, const UA_NodeId& dataTypeNodeId, UA_Variant* var);
    using DurationTimeStamp = std::chrono::steady_clock::time_point;
    DurationTimeStamp GetDurationTimeStamp();

    OpcUaObject<UA_ByteString> LoadFile(const std::string& path);
};

END_NAMESPACE_OPENDAQ_OPCUA
