/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <iomanip>

#include "open62541/util.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

namespace utils
{
    inline std::string ToStdString(const UA_String& value)
    {
        return std::string((const char*) value.data, value.length);
    }

    inline std::string GuidToString(const UA_Guid& guid)
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0')
            << std::setw(8) << guid.data1 << "-"
            << std::setw(4) << guid.data2 << "-"
            << std::setw(4) << guid.data3 << "-"
            << std::setw(2) << static_cast<int>(guid.data4[0])
            << std::setw(2) << static_cast<int>(guid.data4[1]) << "-"
            << std::setw(2) << static_cast<int>(guid.data4[2])
            << std::setw(2) << static_cast<int>(guid.data4[3])
            << std::setw(2) << static_cast<int>(guid.data4[4])
            << std::setw(2) << static_cast<int>(guid.data4[5])
            << std::setw(2) << static_cast<int>(guid.data4[6])
            << std::setw(2) << static_cast<int>(guid.data4[7]);

        return oss.str();
    }

    double ToSeconds(const UA_DateTime& time);
    UA_StatusCode ToUaVariant(double value, const UA_NodeId& dataTypeNodeId, UA_Variant* var);
    void ToUaVariant(const std::string& value, const UA_NodeId& dataTypeNodeId, UA_Variant* var);
    using DurationTimeStamp = std::chrono::steady_clock::time_point;
    DurationTimeStamp GetDurationTimeStamp();

    OpcUaObject<UA_ByteString> LoadFile(const std::string& path);
};

END_NAMESPACE_OPENDAQ_OPCUA
