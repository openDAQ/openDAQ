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
 * OPC UA library; is included in every class across the solution.
 */
#pragma once
#define BEGIN_NAMESPACE_OPENDAQ_OPCUA        \
    namespace daq::opcua   \
    {                         
    
#define END_NAMESPACE_OPENDAQ_OPCUA          \
            }               

#include <sstream>

// clang compiler on macOS emits warnings when compiling code in types.h,
// so we want to silence it here
#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    #pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

#include <open62541/types.h>
#include <open62541/types_generated.h>

#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

BEGIN_NAMESPACE_OPENDAQ_OPCUA

enum class OpcUaNodeClass
{
    Object = UA_NODECLASS_OBJECT,
    Variable = UA_NODECLASS_VARIABLE,
    Method = UA_NODECLASS_METHOD,
    ObjectType = UA_NODECLASS_OBJECTTYPE,
    VariableType = UA_NODECLASS_VARIABLETYPE,
    ReferenceType = UA_NODECLASS_REFERENCETYPE,
    DataType = UA_NODECLASS_DATATYPE,
    View = UA_NODECLASS_VIEW,

    All = Object | Variable | Method | ObjectType | VariableType | ReferenceType | DataType | View  // Mask
};

enum class OpcUaBrowseDirection
{
    Forward = UA_BROWSEDIRECTION_FORWARD,
    Inverse = UA_BROWSEDIRECTION_INVERSE,
    Both = UA_BROWSEDIRECTION_BOTH
};

#define OPCUA_STATUSCODE_FAILED(x) ((x) &0x80000000)
#define OPCUA_STATUSCODE_SUCCEEDED(x) (!OPCUA_STATUSCODE_FAILED(x))
#define OPCUA_STATUSCODE_IS_GOOD(x) (x == UA_STATUSCODE_GOOD)
#define OPCUA_STATUSCODE_NOT_CONNECTED(x) ((x == UA_STATUSCODE_BADDISCONNECT) || (x == UA_STATUSCODE_BADCONNECTIONCLOSED) || (x == UA_STATUSCODE_BADNOTCONNECTED))

#define OPCUA_STATUSCODE_LOG_MESSAGE(STATUS_CODE)                                                                                             \
    "StatusCode 0x" << std::hex << std::uppercase << STATUS_CODE << std::nouppercase << std::dec << "(" << UA_StatusCode_name(STATUS_CODE) \
                    << ")"

#define THROW_RUNTIME_ERROR(x)                             \
    {                                                      \
        std::stringstream excStream;                       \
        excStream << x;                                    \
        throw std::runtime_error(excStream.str().c_str()); \
    }

inline bool operator==(const UA_String& l, const UA_String& r)
{
    return UA_String_equal(&l, &r);
}

inline bool operator!=(const UA_String& l, const UA_String& r)
{
    return !(l == r);
}

inline bool operator==(const UA_String& l, const char* r)
{
    UA_String rStr{};
    if (r)
    {
        rStr.length = strlen(r);
        rStr.data = (UA_Byte*) r;
    }

    return l == rStr;
}

inline bool operator!=(const UA_String& l, const char* r)
{
    return !(l == r);
}

inline bool operator==(const char* l, const UA_String& r)
{
    return r == l;
}

inline bool operator!=(const char* l, const UA_String& r)
{
    return r != l;
}

END_NAMESPACE_OPENDAQ_OPCUA
