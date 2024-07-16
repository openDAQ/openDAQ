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

#include "opcuacommon.h"
#include "opcuavariant.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaDataValue;
using OpcUaDataValuePtr = std::shared_ptr<OpcUaDataValue>;

class OpcUaDataValue
{
public:
    OpcUaDataValue(const UA_DataValue* dataValue);
    virtual ~OpcUaDataValue();

    bool hasValue() const;
    const OpcUaVariant& getValue() const;
    const UA_StatusCode& getStatusCode() const;

    bool isStatusOK() const;

    const UA_DataValue* getDataValue() const;
    operator const UA_DataValue*() const;

protected:
    const UA_DataValue* dataValue;
    const OpcUaVariant variant;
};

END_NAMESPACE_OPENDAQ_OPCUA
