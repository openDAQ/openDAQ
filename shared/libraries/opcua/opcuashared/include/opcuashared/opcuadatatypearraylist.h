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

#include <opcuashared/opcua.h>
#include <list>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaDataTypeArrayList : protected std::list<UA_DataTypeArray>
{
public:
    using std::list<UA_DataTypeArray>::list;

    OpcUaDataTypeArrayList(const OpcUaDataTypeArrayList& copy);
    OpcUaDataTypeArrayList& operator=(const OpcUaDataTypeArrayList& other);

    void add(const size_t typesSize, const UA_DataType* types);
    const UA_DataTypeArray* getCustomDataTypes() const;
};

END_NAMESPACE_OPENDAQ_OPCUA
